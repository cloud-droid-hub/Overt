//
// Created by lxz on 2025/8/7.
//

// 包含线程管理类的头文件
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
    
    // 初始化消息队列的互斥锁和条件变量 - 用于线程间通信
    pthread_mutex_init(&m_messageQueueMutex, nullptr);
    pthread_cond_init(&m_messageQueueCV, nullptr);
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
    }
    return instance;
}

// 析构函数实现 - 清理所有资源
zThread::~zThread() {
    // 停止线程池并等待所有任务完成
    stopThreadPool(true);
    
    // 清理所有工作线程对象
    for (auto* worker : m_workerThreads) {
        delete worker;
    }
    m_workerThreads.clear();
    
    // 销毁任务队列条件变量（shared_mutex 会自动析构）
    pthread_cond_destroy(&m_taskQueueCV);
    
    // 销毁消息队列相关的同步对象
    pthread_mutex_destroy(&m_messageQueueMutex);
    pthread_cond_destroy(&m_messageQueueCV);
    
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
        LOGW("Thread pool is already running");
        return false;
    }
    
    // 计算实际创建的线程数，不超过最大线程数限制
    size_t actualThreadCount = (threadCount < m_maxThreads) ? threadCount : m_maxThreads;
    m_running = true;  // 设置运行标志
    
    // 创建 zChildThread 对象
    for (size_t i = 0; i < actualThreadCount; ++i) {
        string threadName = m_name + "_worker_" + std::to_string(i);
        
        // 创建工作线程对象（使用回调函数实现解耦）
        zChildThread* worker = new zChildThread(i, threadName, 
                                               &zThread::onTaskCompleted,  // 任务完成回调
                                               &zThread::provideTask,      // 任务提供者回调
                                               this);                      // 用户数据（this指针）
        
        // 启动线程
        if (!worker->start()) {
            LOGE("Failed to start worker thread %zu", i);
            delete worker;
            return false;
        }
        
        // 保存工作线程对象
        m_workerThreads.push_back(worker);
    }
    
    LOGI("Thread pool started with %zu threads", actualThreadCount);
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
    for (auto* worker : m_workerThreads) {
        worker->stop(waitForCompletion);
    }
    
    // 清空线程列表（对象在析构函数中删除）
    m_workerThreads.clear();
    
    LOGI("Thread pool stopped");
}

// 检查线程池是否正在运行
bool zThread::isThreadPoolRunning() const {
    return m_running;
}

// 获取活跃线程数量
size_t zThread::getActiveThreadCount() const {
    // 返回工作线程的数量
    return m_workerThreads.size();
}

// 获取正在执行任务的线程数量
size_t zThread::getExecutingTaskCount() {
    // 统计状态为RUNNING的线程数
    size_t executingCount = 0;
    for (const auto* worker : m_workerThreads) {
        if (worker->getState() == WorkerState::RUNNING) {
            executingCount++;
        }
    }
    return executingCount;
}

// 获取活跃任务数量
size_t zThread::getActiveTaskCount() {
    // 使用共享锁读取活跃任务数量，提升并发性能
    std::shared_lock<std::shared_mutex> lock(m_activeTasksMutex);
    return m_activeTasks.size();
}

// 获取待处理任务总数
size_t zThread::getPendingTaskCount() {
    // 使用新的高性能方法读取任务数量
    size_t queueCount = getQueuedTaskCount();
    size_t activeTasksCount = getActiveTaskCount();
    
    // 返回总的待处理任务数：队列中的 + 活跃任务中的
    return queueCount + activeTasksCount;
}

// 获取队列中的任务数量
size_t zThread::getQueuedTaskCount() {
    // 使用共享锁读取任务队列大小，提升并发性能
    std::shared_lock<std::shared_mutex> lock(m_taskQueueMutex);
    return m_taskQueue.size();
}

// 检查任务是否活跃
bool zThread::isTaskActive(const string& taskId) {
    // 使用共享锁检查任务是否活跃，提升并发性能
    std::shared_lock<std::shared_mutex> lock(m_activeTasksMutex);
    return m_activeTasks.find(taskId) != m_activeTasks.end();
}

// 任务管理实现 - 提交普通任务
bool zThread::submitTask(void (*func)(void*), void* arg, TaskPriority priority, const string& taskId) {
    // 检查线程池是否正在运行
    if (!m_running) {
        LOGW("Thread pool is not running");
        return false;
    }
    
    // 生成任务ID，如果未提供则自动生成
    string actualTaskId = taskId.empty() ? generateTaskId() : taskId;
    
    // 创建任务对象
    Task task;
    task.func = func;           // 任务函数指针
    task.arg = arg;             // 任务参数
    task.priority = priority;    // 任务优先级
    task.taskId = actualTaskId; // 任务ID
    
    // 添加到活跃任务列表用于跟踪
    addActiveTask(task);
    
    // 智能任务分发：找到最空闲的工作线程
    zChildThread* targetWorker = getLeastBusyWorker();
    if (targetWorker) {
        // 直接分发给最空闲的线程
        targetWorker->addTask(task);
        LOGD("Task '%s' assigned to worker %zu", actualTaskId.c_str(), targetWorker->getThreadIndex());
    } else {
        // 如果没有可用的工作线程，添加到全局队列
        addTaskToQueue(task);
        LOGD("Task '%s' added to global queue", actualTaskId.c_str());
    }
    
    // 记录任务提交日志
    logThreadEvent("Task submitted: " + actualTaskId, pthread_self());
    return true;
}

// 提交延迟任务 - 在指定延迟后执行
bool zThread::submitDelayedTask(void (*func)(void*), void* arg, int delayMs, const string& taskId) {
    // 生成任务ID，如果未提供则自动生成
    string actualTaskId = taskId.empty() ? generateTaskId() : taskId;
    
    // 创建延迟任务包装器，包含原始函数、参数和延迟时间
    DelayedTaskWrapper* wrapper = new DelayedTaskWrapper(func, arg, delayMs);
    
    // 将包装器作为普通任务提交，使用生成的任务ID
    return submitTask(delayedTaskWrapper, wrapper, TaskPriority::NORMAL, actualTaskId);
}

// 提交周期性任务 - 按指定间隔重复执行
bool zThread::submitPeriodicTask(void (*func)(void*), void* arg, int intervalMs, const string& taskId) {
    // 生成任务ID，如果未提供则自动生成
    string actualTaskId = taskId.empty() ? generateTaskId() : taskId;
    
    // 创建周期性任务包装器，包含原始函数、参数、间隔时间和完成回调
    PeriodicTaskWrapper* wrapper = new PeriodicTaskWrapper(func, arg, intervalMs, actualTaskId, 
                                                          &zThread::onTaskCompleted, this);
    
    // 将包装器作为普通任务提交
    return submitTask(periodicTaskWrapper, wrapper, TaskPriority::NORMAL, actualTaskId);
}

// 取消任务 - 从活跃任务列表或队列中移除指定任务
bool zThread::cancelTask(const string& taskId) {
    // 首先尝试从活跃任务列表中移除
    if (removeActiveTask(taskId)) {
        logThreadEvent("Task cancelled: " + taskId, pthread_self());
        return true;
    }
    
    // 如果活跃任务列表中没有，检查任务队列中是否有该任务
    std::unique_lock<std::shared_mutex> lock(m_taskQueueMutex);
    for (auto it = m_taskQueue.begin(); it != m_taskQueue.end(); ++it) {
        if (it->taskId == taskId) {
            m_taskQueue.erase(it);  // 从队列中移除
            logThreadEvent("Task cancelled from queue: " + taskId, pthread_self());
            return true;
        }
    }
    
    return false;  // 任务不存在，取消失败
}

bool zThread::waitForAllTasks(int timeoutMs) {
    struct timeval startTime;
    gettimeofday(&startTime, nullptr);
    
    LOGI("waitForAllTasks: Starting wait with timeout %d ms", timeoutMs);
    
    while (true) {
        size_t pendingCount = getPendingTaskCount();
        size_t queuedCount = getQueuedTaskCount();
        size_t activeCount = getActiveTaskCount();
        size_t executingCount = getExecutingTaskCount();
        
        // LOGI("waitForAllTasks: pending=%zu, queued=%zu, active=%zu, executing=%zu",
        //     pendingCount, queuedCount, activeCount, executingCount);
        
        if (pendingCount == 0 && executingCount == 0) {
            LOGI("waitForAllTasks: All tasks completed successfully");
            return true;
        }
        
        if (timeoutMs > 0) {
            struct timeval currentTime;
            gettimeofday(&currentTime, nullptr);
            long elapsed = (currentTime.tv_sec - startTime.tv_sec) * 1000 + 
                          (currentTime.tv_usec - startTime.tv_usec) / 1000;
            if (elapsed >= timeoutMs) {
                LOGW("waitForAllTasks: Timeout reached after %ld ms", elapsed);
                
                // 打印未完成的任务信息
                {
                    std::shared_lock<std::shared_mutex> lock(m_activeTasksMutex);
                    LOGI("waitForAllTasks: Active tasks that didn't complete:");
                    for (const auto& pair : m_activeTasks) {
                        LOGI("waitForAllTasks: - Task ID: %s", pair.first.c_str());
                    }
                }
                
                {
                    std::shared_lock<std::shared_mutex> lock(m_taskQueueMutex);
                    LOGI("waitForAllTasks: Queued tasks that didn't complete:");
                    for (const auto& task : m_taskQueue) {
                        LOGI("waitForAllTasks: - Task ID: %s", task.taskId.c_str());
                    }
                }
                
                return false;
            }
        }
        
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 10 * 1000000; // 10ms
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
    pthread_mutex_lock(&m_messageQueueMutex);
    
    if (timeoutMs > 0) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += timeoutMs * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += ts.tv_nsec / 1000000000;
            ts.tv_nsec %= 1000000000;
        }
        
        while (m_messageQueue.empty() && m_running) {
            if (pthread_cond_timedwait(&m_messageQueueCV, &m_messageQueueMutex, &ts) != 0) {
                pthread_mutex_unlock(&m_messageQueueMutex);
                return "";
            }
        }
    } else {
        while (m_messageQueue.empty() && m_running) {
            pthread_cond_wait(&m_messageQueueCV, &m_messageQueueMutex);
        }
    }
    
    if (!m_messageQueue.empty()) {
        string message = m_messageQueue.front();
        m_messageQueue.erase(m_messageQueue.begin());
        pthread_mutex_unlock(&m_messageQueueMutex);
        return message;
    }
    
    pthread_mutex_unlock(&m_messageQueueMutex);
    return "";
}

void zThread::broadcastMessage(const string& message) {
    pthread_mutex_lock(&m_messageQueueMutex);
    m_messageQueue.push_back(message);
    pthread_mutex_unlock(&m_messageQueueMutex);
    pthread_cond_broadcast(&m_messageQueueCV);
}

// 线程监控实现
vector<ThreadInfo> zThread::getThreadInfo() const {
    vector<ThreadInfo> result;
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
    return result;
}

ThreadState zThread::getThreadState(pthread_t threadId) const {
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
    return "Thread_" + std::to_string((unsigned long)tid);
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
    LOGD("updateThreadState called for thread %zu with state %d (now managed by zChildThread)", 
         threadIndex, static_cast<int>(state));
}

void zThread::cleanupTerminatedThreads() {
    // 清理已终止的工作线程对象
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
    return "task_" + std::to_string(pthread_self()) + "_" + std::to_string(__sync_fetch_and_add(&counter, 1));
}

void zThread::logThreadEvent(const string& event, pthread_t threadId) const {
    string message = "[" + m_name + "] ";
    if (threadId != 0) {
        message += "Thread " + std::to_string((unsigned long)threadId) + ": ";
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
    std::unique_lock<std::shared_mutex> lock(m_taskQueueMutex);
    if (m_taskQueue.empty()) {
        return false;
    }
    outTask = m_taskQueue.front();
    m_taskQueue.erase(m_taskQueue.begin());
    return true;
}

// 安全地向队列添加任务
void zThread::addTaskToQueue(const Task& task) {
    std::unique_lock<std::shared_mutex> lock(m_taskQueueMutex);
    m_taskQueue.push_back(task);
}

// 安全地从活跃任务列表中移除任务
bool zThread::removeActiveTask(const string& taskId) {
    std::unique_lock<std::shared_mutex> lock(m_activeTasksMutex);
    auto it = m_activeTasks.find(taskId);
    if (it != m_activeTasks.end()) {
        m_activeTasks.erase(it);
        LOGI("Task %s removed from active tasks", taskId.c_str());
        return true;
    }
    return false;
}

// 安全地向活跃任务列表添加任务
void zThread::addActiveTask(const Task& task) {
    std::unique_lock<std::shared_mutex> lock(m_activeTasksMutex);
    m_activeTasks[task.taskId] = task;
}

// 工作线程管理方法实现

// 获取指定索引的工作线程
zChildThread* zThread::getWorkerThread(size_t index) {
    if (index < m_workerThreads.size()) {
        return m_workerThreads[index];
    }
    return nullptr;
}

// 获取最空闲的工作线程（任务队列最小的线程）
zChildThread* zThread::getLeastBusyWorker() {
    if (m_workerThreads.empty()) {
        return nullptr;
    }
    
    zChildThread* leastBusy = nullptr;
    size_t minQueueSize = SIZE_MAX;
    
    for (auto* worker : m_workerThreads) {
        if (worker->isRunning()) {
            size_t queueSize = worker->getQueueSize();
            if (queueSize < minQueueSize) {
                minQueueSize = queueSize;
                leastBusy = worker;
            }
        }
    }
    
    return leastBusy;
}

// 回调函数实现

// 任务完成回调函数
void zThread::onTaskCompleted(const string& taskId, void* userData) {
    zThread* threadManager = static_cast<zThread*>(userData);
    if (threadManager) {
        threadManager->removeActiveTask(taskId);
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
