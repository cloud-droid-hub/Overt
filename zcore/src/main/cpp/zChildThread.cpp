//
// Created by Assistant on 2025/8/7.
// zChildThread 实现 - 最小轻量级工作线程类
//

#include "zChildThread.h"

// 构造函数
zChildThread::zChildThread(size_t index, const string& name)
    : m_threadId(nullptr)
    , m_threadName(name)
    , m_threadIndex(index)
    , m_taskMutex(nullptr)
    , m_taskCV(nullptr)
    , m_executeFunc(nullptr)
    , m_executeFuncArg(nullptr)
    , m_task(nullptr)
    , m_isTaskRunning(false)
    , m_isThreadRunning(false)
{
    // 初始化互斥锁和条件变量
    m_taskMutex = malloc(sizeof(pthread_mutex_t));
    m_taskCV = malloc(sizeof(pthread_cond_t));
    pthread_mutex_init((pthread_mutex_t*)m_taskMutex, nullptr);
    pthread_cond_init((pthread_cond_t*)m_taskCV, nullptr);
}

// 析构函数
zChildThread::~zChildThread() {
    // 确保线程已停止
    stop();
    
    // 销毁同步对象
    if (m_taskCV) {
        pthread_cond_destroy((pthread_cond_t*)m_taskCV);
        free(m_taskCV);
    }
    if (m_taskMutex) {
        pthread_mutex_destroy((pthread_mutex_t*)m_taskMutex);
        free(m_taskMutex);
    }
    
    LOGI("zChildThread[%zu] '%s' destroyed", m_threadIndex, m_threadName.c_str());
}

// 启动线程
bool zChildThread::start() {
    if (m_isThreadRunning) {
        LOGW("zChildThread[%zu] already running", m_threadIndex);
        return false;
    }
    
    m_isThreadRunning = true;  // 标记线程开始运行
    
    // 创建线程
    int result = pthread_create((pthread_t*)&m_threadId, nullptr, threadEntry, this);
    if (result != 0) {
        LOGE("Failed to create thread[%zu]: %d", m_threadIndex, result);
        m_isThreadRunning = false;
        return false;
    }
    
    LOGI("zChildThread[%zu] started with ID %lu", m_threadIndex, (unsigned long)m_threadId);
    return true;
}

// 停止线程
void zChildThread::stop() {
    if (!m_isThreadRunning) {
        return;
    }
    
    LOGI("Stopping zChildThread[%zu]...", m_threadIndex);
    m_isThreadRunning = false;
    
    // 通知等待的线程
    pthread_cond_signal((pthread_cond_t*)m_taskCV);
    
    // 等待线程结束
    if (m_threadId != nullptr) {
        join();
    }
}

// 等待线程结束
void zChildThread::join() {
    if (m_threadId != nullptr) {
        void* retval;
        int result = pthread_join(*(pthread_t*)&m_threadId, &retval);
        if (result == 0) {
            LOGI("zChildThread[%zu] joined successfully", m_threadIndex);
        } else {
            LOGE("Failed to join thread[%zu]: %d", m_threadIndex, result);
        }
        m_threadId = nullptr;
    }
}

bool zChildThread::setExecuteFunction(std::function<void()> callable){
    LOGI("zChildThread[%zu] setExecuteFunction called, isTaskRunning: %s", 
         m_threadIndex, m_isTaskRunning ? "true" : "false");
    
    // 使用互斥锁保护整个操作
    pthread_mutex_lock((pthread_mutex_t*)m_taskMutex);
    
    // 检查线程是否正在运行
    if (!m_isThreadRunning) {
        LOGW("zChildThread[%zu] thread is not running", m_threadIndex);
        pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
        return false;
    }

    // 检查是否正在执行任务
    if (m_isTaskRunning) {
        LOGW("zChildThread[%zu] task is already executing, cannot add new task", m_threadIndex);
        pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
        return false;
    }

    // 原子地设置任务和状态
    this->m_callable = callable;
    this->m_executeFunc = nullptr;
    this->m_executeFuncArg = nullptr;
    m_isTaskRunning = true; // 在锁保护下设置

    LOGI("zChildThread[%zu] task set and marked as running, callable valid: %s", 
         m_threadIndex, (this->m_callable ? "true" : "false"));

    // 唤醒线程执行任务
    pthread_cond_signal((pthread_cond_t*)m_taskCV);
    pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
    
    LOGI("zChildThread[%zu] awakened to execute new function", m_threadIndex);
    return true;
}

// 设置执行函数并唤醒线程
bool zChildThread::setExecuteFunction(void (*func)(void*), void* arg) {
    // 检查线程是否正在运行
    if (!m_isThreadRunning) {
        LOGW("zChildThread[%zu] thread is not running", m_threadIndex);
        return false;
    }
    
    // 检查是否正在执行任务
    if (m_isTaskRunning) {
        LOGW("zChildThread[%zu] task is already executing, cannot add new task", m_threadIndex);
        return false;
    }

    this->m_callable = nullptr;
    this->m_executeFunc = func;
    this->m_executeFuncArg = arg;
    LOGI("zChildThread[%zu] execute function set", m_threadIndex);
    
    // 立即唤醒线程执行任务
    pthread_mutex_lock((pthread_mutex_t*)m_taskMutex);
    pthread_cond_signal((pthread_cond_t*)m_taskCV);
    pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
    LOGI("zChildThread[%zu] awakened to execute new function", m_threadIndex);
    
    return true;
}

// 设置 zTask 对象并执行
bool zChildThread::setExecuteTask(zTask* task) {
    LOGI("zChildThread[%zu] setExecuteTask called with task '%s'", 
         m_threadIndex, task ? task->getTaskName().c_str() : "null");
    
    // 使用互斥锁保护整个操作
    pthread_mutex_lock((pthread_mutex_t*)m_taskMutex);
    
    // 检查线程是否正在运行
    if (!m_isThreadRunning) {
        LOGW("zChildThread[%zu] thread is not running", m_threadIndex);
        pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
        return false;
    }

    // 检查是否正在执行任务
    if (m_isTaskRunning) {
        LOGW("zChildThread[%zu] task is already executing, cannot add new task", m_threadIndex);
        pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
        return false;
    }

    // 检查任务是否有效
    if (!task || !task->isValid()) {
        LOGW("zChildThread[%zu] invalid task provided", m_threadIndex);
        pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
        return false;
    }

    // 原子地设置任务和状态
    this->m_task = task;
    this->m_callable = nullptr;
    this->m_executeFunc = nullptr;
    this->m_executeFuncArg = nullptr;
    m_isTaskRunning = true; // 在锁保护下设置

    LOGI("zChildThread[%zu] task '%s' (ID: %s) set and marked as running", 
         m_threadIndex, task->getTaskName().c_str(), task->getTaskId().c_str());

    // 唤醒线程执行任务
    pthread_cond_signal((pthread_cond_t*)m_taskCV);
    pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
    
    LOGI("zChildThread[%zu] awakened to execute task '%s'", m_threadIndex, task->getTaskName().c_str());
    return true;
}

// 设置执行函数并唤醒线程
void zChildThread::setTaskCompletionCallback(void (*taskCompletionCallback)(string taskId, void* userData)) {
    this->m_taskCompletionCallback = taskCompletionCallback;
    this->m_taskCompletionCallbackFunc = nullptr; // 清除 std::function 版本
}

// std::function 版本的重载函数
void zChildThread::setTaskCompletionCallback(std::function<void(string, void*)> callback) {
    this->m_taskCompletionCallbackFunc = callback;
    this->m_taskCompletionCallback = nullptr; // 清除函数指针版本
}

// 静态线程入口函数
void* zChildThread::threadEntry(void* arg) {
    zChildThread* thread = static_cast<zChildThread*>(arg);
    return thread->threadMain();
}

// 线程主函数 - 简单的循环等待
void* zChildThread::threadMain() {
    LOGI("zChildThread[%zu] main loop started", m_threadIndex);
    
    while (m_isThreadRunning) {
        // 等待外部线程的信号
        pthread_mutex_lock((pthread_mutex_t*)m_taskMutex);
        
        // 如果没有任务且线程仍在运行，等待条件变量信号
        while (m_isThreadRunning && !m_isTaskRunning) {
            // 等待条件变量信号或超时
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1; // 1秒超时
            
            int result = pthread_cond_timedwait((pthread_cond_t*)m_taskCV, (pthread_mutex_t*)m_taskMutex, &ts);
            
            if (result == 0) {
                // 收到信号，检查是否有任务
                if (m_isTaskRunning) {
                    LOGI("zChildThread[%zu] received signal and has task, processing...", m_threadIndex);
                    break;
                }
            } else if (result == ETIMEDOUT) {
                // 超时，检查线程是否应该继续运行
                if (!m_isThreadRunning) {
                    LOGI("zChildThread[%zu] thread should stop, exiting wait loop", m_threadIndex);
                    break;
                }
                LOGI("zChildThread[%zu] timeout, continuing...", m_threadIndex);
                continue;
            } else {
                // 其他错误
                LOGE("zChildThread[%zu] cond wait error: %d", m_threadIndex, result);
                break;
            }
        }
        
        pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
        
        // 执行设置的任务函数
        if (m_isThreadRunning && m_task) {
            LOGI("zChildThread[%zu] executing zTask '%s' (ID: %s)...", 
                 m_threadIndex, m_task->getTaskName().c_str(), m_task->getTaskId().c_str());
            
            // 执行 zTask 对象
            m_task->execute();
            
            // 保存任务ID用于回调
            string taskId = m_task->getTaskId();
            
            // 清理任务对象
            delete m_task;
            m_task = nullptr;
            m_isTaskRunning = false;

            if(m_taskCompletionCallback){
                m_taskCompletionCallback(taskId, nullptr);
            } else if(m_taskCompletionCallbackFunc) {
                m_taskCompletionCallbackFunc(taskId, nullptr);
            }

            LOGI("zChildThread[%zu] zTask completed", m_threadIndex);
        } else if (m_isThreadRunning && m_executeFunc) {
            LOGI("zChildThread[%zu] executing task m_executeFunc function...", m_threadIndex);
            // m_isTaskRunning 已经在 setExecuteFunction 中设置了，这里不需要重复设置
            m_executeFunc(m_executeFuncArg);
            m_isTaskRunning = false;

            if(m_taskCompletionCallback){
                m_taskCompletionCallback("function_task", nullptr);
            } else if(m_taskCompletionCallbackFunc) {
                m_taskCompletionCallbackFunc("function_task", nullptr);
            }

            LOGI("zChildThread[%zu] task function completed", m_threadIndex);
        } else if (m_isThreadRunning && m_callable) {
            LOGI("zChildThread[%zu] executing task m_callable function...", m_threadIndex);
            // m_isTaskRunning 已经在 setExecuteFunction 中设置了，这里不需要重复设置
            try {
                m_callable();
                LOGI("zChildThread[%zu] task function executed successfully", m_threadIndex);
            } catch (...) {
                LOGE("zChildThread[%zu] task function execution failed with exception", m_threadIndex);
            }
            m_isTaskRunning = false;
            if(m_taskCompletionCallback){
                m_taskCompletionCallback("callable_task", nullptr);
            } else if(m_taskCompletionCallbackFunc) {
                m_taskCompletionCallbackFunc("callable_task", nullptr);
            }
            LOGI("zChildThread[%zu] task function completed", m_threadIndex);
        } else if (m_isThreadRunning && m_isTaskRunning) {
            // 如果标记为正在执行任务，但是没有可执行的任务，这是一个错误状态
            LOGW("zChildThread[%zu] isTaskRunning is true but no task available", m_threadIndex);
            m_isTaskRunning = false;
        } else if (m_isThreadRunning) {
            // 如果没有设置执行函数，执行默认任务
            LOGI("zChildThread[%zu] executing default task...", m_threadIndex);
            usleep(100000); // 100ms
        }
    }
    
    LOGI("zChildThread[%zu] main loop ended", m_threadIndex);
    return nullptr;
}

// 设置线程名称
zChildThread* zChildThread::setName(const string& name) {
    m_threadName = name;
    return this;
}

// 获取线程名称
string zChildThread::getName() const {
    return m_threadName;
}

// 检查线程是否正在运行
bool zChildThread::isThreadRunning() const {
    return m_isThreadRunning;
}

// 检查是否正在执行任务
bool zChildThread::isTaskRunning() const {
    // 使用锁保护状态检查，确保原子性
    pthread_mutex_lock((pthread_mutex_t*)m_taskMutex);
    bool result = m_isTaskRunning;
    pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
    return result;
}