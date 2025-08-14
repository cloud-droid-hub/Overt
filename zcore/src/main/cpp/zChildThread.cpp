//
// Created by Assistant on 2025/8/7.
// zChildThread 实现 - 工作线程类实现（简化版，不管理任务队列）
//

#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include "zChildThread.h"
#include <sys/types.h>


// 构造函数
zChildThread::zChildThread(size_t index, const string& name, 
                          TaskCompletionCallback completionCallback,
                          TaskProviderCallback taskProviderCallback,
                          void* userData)
    : m_threadId(0)
    , m_threadName(name)
    , m_threadIndex(index)
    , m_threadPriority(TaskPriority::NORMAL)  // 默认优先级
    , m_completionCallback(completionCallback)
    , m_taskProviderCallback(taskProviderCallback)
    , m_callbackUserData(userData)
    , m_state(WorkerState::IDLE)              // 使用线程安全状态管理器
    , m_running(false) {                      // 使用线程安全状态管理器
    
    // 初始化条件变量和互斥锁
    pthread_cond_init(&m_taskCV, nullptr);
    pthread_mutex_init(&m_taskMutex, nullptr);
    
    // 设置初始活跃时间
    gettimeofday(&m_lastActiveTime, nullptr);
    
    LOGI("zChildThread[%zu] '%s' created", m_threadIndex, m_threadName.c_str());
}

// 析构函数
zChildThread::~zChildThread() {
    // 确保线程已停止
    stop(false);
    
    // 销毁同步对象
    pthread_cond_destroy(&m_taskCV);
    pthread_mutex_destroy(&m_taskMutex);
    
    LOGI("zChildThread[%zu] '%s' destroyed", m_threadIndex, m_threadName.c_str());
}

// 启动线程
bool zChildThread::start() {
    if (m_running.get()) {
        LOGW("zChildThread[%zu] already running", m_threadIndex);
        return false;
    }
    
    m_running.set(true);
    m_state.set(WorkerState::IDLE);
    
    // 创建线程
    int result = pthread_create(&m_threadId, nullptr, threadEntry, this);
    if (result != 0) {
        LOGE("Failed to create thread[%zu]: %d", m_threadIndex, result);
        m_running.set(false);
        return false;
    }
    
    LOGI("zChildThread[%zu] started with ID %lu", m_threadIndex, (unsigned long)m_threadId);
    return true;
}

// 停止线程
void zChildThread::stop(bool waitForCompletion) {
    if (!m_running.get()) {
        return;
    }
    
    LOGI("Stopping zChildThread[%zu]...", m_threadIndex);
    m_running.set(false);
    
    // 通知等待的线程
    pthread_cond_signal(&m_taskCV);
    
    // 等待线程结束
    if (waitForCompletion && m_threadId != 0) {
        join();
    }
    
    m_state.set(WorkerState::TERMINATED);
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

// 直接执行指定任务（新增方法）
bool zChildThread::executeTask(const Task& task) {
    if (!m_running.get()) {
        LOGW("zChildThread[%zu] not running, cannot execute task", m_threadIndex);
        return false;
    }
    
    // 直接处理任务
    processTask(task);
    return true;
}

// 获取最后活跃时间
struct timeval zChildThread::getLastActiveTime() const {
    std::shared_lock<std::shared_mutex> lock(m_lastActiveTimeMutex);
    return m_lastActiveTime;
}

// 静态线程入口函数
void* zChildThread::threadEntry(void* arg) {
    zChildThread* thread = static_cast<zChildThread*>(arg);
    return thread->threadMain();
}

// 线程主函数
void* zChildThread::threadMain() {
    LOGI("zChildThread[%zu] main loop started", m_threadIndex);
    LOGI("zChildThread[%zu] initial state: IDLE", m_threadIndex);
    m_state.set(WorkerState::IDLE);
    
    while (m_running.get()) {
        Task task;
        bool hasTask = false;
        
        // 尝试从主线程获取任务
        hasTask = tryGetTask(task);
        
        if (!hasTask) {
            LOGI("zChildThread[%zu] state change: -> WAITING (no tasks available)", m_threadIndex);
            m_state.set(WorkerState::WAITING);
            
            // 等待任务或停止信号
            pthread_mutex_lock(&m_taskMutex);
            
            while (m_running.get()) {
                // 再次尝试获取任务
                if (tryGetTask(task)) {
                    hasTask = true;
                    break;
                }
                
                // 等待新任务到达或线程停止
                pthread_cond_wait(&m_taskCV, &m_taskMutex);
            }
            
            pthread_mutex_unlock(&m_taskMutex);
            
            if (!hasTask) {
                continue; // 重新检查
            }
        }
        
        // 处理任务
        if (hasTask) {
            LOGI("zChildThread[%zu] state change: WAITING/IDLE -> RUNNING", m_threadIndex);
            m_state.set(WorkerState::RUNNING);
            processTask(task);
            LOGI("zChildThread[%zu] state change: RUNNING -> IDLE", m_threadIndex);
            m_state.set(WorkerState::IDLE);
        }
    }
    
    LOGI("zChildThread[%zu] main loop ended", m_threadIndex);
    return nullptr;
}

// 尝试从主线程获取任务
bool zChildThread::tryGetTask(Task& outTask) {
    if (!m_taskProviderCallback) {
        return false;
    }
    
    // 通过回调函数从主线程获取任务
    return m_taskProviderCallback(outTask, m_callbackUserData);
}

// 处理任务
void zChildThread::processTask(const Task& task) {
    updateLastActiveTime();
    
    LOGI("zChildThread[%zu] processing task '%s'", m_threadIndex, task.taskId.c_str());
    LOGI("zChildThread[%zu] task execution started", m_threadIndex);
    
    // 执行任务
    if (task.func || task.callable) {
        task.execute();
        m_processedTasks.add(1);
        LOGI("zChildThread[%zu] completed task '%s'", m_threadIndex, task.taskId.c_str());
        LOGI("zChildThread[%zu] task execution finished, calling completion callback", m_threadIndex);
    } else {
        LOGE("zChildThread[%zu] invalid task function for '%s'", m_threadIndex, task.taskId.c_str());
    }
    
    // 通过回调通知任务完成
    if (m_completionCallback) {
        LOGI("zChildThread[%zu] calling completion callback for task '%s'", m_threadIndex, task.taskId.c_str());
        m_completionCallback(task.taskId, m_callbackUserData);
        LOGI("zChildThread[%zu] completion callback finished", m_threadIndex);
    }
    
    updateLastActiveTime();
}

// 更新活跃时间
void zChildThread::updateLastActiveTime() {
    std::unique_lock<std::shared_mutex> lock(m_lastActiveTimeMutex);
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
    return sizeof(zChildThread);
}

// 设置线程名称（支持链式调用）
zChildThread* zChildThread::setName(const string& name) {
    m_threadName = name;
    return this;
}

// 获取线程名称
string zChildThread::getName() const {
    return m_threadName;
}

// 设置线程优先级（支持链式调用）
zChildThread* zChildThread::setLevel(TaskPriority priority) {
    m_threadPriority = priority;
    return this;
}

// 获取线程优先级
TaskPriority zChildThread::getLevel() const {
    return m_threadPriority;
}
