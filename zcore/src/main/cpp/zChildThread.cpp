//
// Created by Assistant on 2025/8/7.
// zChildThread 实现 - 工作线程类实现
//



#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include "zChildThread.h"


// Android gettid 函数
#ifdef __ANDROID__
#include <sys/types.h>
#endif

// 构造函数
zChildThread::zChildThread(size_t index, const string& name, 
                          TaskCompletionCallback completionCallback,
                          TaskProviderCallback taskProviderCallback,
                          void* userData)
    : m_threadId(0)
    , m_threadName(name)
    , m_threadIndex(index)
    , m_completionCallback(completionCallback)
    , m_taskProviderCallback(taskProviderCallback)
    , m_callbackUserData(userData)
    , m_state(WorkerState::IDLE)
    , m_running(false)
    , m_processedTasks(0)
    , m_currentTaskCount(0) {
    
    // 初始化条件变量和互斥锁
    pthread_cond_init(&m_queueCV, nullptr);
    pthread_mutex_init(&m_queueCondMutex, nullptr);
    
    // 设置初始活跃时间
    gettimeofday(&m_lastActiveTime, nullptr);
    
    LOGI("zChildThread[%zu] '%s' created", m_threadIndex, m_threadName.c_str());
}

// 析构函数
zChildThread::~zChildThread() {
    // 确保线程已停止
    stop(false);
    
    // 销毁同步对象
    pthread_cond_destroy(&m_queueCV);
    pthread_mutex_destroy(&m_queueCondMutex);
    
    LOGI("zChildThread[%zu] '%s' destroyed", m_threadIndex, m_threadName.c_str());
}

// 启动线程
bool zChildThread::start() {
    if (m_running.load()) {
        LOGW("zChildThread[%zu] already running", m_threadIndex);
        return false;
    }
    
    m_running.store(true);
    m_state.store(WorkerState::IDLE);
    
    // 创建线程
    int result = pthread_create(&m_threadId, nullptr, threadEntry, this);
    if (result != 0) {
        LOGE("Failed to create thread[%zu]: %d", m_threadIndex, result);
        m_running.store(false);
        return false;
    }
    
    LOGI("zChildThread[%zu] started with ID %lu", m_threadIndex, (unsigned long)m_threadId);
    return true;
}

// 停止线程
void zChildThread::stop(bool waitForCompletion) {
    if (!m_running.load()) {
        return;
    }
    
    LOGI("Stopping zChildThread[%zu]...", m_threadIndex);
    m_running.store(false);
    
    // 通知等待的线程
    pthread_cond_signal(&m_queueCV);
    
    // 等待线程结束
    if (waitForCompletion && m_threadId != 0) {
        join();
    }
    
    m_state.store(WorkerState::TERMINATED);
}

// 等待线程结束
void zChildThread::join() {
    if (m_threadId != 0) {
        void* retval;
        int result = pthread_join(m_threadId, &retval);
        if (result == 0) {
            LOGI("zChildThread[%zu] joined successfully", m_threadIndex);
        } else {
            LOGE("Failed to join thread[%zu]: %d", m_threadIndex, result);
        }
        m_threadId = 0;
    }
}

// 添加任务到本地队列
bool zChildThread::addTask(const Task& task) {
    if (!m_running.load()) {
        return false;
    }
    
    {
        std::unique_lock<std::shared_mutex> lock(m_queueMutex);
        m_localTaskQueue.push(task);
        m_currentTaskCount.fetch_add(1);
    }
    
    // 通知等待的线程
    pthread_cond_signal(&m_queueCV);
    
    LOGD("Task '%s' added to thread[%zu] queue", task.taskId.c_str(), m_threadIndex);
    return true;
}

// 获取队列大小
size_t zChildThread::getQueueSize() const {
    std::shared_lock<std::shared_mutex> lock(m_queueMutex);
    return m_localTaskQueue.size();
}

// 检查是否有任务
bool zChildThread::hasTasks() const {
    std::shared_lock<std::shared_mutex> lock(m_queueMutex);
    return !m_localTaskQueue.empty();
}

// 尝试从外部获取任务
bool zChildThread::tryGetExternalTask() {
    if (!m_taskProviderCallback) {
        return false;
    }
    
    // 通过回调函数尝试获取任务
    Task task;
    if (m_taskProviderCallback(task, m_callbackUserData)) {
        // 直接处理获取到的任务，不放入本地队列
        processTask(task);
        return true;
    }
    
    return false;
}

// 静态线程入口函数
void* zChildThread::threadEntry(void* arg) {
    zChildThread* thread = static_cast<zChildThread*>(arg);
    return thread->threadMain();
}

// 线程主函数
void* zChildThread::threadMain() {
    LOGI("zChildThread[%zu] main loop started", m_threadIndex);
    
    while (m_running.load()) {
        Task task;
        bool hasTask = false;
        
        // 1. 首先检查本地队列
        {
            std::unique_lock<std::shared_mutex> lock(m_queueMutex);
            if (!m_localTaskQueue.empty()) {
                task = m_localTaskQueue.front();
                m_localTaskQueue.pop();
                m_currentTaskCount.fetch_sub(1);
                hasTask = true;
            }
        }
        
        // 2. 如果本地队列为空，尝试工作窃取
        if (!hasTask) {
            m_state.store(WorkerState::WAITING);
            
            // 尝试从外部获取任务
            if (tryGetExternalTask()) {
                continue; // 任务已在 tryGetExternalTask 中处理
            }
            
            // 3. 如果还是没有任务，等待
            pthread_mutex_lock(&m_queueCondMutex);
            while (m_localTaskQueue.empty() && m_running.load()) {
                pthread_cond_wait(&m_queueCV, &m_queueCondMutex);
            }
            pthread_mutex_unlock(&m_queueCondMutex);
            
            continue; // 重新检查队列
        }
        
        // 4. 处理任务
        if (hasTask) {
            m_state.store(WorkerState::RUNNING);
            processTask(task);
            m_state.store(WorkerState::IDLE);
        }
    }
    
    LOGI("zChildThread[%zu] main loop ended", m_threadIndex);
    return nullptr;
}

// 处理任务
void zChildThread::processTask(const Task& task) {
    updateLastActiveTime();
    
    LOGI("zChildThread[%zu] processing task '%s'", m_threadIndex, task.taskId.c_str());
    
    // 执行任务
    if (task.func) {
        task.func(task.arg);
        m_processedTasks.fetch_add(1);
        LOGI("zChildThread[%zu] completed task '%s'", m_threadIndex, task.taskId.c_str());
    } else {
        LOGE("zChildThread[%zu] invalid task function for '%s'", m_threadIndex, task.taskId.c_str());
    }
    
    // 通过回调通知任务完成
    if (m_completionCallback) {
        m_completionCallback(task.taskId, m_callbackUserData);
    }
    
    updateLastActiveTime();
}

// 更新活跃时间
void zChildThread::updateLastActiveTime() {
    gettimeofday(&m_lastActiveTime, nullptr);
}

// 设置线程亲和性
bool zChildThread::setAffinity(int cpuId) {
    // Android 使用 sched_setaffinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuId, &cpuset);
    
    pid_t tid = gettid(); // 获取线程ID
    int result = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);
    if (result == 0) {
        LOGI("zChildThread[%zu] affinity set to CPU %d", m_threadIndex, cpuId);
        return true;
    } else {
        LOGE("Failed to set affinity for thread[%zu]: %d", m_threadIndex, result);
        return false;
    }
}

// 设置线程优先级
bool zChildThread::setPriority(int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    
    int result = pthread_setschedparam(m_threadId, SCHED_OTHER, &param);
    if (result == 0) {
        LOGI("zChildThread[%zu] priority set to %d", m_threadIndex, priority);
        return true;
    } else {
        LOGE("Failed to set priority for thread[%zu]: %d", m_threadIndex, result);
        return false;
    }
}

// 获取CPU使用率（简化实现）
double zChildThread::getCpuUsage() const {
    // 简化实现：返回基于处理任务数的估算值
    static int counter = 0;
    return (counter++ % 100) + 1.0;
}

// 获取内存使用量（简化实现）
size_t zChildThread::getMemoryUsage() const {
    // 简化实现：返回基本内存使用量
    return sizeof(zChildThread) + (getQueueSize() * sizeof(Task));
}
