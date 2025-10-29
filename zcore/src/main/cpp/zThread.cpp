//
// Created by Assistant on 2025/8/7.
// zThread 实现 - 最小轻量级工作线程类
//

#include "zThread.h"

/**
 * 构造函数 - 创建线程实例
 * 
 * 功能说明：
 * 1. 初始化线程基本属性（ID、名称、索引）
 * 2. 创建线程同步对象（互斥锁、条件变量）
 * 3. 初始化执行相关成员变量
 * 4. 设置初始状态为未运行
 * 
 * 设计特点：
 * - 使用RAII管理同步对象，自动清理资源
 * - 采用智能指针避免手动内存管理
 * - 提供详细的初始化日志
 * 
 * @param index 线程索引，用于标识和日志记录
 * @param name 线程名称，便于调试和监控
 */
zThread::zThread(size_t index, const string& name)
    : m_threadId(nullptr)                                    // 线程ID，由pthread_create设置
    , m_threadName(name)                                     // 线程名称
    , m_threadIndex(index)                                   // 线程索引
    , m_taskMutex(std::make_unique<std::mutex>())           // 任务互斥锁，RAII管理
    , m_taskCV(std::make_unique<std::condition_variable>()) // 任务条件变量，RAII管理
    , m_executeFunc(nullptr)                                 // 执行函数指针
    , m_executeFuncArg(nullptr)                              // 执行函数参数
    , m_task(nullptr)                                        // 任务对象指针
    , m_isTaskRunning(false)                                 // 任务运行状态
    , m_isThreadRunning(false)                               // 线程运行状态
{
    // std::mutex 和 std::condition_variable 会自动初始化
    LOGI("zThread[%zu] '%s' created with RAII mutex and condition variable", m_threadIndex, m_threadName.c_str());
}

/**
 * 析构函数 - 清理线程资源
 * 
 * 功能说明：
 * 1. 确保线程已完全停止
 * 2. 等待线程结束并清理资源
 * 3. 记录销毁日志
 * 
 * 资源管理：
 * - 自动调用stop()确保线程停止
 * - std::unique_ptr自动清理同步对象
 * - 不需要手动清理pthread资源
 */
zThread::~zThread() {
    // 确保线程已停止
    stop();
    
    // std::unique_ptr 会自动清理资源
    LOGI("zThread[%zu] '%s' destroyed", m_threadIndex, m_threadName.c_str());
}

/**
 * 启动线程
 * 
 * 功能说明：
 * 1. 检查线程是否已经在运行
 * 2. 创建pthread线程
 * 3. 设置线程入口函数
 * 4. 更新线程运行状态
 * 
 * 线程创建：
 * - 使用pthread_create创建POSIX线程
 * - 线程入口为threadEntry静态函数
 * - 传递this指针作为线程参数
 * 
 * 错误处理：
 * - 检查pthread_create返回值
 * - 失败时重置运行状态
 * - 记录详细的错误日志
 * 
 * @return true表示启动成功，false表示启动失败
 */
bool zThread::start() {
    // 检查线程是否已经在运行
    if (m_isThreadRunning) {
        LOGW("zThread[%zu] already running", m_threadIndex);
        return false;
    }
    
    m_isThreadRunning = true;  // 标记线程开始运行
    
    // 创建pthread线程
    int result = pthread_create((pthread_t*)&m_threadId, nullptr, threadEntry, this);
    if (result != 0) {
        LOGE("Failed to create thread[%zu]: %d", m_threadIndex, result);
        m_isThreadRunning = false;
        return false;
    }
    
    LOGI("zThread[%zu] started with ID %lu", m_threadIndex, (unsigned long)m_threadId);
    return true;
}

// 停止线程
void zThread::stop() {
    if (!m_isThreadRunning) {
        return;
    }
    
    LOGI("Stopping zThread[%zu]...", m_threadIndex);
    m_isThreadRunning = false;
    
    // 通知等待的线程
    m_taskCV->notify_one();
    
    // 等待线程结束
    if (m_threadId != nullptr) {
        join();
    }
}

// 等待线程结束
void zThread::join() {
    if (m_threadId != nullptr) {
        void* retval;
        int result = pthread_join(*(pthread_t*)&m_threadId, &retval);
        if (result == 0) {
            LOGI("zThread[%zu] joined successfully", m_threadIndex);
        } else {
            LOGE("Failed to join thread[%zu]: %d", m_threadIndex, result);
        }
        m_threadId = nullptr;
    }
}

bool zThread::setExecuteFunction(std::function<void()> callable){
    LOGI("zThread[%zu] setExecuteFunction called, isTaskRunning: %s",
         m_threadIndex, m_isTaskRunning ? "true" : "false");
    
    // 使用 RAII 锁保护整个操作
    {
        std::lock_guard<std::mutex> lock(*m_taskMutex);
        
        // 检查线程是否正在运行
        if (!m_isThreadRunning) {
            LOGW("zThread[%zu] thread is not running", m_threadIndex);
            return false;
        }

        // 检查是否正在执行任务
        if (m_isTaskRunning) {
            LOGW("zThread[%zu] task is already executing, cannot add new task", m_threadIndex);
            return false;
        }

        // 原子地设置任务和状态
        this->m_callable = callable;
        this->m_executeFunc = nullptr;
        this->m_executeFuncArg = nullptr;
        m_isTaskRunning = true; // 在锁保护下设置

        LOGI("zThread[%zu] task set and marked as running, callable valid: %s",
             m_threadIndex, (this->m_callable ? "true" : "false"));
    }

    // 唤醒线程执行任务
    m_taskCV->notify_one();
    
    LOGI("zThread[%zu] awakened to execute new function", m_threadIndex);
    return true;
}

// 设置执行函数并唤醒线程
bool zThread::setExecuteFunction(void (*func)(void*), void* arg) {
    // 检查线程是否正在运行
    if (!m_isThreadRunning) {
        LOGW("zThread[%zu] thread is not running", m_threadIndex);
        return false;
    }
    
    // 检查是否正在执行任务
    if (m_isTaskRunning) {
        LOGW("zThread[%zu] task is already executing, cannot add new task", m_threadIndex);
        return false;
    }

    this->m_callable = nullptr;
    this->m_executeFunc = func;
    this->m_executeFuncArg = arg;
    LOGI("zThread[%zu] execute function set", m_threadIndex);
    
    // 立即唤醒线程执行任务
    m_taskCV->notify_one();
    LOGI("zThread[%zu] awakened to execute new function", m_threadIndex);
    
    return true;
}

// 设置 zTask 对象并执行
bool zThread::setExecuteTask(zTask* task) {
    LOGI("zThread[%zu] setExecuteTask called with task '%s'",
         m_threadIndex, task ? task->getTaskName().c_str() : "null");
    
    // 使用 RAII 锁保护整个操作
    {
        std::lock_guard<std::mutex> lock(*m_taskMutex);
        
        // 检查线程是否正在运行
        if (!m_isThreadRunning) {
            LOGW("zThread[%zu] thread is not running", m_threadIndex);
            return false;
        }

        // 检查是否正在执行任务
        if (m_isTaskRunning) {
            LOGW("zThread[%zu] task is already executing, cannot add new task", m_threadIndex);
            return false;
        }

        // 检查任务是否有效
        if (!task || !task->isValid()) {
            LOGW("zThread[%zu] invalid task provided", m_threadIndex);
            return false;
        }

        // 原子地设置任务和状态
        this->m_task = task;
        this->m_callable = nullptr;
        this->m_executeFunc = nullptr;
        this->m_executeFuncArg = nullptr;
        m_isTaskRunning = true; // 在锁保护下设置

        LOGI("zThread[%zu] task '%s' (ID: %s) set and marked as running",
             m_threadIndex, task->getTaskName().c_str(), task->getTaskId().c_str());
    }

    // 唤醒线程执行任务
    m_taskCV->notify_one();
    
    LOGI("zThread[%zu] awakened to execute task '%s'", m_threadIndex, task->getTaskName().c_str());
    return true;
}

// 设置执行函数并唤醒线程
void zThread::setTaskCompletionCallback(void (*taskCompletionCallback)(string taskId, void* userData)) {
    this->m_taskCompletionCallback = taskCompletionCallback;
    this->m_taskCompletionCallbackFunc = nullptr; // 清除 std::function 版本
}

// std::function 版本的重载函数
void zThread::setTaskCompletionCallback(std::function<void(string, void*)> callback) {
    this->m_taskCompletionCallbackFunc = callback;
    this->m_taskCompletionCallback = nullptr; // 清除函数指针版本
}

// 静态线程入口函数
void* zThread::threadEntry(void* arg) {
    zThread* thread = static_cast<zThread*>(arg);
    return thread->threadMain();
}

// 线程主函数 - 简单的循环等待
void* zThread::threadMain() {
    LOGI("zThread[%zu] main loop started", m_threadIndex);
    
    while (m_isThreadRunning) {
        // 等待外部线程的信号
        {
            std::unique_lock<std::mutex> lock(*m_taskMutex);
            
            // 如果没有任务且线程仍在运行，等待条件变量信号
            m_taskCV->wait_for(lock, std::chrono::seconds(1), [this]() {
                return m_isTaskRunning || !m_isThreadRunning;
            });
            
            // 检查是否有任务或线程应该停止
            if (m_isTaskRunning) {
                LOGI("zThread[%zu] received signal and has task, processing...", m_threadIndex);
            } else if (!m_isThreadRunning) {
                LOGI("zThread[%zu] thread should stop, exiting wait loop", m_threadIndex);
                break;
            } else {
                LOGI("zThread[%zu] timeout, continuing...", m_threadIndex);
                continue;
            }
        }
        
        // 执行设置的任务函数
        if (m_isThreadRunning && m_task) {
            LOGI("zThread[%zu] executing zTask '%s' (ID: %s)...",
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

            LOGI("zThread[%zu] zTask completed", m_threadIndex);
        } else if (m_isThreadRunning && m_executeFunc) {
            LOGI("zThread[%zu] executing task m_executeFunc function...", m_threadIndex);
            // m_isTaskRunning 已经在 setExecuteFunction 中设置了，这里不需要重复设置
            m_executeFunc(m_executeFuncArg);
            m_isTaskRunning = false;

            if(m_taskCompletionCallback){
                m_taskCompletionCallback("function_task", nullptr);
            } else if(m_taskCompletionCallbackFunc) {
                m_taskCompletionCallbackFunc("function_task", nullptr);
            }

            LOGI("zThread[%zu] task function completed", m_threadIndex);
        } else if (m_isThreadRunning && m_callable) {
            LOGI("zThread[%zu] executing task m_callable function...", m_threadIndex);
            // m_isTaskRunning 已经在 setExecuteFunction 中设置了，这里不需要重复设置
            try {
                m_callable();
                LOGI("zThread[%zu] task function executed successfully", m_threadIndex);
            } catch (...) {
                LOGE("zThread[%zu] task function execution failed with exception", m_threadIndex);
            }
            m_isTaskRunning = false;
            if(m_taskCompletionCallback){
                m_taskCompletionCallback("callable_task", nullptr);
            } else if(m_taskCompletionCallbackFunc) {
                m_taskCompletionCallbackFunc("callable_task", nullptr);
            }
            LOGI("zThread[%zu] task function completed", m_threadIndex);
        } else if (m_isThreadRunning && m_isTaskRunning) {
            // 如果标记为正在执行任务，但是没有可执行的任务，这是一个错误状态
            LOGW("zThread[%zu] isTaskRunning is true but no task available", m_threadIndex);
            m_isTaskRunning = false;
        } else if (m_isThreadRunning) {
            // 如果没有设置执行函数，执行默认任务
            LOGI("zThread[%zu] executing default task...", m_threadIndex);
            usleep(100000); // 100ms
        }
    }
    
    LOGI("zThread[%zu] main loop ended", m_threadIndex);
    return nullptr;
}

// 设置线程名称
zThread* zThread::setName(const string& name) {
    m_threadName = name;
    return this;
}

// 获取线程名称
string zThread::getName() const {
    return m_threadName;
}

// 检查线程是否正在运行
bool zThread::isThreadRunning() const {
    return m_isThreadRunning;
}

// 检查是否正在执行任务
bool zThread::isTaskRunning() const {
    // 使用 RAII 锁保护状态检查，确保原子性
    std::lock_guard<std::mutex> lock(*m_taskMutex);
    return m_isTaskRunning;
}