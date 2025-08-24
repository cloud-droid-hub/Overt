//
// Created by lxz on 2025/8/21.
//


#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zStdUtil.h"
#include <mutex>

#include "zThreadPool.h"
#include "zThread.h"

// 静态单例实例指针 - 用于实现单例模式
zThreadPool* zThreadPool::instance = nullptr;

// 静态任务队列互斥锁 - 使用 std::mutex（自动初始化）
std::mutex zThreadPool::m_taskQueueMutex;

/**
 * zThread构造函数
 * 初始化线程管理器，设置默认参数
 */
zThreadPool::zThreadPool() : m_name("zThreadPool"), m_maxThreads(8), m_running(false) {
    LOGI("zThread: Constructor - Thread pool manager initialized");
}

/**
 * 获取zThread单例实例
 * 采用线程安全的懒加载模式，首次调用时创建实例
 * @return zThread单例指针
 */
zThreadPool* zThreadPool::getInstance() {
    // 使用 std::call_once 确保线程安全的单例初始化
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        try {
            instance = new zThreadPool();
            // 启动线程池
            bool success = instance->startThreadPool(16);
            if (success) {
                LOGI("zThreadPool: Created singleton instance and started thread pool");
            } else {
                LOGE("zThreadPool: Failed to start thread pool after creating instance");
                // 如果启动失败，清理实例
                delete instance;
                instance = nullptr;
            }
        } catch (const std::exception& e) {
            LOGE("zThreadPool: Failed to create singleton instance: %s", e.what());
        } catch (...) {
            LOGE("zThreadPool: Failed to create singleton instance with unknown error");
        }
    });
    
    return instance;
}

void zThreadPool::taskCompletionCallback(string taskName, void* data){
    LOGI("zThread: taskCompletionCallback is call");
    tryRunTask();
}

// 线程池管理实现 - 启动线程池
bool zThreadPool::startThreadPool(size_t threadCount) {
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

    // 创建 zThread 对象
    for (size_t i = 0; i < actualThreadCount; ++i) {
        string threadName = m_name + "_worker_" + to_string(i);

        LOGI("startThreadPool: Creating worker thread %zu with name '%s'", i, threadName.c_str());

        // 创建工作线程对象（使用回调函数实现解耦）
        zThread* worker = new zThread(i, threadName);

        // 使用 std::function 版本的重载函数，可以直接传递成员函数
        worker->setTaskCompletionCallback([this](string taskId, void* userData) {
            this->taskCompletionCallback(taskId, userData);
        });

        // 启动线程
        if (!worker->start()) {
            LOGE("startThreadPool: Failed to start worker thread %zu", i);
            delete worker;
            m_running = false;
            return false;
        }

        LOGI("startThreadPool: Worker thread %zu started successfully", i);

        m_workerThreads.push_back(worker);

    }

    LOGI("startThreadPool: Thread pool started successfully with %zu worker threads", actualThreadCount);
    return true;
}

/**
 * zThread析构函数
 * 清理资源，确保线程池正确关闭
 */
zThreadPool::~zThreadPool() {
    LOGI("zThread: Destructor called");
    
    // 停止线程池
    if (m_running) {
        // 清理工作线程
        for (auto* worker : m_workerThreads) {
            if (worker) {
                worker->stop();
                delete worker;
            }
        }
        m_workerThreads.clear();
        m_running = false;
    }
    
    // std::mutex 会自动销毁，无需手动清理
    LOGI("zThread: Task queue mutex will be automatically destroyed");
    
    LOGI("zThread: Destructor completed");
}


void zThreadPool::tryRunTask(){
    LOGI("zThread: tryRunTask called, queue size: %zu, worker threads: %zu", 
         m_tasks.size(), m_workerThreads.size());

    // 使用专门的 tryRunTask 互斥锁，避免与任务队列锁冲突
    static std::mutex try_run_task_mutex;
    
    std::lock_guard<std::mutex> lock(try_run_task_mutex);
    LOGI("zThread: tryRunTask acquired try_run_task lock");
    
    if(this->m_tasks.empty()){
        LOGI("zThread: tryRunTask - No tasks in queue");
        return;
    }
    
    if(this->m_workerThreads.empty()){
        LOGE("zThread: tryRunTask - No worker threads available");
        return;
    }
    
    // 直接为每个任务分配一个工作线程
    int successfulAssignments = 0;
    while(!this->m_tasks.empty()) {
        bool foundAvailableWorker = false;
        
        for(size_t i = 0; i < this->m_workerThreads.size(); i++){
            zThread* m_workerThread = m_workerThreads[i];
            
            // 检查工作线程是否可用（不在执行任务）
            if(m_workerThread->isTaskRunning()) {
                LOGI("zThread: tryRunTask - Worker %zu is busy, skipping", i);
                continue; // 跳过忙碌的线程，尝试下一个
            }
            
            // 使用 RAII 锁保护队列操作
            zTask* task = nullptr;
            {
                std::lock_guard<std::mutex> queueLock(m_taskQueueMutex);
                
                // 从队列头部取任务
                task = m_tasks.front();
                m_tasks.erase(m_tasks.begin());
            }
            
            // 直接调用 setExecuteTask 分配任务
            bool success = m_workerThread->setExecuteTask(task);
            if(success) {
                successfulAssignments++;
                LOGI("zThread: tryRunTask - Successfully assigned task '%s' (ID: %s) to worker %zu, remaining tasks: %zu", 
                     task->getTaskName().c_str(), task->getTaskId().c_str(), i, m_tasks.size());
                
                // 验证任务是否真的被设置
                if (!m_workerThread->isTaskRunning()) {
                    LOGW("zThread: tryRunTask - Task assigned to worker %zu but isTaskRunning is false", i);
                }
            } else {
                LOGW("zThread: tryRunTask - Failed to assign task '%s' to worker %zu", 
                     task->getTaskName().c_str(), i);
                
                // 如果分配失败，需要把任务放回队列
                {
                    std::lock_guard<std::mutex> queueLock(m_taskQueueMutex);
                    m_tasks.insert(m_tasks.begin(), task);
                }
            }
            
            foundAvailableWorker = true;
            break; // 分配一个任务后跳出内层循环，重新开始查找
        }
        
        if(!foundAvailableWorker) {
            LOGI("zThread: tryRunTask - No more worker threads available, stopping assignment");
            break;
        }
    }

    LOGI("zThread: tryRunTask completed, assigned %d tasks successfully, remaining tasks: %zu", 
         successfulAssignments, m_tasks.size());
}


