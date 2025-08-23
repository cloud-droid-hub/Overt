//
// Created by lxz on 2025/8/21.
//

#ifndef TESTSLEEP_ZTHREADPOOL_H
#define TESTSLEEP_ZTHREADPOOL_H


#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zChildThread.h"


class zThreadPool {
private:
    // 私有构造函数，防止外部实例化
    zThreadPool();

    // 禁用拷贝构造函数
    zThreadPool(const zThreadPool&) = delete;

    // 禁用赋值操作符
    zThreadPool& operator = (const zThreadPool&) = delete;

    // 单例实例指针
    static zThreadPool* instance;

    // 线程池名称
    string m_name;

    // 最大线程数
    size_t m_maxThreads;

    // 线程池运行状态
    bool m_running;

    vector<zChildThread*> m_workerThreads;

    // 添加互斥锁来保护任务队列
    static pthread_mutex_t m_taskQueueMutex;

public:
    /**
     * 获取单例实例
     * @return zThread单例指针
     */
    static zThreadPool* getInstance();

    /**
     * 析构函数
     */
    ~zThreadPool();

    bool startThreadPool(size_t threadCount = 4);

    vector<zTask*> m_tasks;

    template<typename Obj, typename RetType, typename... FuncArgs, typename... Args>
    bool addTask(Obj* obj, RetType (Obj::*memberFunc)(FuncArgs...), Args&&... args) {
        LOGI("zThread: addTask called with object %p", obj);
        
        // 检查对象和函数指针是否有效
        if (!obj || !memberFunc) {
            LOGE("zThread: addTask - Invalid object or member function pointer");
            return false;
        }
        
        // 创建包装 lambda 来调用成员函数
        std::function<void()> taskFunction = [obj, memberFunc, args...]() {
            LOGI("zThread: Executing member function task");
            (obj->*memberFunc)(args...); // 调用成员函数
        };
        
        // 创建 zTask 对象
        zTask* task = new zTask("MemberFunctionTask", taskFunction);

        return addTask(task);
    }

    // 重载：直接添加 std::function
    bool addTask(std::function<void()> taskFunction) {
        LOGI("zThread: addTask(std::function) called");
        
        if (!taskFunction) {
            LOGE("zThread: addTask - Invalid task function");
            return false;
        }
        
        // 创建 zTask 对象
        zTask* task = new zTask("FunctionTask", taskFunction);

        return addTask(task);
    }
    
    // 重载：带任务名的成员函数版本
    template<typename Obj, typename RetType, typename... FuncArgs, typename... Args>
    bool addTask(const string& taskName, Obj* obj, RetType (Obj::*memberFunc)(FuncArgs...), Args&&... args) {
        LOGI("zThread: addTask called with task name '%s' and object %p", taskName.c_str(), obj);
        
        // 检查对象和函数指针是否有效
        if (!obj || !memberFunc) {
            LOGE("zThread: addTask - Invalid object or member function pointer for task '%s'", taskName.c_str());
            return false;
        }
        
        // 创建 zTask 对象
        zTask* task = new zTask(taskName, obj, memberFunc, std::forward<Args>(args)...);

        return addTask(task);
    }
    
    // 重载：带任务名的 std::function 版本
    bool addTask(const string& taskName, std::function<void()> taskFunction) {
        LOGI("zThread: addTask called with task name '%s'", taskName.c_str());
        
        if (!taskFunction) {
            LOGE("zThread: addTask - Invalid task function for task '%s'", taskName.c_str());
            return false;
        }
        
        // 创建 zTask 对象
        zTask* task = new zTask(taskName, taskFunction);

        return addTask(task);
    }
    
    // 重载：带任务名的默认任务版本
    bool addTask(const string& taskName) {
        LOGI("zThread: addTask called with task name '%s' (using default task)", taskName.c_str());
        // 创建带默认任务函数的 zTask 对象
        zTask* task = new zTask(taskName);
        return addTask(task);
    }

    bool addTask(zTask* task) {
        LOGI("zThread: addTask called with task name '%s' (using default task)", task->getTaskName().c_str());

        if (!task->isValid()) {
            LOGE("zThread: addTask - Failed to create valid default task '%s'", task->getTaskName().c_str());
            delete task;
            return false;
        }

        // 加锁保护任务队列
        pthread_mutex_lock(&m_taskQueueMutex);
        m_tasks.push_back(task);
        LOGI("zThread: Default task '%s' (ID: %s) added to queue, current queue size: %zu",
             task->getTaskName().c_str(), task->getTaskId().c_str(), m_tasks.size());
        pthread_mutex_unlock(&m_taskQueueMutex);

        // 尝试运行任务（在锁外调用，避免死锁）
        tryRunTask();
        return true;
    }

    void taskCompletionCallback(string taskName, void* data);

    void tryRunTask();

    bool hasTaskName(string taskName){
        // 使用任务队列锁保护队列操作
        pthread_mutex_lock(&m_taskQueueMutex);
        bool hasTask = false;
        for(zTask* task : m_tasks){
            if (taskName == task->getTaskName()){
                hasTask = true;
                break;
            }
        }
        pthread_mutex_unlock(&m_taskQueueMutex);
        return hasTask;
    }
};


#endif //TESTSLEEP_ZTHREADPOOL_H
