//
// Created by lxz on 2025/8/7.
//

// 包含线程管理类的头文件

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "zStd.h"
#include "zStdUtil.h"
#include "zThread.h"
#include "zChildThread.h"


// 静态单例实例指针 - 用于实现单例模式
zThread* zThread::instance = nullptr;

// ThreadArg 结构体已被 zChildThread 替代，不再需要

/**
 * zThread构造函数
 * 初始化线程管理器，设置默认参数
 */
zThread::zThread() : m_name("zThread"), m_maxThreads(4), m_running(false), m_activeThreads(0) {
    // shared_mutex 有默认构造函数，不需要手动初始化
    // m_taskQueueMutex 和 m_activeTasksMutex 会自动初始化
    
    // 初始化任务队列条件变量
    pthread_cond_init(&m_taskQueueCV, nullptr);
    
    // 初始化任务完成条件变量
    pthread_cond_init(&m_taskCompletionCV, nullptr);
    pthread_mutex_init(&m_taskCompletionMutex, nullptr);
    
    // 消息队列现在使用 ThreadSafeQueue，不需要额外的锁初始化
}

/**
 * 获取zThread单例实例
 * 采用懒加载模式，首次调用时创建实例
 * @return zThread单例指针
 */
zThread* zThread::getInstance() {
    // 检查实例是否已存在，如果不存在则创建新实例
    if (instance == nullptr) {
        instance = new zThread();
        // 只在首次创建时启动线程池
        instance->startThreadPool(4);
    }
    return instance;
}

// 析构函数实现 - 清理所有资源
zThread::~zThread() {
    // 停止线程池并等待所有任务完成
    stopThreadPool(true);
    
    // 清理所有工作线程对象
    {
        std::unique_lock<std::shared_mutex> lock(m_workerThreadsMutex);
        for (auto* worker : m_workerThreads) {
            delete worker;
        }
        m_workerThreads.clear();
    }
    
    // 销毁任务队列条件变量（shared_mutex 会自动析构）
    pthread_cond_destroy(&m_taskQueueCV);
    
    // 销毁任务完成条件变量
    pthread_cond_destroy(&m_taskCompletionCV);
    pthread_mutex_destroy(&m_taskCompletionMutex);
    
    // 消息队列现在使用 ThreadSafeQueue，不需要额外的锁销毁
    
    // shared_mutex m_activeTasksMutex 会自动析构
    
    // 销毁所有创建的互斥锁
    for (auto& mutex : m_mutexes) {
        pthread_mutex_destroy(&mutex);
    }
    
    // 销毁所有创建的条件变量
    for (auto& cv : m_conditionVariables) {
        pthread_cond_destroy(&cv);
    }
    
    // 销毁所有创建的读写锁
    for (auto& sharedMutex : m_sharedMutexes) {
        delete sharedMutex;
    }
}

// 旧的 workerThreadFunction 已被 zChildThread 内部线程函数替代

// 线程池管理实现 - 启动线程池
bool zThread::startThreadPool(size_t threadCount) {
    // 检查线程池是否已经在运行
    if (m_running) {
        LOGW("startThreadPool: Thread pool is already running");
        return false;
    }
    
    LOGI("startThreadPool: Starting thread pool with %zu requested threads (max: %zu)", 
         threadCount, m_maxThreads);
    
    // 计算实际创建的线程数，不超过最大线程数限制
    size_t actualThreadCount = (threadCount < m_maxThreads) ? threadCount : m_maxThreads;
    if (actualThreadCount != threadCount) {
        LOGW("startThreadPool: Thread count limited from %zu to %zu (max threads)", 
             threadCount, actualThreadCount);
    }
    
    m_running = true;  // 设置运行标志
    
    // 创建 zChildThread 对象
    for (size_t i = 0; i < actualThreadCount; ++i) {
        string threadName = m_name + "_worker_" + itoa(i, 10);
        
        LOGI("startThreadPool: Creating worker thread %zu with name '%s'", i, threadName.c_str());
        
        // 创建工作线程对象（使用回调函数实现解耦）
        zChildThread* worker = new zChildThread(i, threadName, 
                                               &zThread::onTaskCompleted,  // 任务完成回调
                                               &zThread::provideTask,      // 任务提供者回调
                                               this);                      // 用户数据（this指针）
        
        // 启动线程
        if (!worker->start()) {
            LOGE("startThreadPool: Failed to start worker thread %zu", i);
            delete worker;
            m_running = false;
            return false;
        }
        
        LOGI("startThreadPool: Worker thread %zu started successfully", i);
        
        // 保存工作线程对象
        {
            std::unique_lock<std::shared_mutex> lock(m_workerThreadsMutex);
            m_workerThreads.push_back(worker);
        }
    }
    
    LOGI("startThreadPool: Thread pool started successfully with %zu worker threads", actualThreadCount);
    return true;
}

// 停止线程池
void zThread::stopThreadPool(bool waitForCompletion) {
    // 如果线程池已经停止，直接返回
    if (!m_running) {
        return;
    }
    
    m_running = false;  // 设置停止标志
    
    // 停止所有工作线程
    {
        std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
        for (auto* worker : m_workerThreads) {
            worker->stop(waitForCompletion);
        }
    }
    
    // 清空线程列表（对象在析构函数中删除）
    {
        std::unique_lock<std::shared_mutex> lock(m_workerThreadsMutex);
        m_workerThreads.clear();
    }
    
    LOGI("Thread pool stopped");
}

// 检查线程池是否正在运行
bool zThread::isThreadPoolRunning() const {
    return m_running;
}

// 获取活跃线程数量
size_t zThread::getActiveThreadCount() const {
    // 返回工作线程的数量
    std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
    return m_workerThreads.size();
}

// 获取正在执行任务的线程数量
size_t zThread::getExecutingTaskCount() {
    // 统计状态为RUNNING的线程数
    size_t executingCount = 0;
    
    {
        std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
        LOGI("getExecutingTaskCount: Checking %zu worker threads", m_workerThreads.size());
        
        for (const auto* worker : m_workerThreads) {
            WorkerState state = worker->getState();
            LOGI("getExecutingTaskCount: Thread %lu (%s) state=%d", 
                 (unsigned long)worker->getThreadId(), 
                 worker->getThreadName().c_str(), 
                 static_cast<int>(state));
                 
            if (state == WorkerState::RUNNING) {
                executingCount++;
                LOGI("getExecutingTaskCount: Found executing thread %lu", (unsigned long)worker->getThreadId());
            }
        }
    }
    
    LOGI("getExecutingTaskCount: Total executing threads = %zu", executingCount);
    return executingCount;
}

// 获取活跃任务数量
size_t zThread::getActiveTaskCount() {
    // 使用线程安全映射，无需额外加锁
    size_t activeCount = m_activeTasks.size();
    
    LOGI("getActiveTaskCount: Active tasks in m_activeTasks = %zu", activeCount);
    if (activeCount > 0) {
        LOGI("getActiveTaskCount: Active task IDs:");
        m_activeTasks.for_each([](const string& taskId, const Task& task) {
            LOGI("  - %s", taskId.c_str());
        });
    }
    
    return activeCount;
}

// 获取待处理任务总数
size_t zThread::getPendingTaskCount() {
    // 使用新的高性能方法读取任务数量
    LOGI("getPendingTaskCount: Starting calculation");
    
    size_t queueCount = getQueuedTaskCount();
    size_t activeTasksCount = getActiveTaskCount();
    size_t pendingTotal = queueCount + activeTasksCount;
    
    LOGI("getPendingTaskCount: queued=%zu + active=%zu = pending=%zu", 
         queueCount, activeTasksCount, pendingTotal);
    
    // 返回总的待处理任务数：队列中的 + 活跃任务中的
    return pendingTotal;
}

// 获取队列中的任务数量
size_t zThread::getQueuedTaskCount() {
    // 只统计中央队列，子线程不再有本地队列
    size_t centralQueueSize = 0;
    
    LOGI("getQueuedTaskCount: Starting queue count calculation");
    
    // 中央队列（使用线程安全队列，无需额外加锁）
    centralQueueSize = m_taskQueue.size();
    
    LOGI("getQueuedTaskCount: Central queue size = %zu", centralQueueSize);
    
    return centralQueueSize;
}

// 检查任务是否活跃
bool zThread::isTaskActive(const string& taskId) {
    // 使用线程安全映射，无需额外加锁
    return m_activeTasks.contains(taskId);
}

// 取消任务 - 从活跃任务列表或队列中移除指定任务
bool zThread::cancelTask(const string& taskId) {
    // 首先尝试从活跃任务列表中移除
    if (removeActiveTask(taskId)) {
        logThreadEvent("Task cancelled: " + taskId, pthread_self());
        return true;
    }
    
    // 尝试从中央队列中移除任务
    size_t removedFromQueue = m_taskQueue.remove_if([&taskId](const Task& task) {
        return task.taskId == taskId;
    });
    
    if (removedFromQueue > 0) {
        logThreadEvent("Task cancelled from queue: " + taskId, pthread_self());
        return true;
    }
    
    logThreadEvent("Task not found for cancellation: " + taskId, pthread_self());
    return false;  // 任务不存在，取消失败
}

bool zThread::waitForAllTasks(int timeoutMs) {
    struct timeval startTime;
    gettimeofday(&startTime, nullptr);
    
    if (timeoutMs == 0) {
        LOGI("waitForAllTasks: Starting wait without timeout (infinite wait until all tasks complete)");
    } else {
        LOGI("waitForAllTasks: Starting wait with timeout %d ms", timeoutMs);
    }
    
    int loopCount = 0;
    while (true) {
        loopCount++;
        
        // 简单检查所有任务状态
        size_t activeCount = m_activeTasks.size();
        size_t queuedCount = m_taskQueue.size();
        size_t executingCount = 0;
        size_t workerCount = 0;
        
        // 检查工作线程状态
        {
            std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
            workerCount = m_workerThreads.size();
            for (const auto* worker : m_workerThreads) {
                if (worker->getState() == WorkerState::RUNNING) {
                    executingCount++;
                }
            }
        }
        
        // 记录任务状态（每10次循环记录一次，避免日志过多）
        if (loopCount % 10 == 1) {
            LOGI("waitForAllTasks: Loop %d - active=%zu, queued=%zu, executing=%zu, workers=%zu", 
                 loopCount, activeCount, queuedCount, executingCount, workerCount);
            
            // 如果任务数量很少，输出详细的工作线程状态
            if (activeCount + queuedCount + executingCount <= 5) {
                std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
                for (size_t i = 0; i < m_workerThreads.size(); ++i) {
                    const auto* worker = m_workerThreads[i];
                    LOGI("waitForAllTasks: Worker[%zu] state=%d", 
                         i, static_cast<int>(worker->getState()));
                }
            }
        }
        
        // 所有任务完成
        if (activeCount == 0 && queuedCount == 0 && executingCount == 0) {
            struct timeval endTime;
            gettimeofday(&endTime, nullptr);
            long totalElapsed = (endTime.tv_sec - startTime.tv_sec) * 1000 + 
                               (endTime.tv_usec - startTime.tv_usec) / 1000;
            LOGI("waitForAllTasks: All tasks completed after %ld ms (%d loops)", totalElapsed, loopCount);
            return true;
        }
        
        // 检查超时
        if (timeoutMs > 0) {  // 只有正值才检查超时，0表示无限等待直到所有任务完成
            struct timeval currentTime;
            gettimeofday(&currentTime, nullptr);
            long elapsed = (currentTime.tv_sec - startTime.tv_sec) * 1000 + 
                          (currentTime.tv_usec - startTime.tv_usec) / 1000;
            
            if (elapsed >= timeoutMs) {
                LOGW("waitForAllTasks: Timeout reached after %ld ms - active=%zu, queued=%zu, executing=%zu", 
                     elapsed, activeCount, queuedCount, executingCount);
                return false;
            }
        }
        // 如果 timeoutMs = 0，继续循环直到所有任务完成
        
        // 50ms 轮询间隔
        struct timespec ts = {0, 50 * 1000000}; // 50ms
        nanosleep(&ts, nullptr);
    }
}

// 线程同步实现
pthread_mutex_t* zThread::createMutex() {
    pthread_mutex_t* mutex = new pthread_mutex_t();
    pthread_mutex_init(mutex, nullptr);
    m_mutexes.push_back(*mutex);
    return mutex;
}

pthread_cond_t* zThread::createConditionVariable() {
    pthread_cond_t* cv = new pthread_cond_t();
    pthread_cond_init(cv, nullptr);
    m_conditionVariables.push_back(*cv);
    return cv;
}

int* zThread::createSemaphore(int initialCount) {
    int* semaphore = new int(initialCount);
    m_semaphores.push_back(semaphore);
    return semaphore;
}

std::shared_mutex* zThread::createSharedMutex() {
    std::shared_mutex* sharedMutex = new std::shared_mutex();
    m_sharedMutexes.push_back(sharedMutex);
    return sharedMutex;
}

// 线程间通信实现
bool zThread::sendMessage(pthread_t threadId, const string& message) {
    // 简化实现：广播消息到所有线程
    broadcastMessage(message);
    return true;
}

string zThread::receiveMessage(int timeoutMs) {
    // 使用线程安全队列的阻塞等待功能
    string message;
    
    if (m_messageQueue.wait_and_pop(message, timeoutMs)) {
        return message;  // 成功获取消息
    }
    
    return "";  // 超时或失败时返回空字符串
}

void zThread::broadcastMessage(const string& message) {
    // 使用线程安全队列，自动通知等待的线程
    m_messageQueue.push(message);
}

// 线程监控实现
vector<ThreadInfo> zThread::getThreadInfo() const {
    vector<ThreadInfo> result;
    {
        std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
        for (const auto* worker : m_workerThreads) {
            ThreadInfo info;
            info.threadId = worker->getThreadId();
            info.name = worker->getThreadName();
            // 将 WorkerState 转换为 ThreadState
            switch (worker->getState()) {
                case WorkerState::IDLE:
                    info.state = ThreadState::IDLE;
                    break;
                case WorkerState::RUNNING:
                    info.state = ThreadState::RUNNING;
                    break;
                case WorkerState::WAITING:
                    info.state = ThreadState::IDLE; // 等待状态映射为空闲
                    break;
                case WorkerState::TERMINATED:
                    info.state = ThreadState::TERMINATED;
                    break;
            }
            info.lastActiveTime = worker->getLastActiveTime();
            result.push_back(info);
        }
    }
    return result;
}

ThreadState zThread::getThreadState(pthread_t threadId) const {
    {
        std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
        for (const auto* worker : m_workerThreads) {
            if (pthread_equal(worker->getThreadId(), threadId)) {
                // 将 WorkerState 转换为 ThreadState
                switch (worker->getState()) {
                    case WorkerState::IDLE:
                        return ThreadState::IDLE;
                    case WorkerState::RUNNING:
                        return ThreadState::RUNNING;
                    case WorkerState::WAITING:
                        return ThreadState::IDLE; // 等待状态映射为空闲
                    case WorkerState::TERMINATED:
                        return ThreadState::TERMINATED;
                }
            }
        }
    }
    return ThreadState::TERMINATED;
}

double zThread::getThreadCpuUsage(pthread_t threadId) const {
    // 简化实现：返回随机值
    static int counter = 0;
    return (counter++ % 100) + 1.0;
}

size_t zThread::getThreadMemoryUsage(pthread_t threadId) const {
    // 简化实现：返回固定值
    return 1024 * 1024; // 1MB
}

// 工具方法实现
pthread_t zThread::getCurrentThreadId() {
    return pthread_self();
}

string zThread::getCurrentThreadName() {
    // 简化实现：返回线程ID的字符串表示
    pthread_t tid = pthread_self();
    return "Thread_" + itoa((unsigned long)tid, 10);
}

void zThread::setCurrentThreadName(const string& name) {
    // 在大多数平台上，设置线程名称需要平台特定的API
    // 这里只是记录日志
    LOGI("Setting thread name to: %s", name.c_str());
}

void zThread::sleep(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, nullptr);
}

void zThread::yield() {
    sched_yield();
}

// 私有方法实现
void zThread::processTask(Task* task) {
    if (task && task->func) {
        logThreadEvent("Processing task: " + task->taskId, pthread_self());
        task->func(task->arg);
        logThreadEvent("Task completed: " + task->taskId, pthread_self());
        
        // 任务已经在工作线程中被从活跃任务列表中移除，这里不需要再次移除
        LOGI("Task %s processing completed", task->taskId.c_str());
    } else {
        LOGE("processTask: Invalid task or function");
    }
}

void zThread::updateThreadState(size_t threadIndex, ThreadState state) {
    // 线程状态现在由 zChildThread 自己管理
    // 这个方法保留以维持向后兼容性，但实际不做任何操作
    LOGI("updateThreadState called for thread %zu with state %d (now managed by zChildThread)", 
         threadIndex, static_cast<int>(state));
}

void zThread::cleanupTerminatedThreads() {
    // 清理已终止的工作线程对象
    std::unique_lock<std::shared_mutex> lock(m_workerThreadsMutex);
    auto it = m_workerThreads.begin();
    while (it != m_workerThreads.end()) {
        if ((*it)->getState() == WorkerState::TERMINATED) {
            delete *it;
            it = m_workerThreads.erase(it);
            LOGI("Cleaned up terminated worker thread");
        } else {
            ++it;
        }
    }
}

string zThread::generateTaskId() const {
    static int counter = 0;
    return "task_" + itoa(pthread_self(), 10) + "_" + itoa(__sync_fetch_and_add(&counter, 1), 10);
}

void zThread::logThreadEvent(const string& event, pthread_t threadId) const {
    string message = "[" + m_name + "] ";
    if (threadId != 0) {
        message += "Thread " + itoa((unsigned long)threadId, 10) + ": ";
    }
    message += event;
    LOGI("%s", message.c_str());
}

// 延迟任务包装器函数实现 - 在指定延迟后执行原始任务
void delayedTaskWrapper(void* arg) {
    // 将参数转换为延迟任务包装器
    DelayedTaskWrapper* wrapper = static_cast<DelayedTaskWrapper*>(arg);
    
    LOGI("DelayedTaskWrapper: Starting delay for %d ms", wrapper->delayMs);
    
    // 设置延迟时间结构体
    struct timespec ts;
    ts.tv_sec = wrapper->delayMs / 1000;        // 秒数部分
    ts.tv_nsec = (wrapper->delayMs % 1000) * 1000000;  // 纳秒部分
    
    LOGI("DelayedTaskWrapper: Sleeping for %ld seconds and %ld nanoseconds", ts.tv_sec, ts.tv_nsec);
    
    // 使用循环确保完整的延迟时间，处理被信号中断的情况
    struct timespec remaining;
    int sleepResult;
    while ((sleepResult = nanosleep(&ts, &remaining)) == -1 && errno == EINTR) {
        LOGI("DelayedTaskWrapper: nanosleep interrupted, remaining: %ld.%09ld", remaining.tv_sec, remaining.tv_nsec);
        ts = remaining;  // 继续剩余的时间
    }
    
    // 检查睡眠是否成功完成
    if (sleepResult == -1) {
        LOGE("DelayedTaskWrapper: nanosleep failed with errno %d", errno);
    } else {
        LOGI("DelayedTaskWrapper: Sleep completed successfully");
    }
    
    // 执行原始任务
    if (wrapper->originalFunc) {
        LOGI("DelayedTaskWrapper: Executing original task");
        wrapper->originalFunc(wrapper->originalArg);  // 调用原始任务函数
        LOGI("DelayedTaskWrapper: Original task completed");
    } else {
        LOGE("DelayedTaskWrapper: Original function is null");
    }
    
    // 清理包装器内存
    delete wrapper;
    LOGI("DelayedTaskWrapper: Cleanup completed");
}

void periodicTaskWrapper(void* arg) {
    PeriodicTaskWrapper* wrapper = static_cast<PeriodicTaskWrapper*>(arg);
    
    LOGI("PeriodicTaskWrapper: Starting periodic task with interval %d ms", wrapper->intervalMs);
    
    // 获取线程管理器指针
    zThread* threadManager = static_cast<zThread*>(wrapper->callbackUserData);
    
    // 检查线程管理器是否有效
    if (!threadManager) {
        LOGE("PeriodicTaskWrapper: Thread manager is null");
        delete wrapper;
        return;
    }
    
    int cycleCount = 0;
    while (threadManager->isThreadPoolRunning()) {
        cycleCount++;
        LOGI("PeriodicTaskWrapper: Cycle %d - checking if task is still active", cycleCount);
        
        // 检查任务是否仍然在活跃任务列表中
        if (!threadManager->isTaskActive(wrapper->taskId)) {
            LOGI("PeriodicTaskWrapper: Task %s was cancelled or completed, exiting", wrapper->taskId.c_str());
            break;
        }
        
        // 执行原始任务
        if (wrapper->originalFunc) {
            LOGI("PeriodicTaskWrapper: Executing original task in cycle %d", cycleCount);
            wrapper->originalFunc(wrapper->originalArg);
            LOGI("PeriodicTaskWrapper: Original task completed in cycle %d", cycleCount);
        } else {
            LOGE("PeriodicTaskWrapper: Original function is null");
            break;
        }
        
        // 等待间隔时间
        struct timespec ts;
        ts.tv_sec = wrapper->intervalMs / 1000;
        ts.tv_nsec = (wrapper->intervalMs % 1000) * 1000000;
        
        LOGI("PeriodicTaskWrapper: Sleeping for %ld seconds and %ld nanoseconds", ts.tv_sec, ts.tv_nsec);
        
        // 使用循环确保完整的延迟时间
        struct timespec remaining;
        int sleepResult;
        while ((sleepResult = nanosleep(&ts, &remaining)) == -1 && errno == EINTR) {
            LOGI("PeriodicTaskWrapper: nanosleep interrupted, remaining: %ld.%09ld", remaining.tv_sec, remaining.tv_nsec);
            ts = remaining;
        }
        
        if (sleepResult == -1) {
            LOGE("PeriodicTaskWrapper: nanosleep failed with errno %d", errno);
        } else {
            LOGI("PeriodicTaskWrapper: Sleep completed successfully");
        }
    }
    
    // 清理包装器
    delete wrapper;
    LOGI("PeriodicTaskWrapper: Cleanup completed after %d cycles", cycleCount);
}

// 高性能队列操作方法实现（使用读写锁）

// 安全地从队列获取下一个任务
bool zThread::getNextTask(Task& outTask) {
    return m_taskQueue.pop(outTask);  // 使用线程安全队列
}

// 安全地向队列添加任务
void zThread::addTaskToQueue(const Task& task) {
    m_taskQueue.push(task);  // 使用线程安全队列
    
    size_t queuedCount = m_taskQueue.size();
    size_t activeCount = m_activeTasks.size();
    
    LOGI("addTaskToQueue: Task '%s' added to queue - queued=%zu, active=%zu", 
         task.taskId.c_str(), queuedCount, activeCount);
}

// 安全地从活跃任务列表中移除任务
bool zThread::removeActiveTask(const string& taskId) {
    bool removed = m_activeTasks.remove(taskId);  // 使用线程安全映射
    
    if (removed) {
        size_t remainingActive = m_activeTasks.size();
        size_t queuedCount = m_taskQueue.size();
        
        LOGI("removeActiveTask: Task '%s' removed - remaining: active=%zu, queued=%zu", 
             taskId.c_str(), remainingActive, queuedCount);
        
        // 如果所有任务都完成了，记录特殊日志
        if (remainingActive == 0 && queuedCount == 0) {
            LOGI("removeActiveTask: All tasks completed! No more active or queued tasks");
        }
    } else {
        LOGW("removeActiveTask: Failed to remove task '%s' - task not found in active list", taskId.c_str());
    }
    
    return removed;
}

// 安全地向活跃任务列表添加任务
void zThread::addActiveTask(const Task& task) {
    m_activeTasks.set(task.taskId, task);  // 使用线程安全映射
    
    size_t activeCount = m_activeTasks.size();
    size_t queuedCount = m_taskQueue.size();
    
    LOGI("addActiveTask: Task '%s' added to active list - active=%zu, queued=%zu", 
         task.taskId.c_str(), activeCount, queuedCount);
}

// 设置任务完成回调函数
void zThread::setTaskCompletionCallback(std::function<void()> callback) {
    std::unique_lock<std::shared_mutex> lock(m_callbackMutex);
    m_taskCompletionCallback = callback;
    LOGI("setTaskCompletionCallback: Callback function set");
}

// 检查并触发完成回调
void zThread::checkAndTriggerCompletionCallback() {
    // 检查是否所有任务都完成
    size_t activeCount = m_activeTasks.size();
    size_t queuedCount = m_taskQueue.size();
    
    size_t totalPending = activeCount + queuedCount;
    
    LOGI("checkAndTriggerCompletionCallback: active=%zu, queued=%zu, total=%zu", 
         activeCount, queuedCount, totalPending);
    
    if (totalPending == 0) {
        // 所有任务都完成，触发回调
        std::shared_lock<std::shared_mutex> lock(m_callbackMutex);
        if (m_taskCompletionCallback) {
            LOGI("checkAndTriggerCompletionCallback: All tasks completed, triggering callback immediately");
            struct timeval callbackTime;
            gettimeofday(&callbackTime, nullptr);
            LOGI("checkAndTriggerCompletionCallback: Callback triggered at %ld.%06ld", 
                 callbackTime.tv_sec, callbackTime.tv_usec);
            
            m_taskCompletionCallback();
            
            struct timeval callbackEndTime;
            gettimeofday(&callbackEndTime, nullptr);
            long callbackDuration = (callbackEndTime.tv_sec - callbackTime.tv_sec) * 1000 + 
                                   (callbackEndTime.tv_usec - callbackTime.tv_usec) / 1000;
            LOGI("checkAndTriggerCompletionCallback: Callback completed in %ldms", callbackDuration);
        } else {
            LOGI("checkAndTriggerCompletionCallback: All tasks completed but no callback set");
        }
    }
}

// 工作线程管理方法实现

// 获取指定索引的工作线程
zChildThread* zThread::getWorkerThread(size_t index) {
    std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
    if (index < m_workerThreads.size()) {
        return m_workerThreads[index];
    }
    return nullptr;
}

// 获取最空闲的工作线程（状态为IDLE的线程）
zChildThread* zThread::getLeastBusyWorker() {
    std::shared_lock<std::shared_mutex> lock(m_workerThreadsMutex);
    if (m_workerThreads.empty()) {
        return nullptr;
    }
    
    // 优先选择IDLE状态的线程
    for (auto* worker : m_workerThreads) {
        if (worker->isRunning() && worker->getState() == WorkerState::IDLE) {
            return worker;
        }
    }
    
    // 如果没有IDLE线程，选择WAITING状态的线程
    for (auto* worker : m_workerThreads) {
        if (worker->isRunning() && worker->getState() == WorkerState::WAITING) {
            return worker;
        }
    }
    
    // 如果都没有，返回第一个运行中的线程
    for (auto* worker : m_workerThreads) {
        if (worker->isRunning()) {
            return worker;
        }
    }
    
    return nullptr;
}

// 回调函数实现

// 任务完成回调函数
void zThread::onTaskCompleted(const string& taskId, void* userData) {
    LOGI("onTaskCompleted: Task '%s' completed, calling removeActiveTask", taskId.c_str());
    
    zThread* threadManager = static_cast<zThread*>(userData);
    if (threadManager) {
        threadManager->removeActiveTask(taskId);
        LOGI("onTaskCompleted: Task '%s' processed successfully", taskId.c_str());
    } else {
        LOGE("onTaskCompleted: NULL thread manager for task '%s'", taskId.c_str());
    }
}

// 任务提供者回调函数（用于工作窃取）
bool zThread::provideTask(Task& outTask, void* userData) {
    zThread* threadManager = static_cast<zThread*>(userData);
    if (threadManager) {
        return threadManager->getNextTask(outTask);
    }
    return false;
}
