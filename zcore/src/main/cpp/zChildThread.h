//
// Created by Assistant on 2025/8/7.
// zChildThread - 最小轻量级工作线程类
//

#ifndef OVERT_ZCHILDTHREAD_H
#define OVERT_ZCHILDTHREAD_H

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <functional>

#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zTask.h"

// 前向声明
class zTask;

/**
 * 最小轻量级工作线程类
 * 提供简单的循环等待机制，支持外部线程通过互斥锁和条件变量控制
 */
class zChildThread {
private:
    // 线程基本信息
    void* m_threadId;               // 线程ID
    string m_threadName;            // 线程名称
    size_t m_threadIndex;           // 线程索引

    // 线程同步
    void* m_taskCV;                 // 任务条件变量
    void* m_taskMutex;              // 任务互斥锁
    
    // 执行函数
    void (*m_executeFunc)(void*);         // 执行函数指针
    void* m_executeFuncArg;               // 执行函数参数

    std::function<void()> m_callable;
    
    // zTask 对象
    zTask* m_task;                        // 当前执行的任务对象

    bool m_isTaskRunning;                   // 是否有任务正在执行
    bool m_isThreadRunning;                 // 线程是否终止并销毁

    void (*m_taskCompletionCallback)(string taskId, void* userData);
    std::function<void(string, void*)> m_taskCompletionCallbackFunc;  // std::function 版本的回调

public:
    /**
     * 构造函数
     * @param index 线程索引
     * @param name 线程名称
     */
    zChildThread();
    zChildThread(size_t index, const string& name);
    
    /**
     * 析构函数
     */
    ~zChildThread();
    
    // 线程生命周期管理
    bool start();                    // 启动线程
    void stop();                     // 停止线程
    void join();                     // 等待线程结束

    // 设置执行函数并唤醒线程
    void setTaskCompletionCallback(void (*m_taskCompletionCallback)(string taskId, void* userData));
    
    // std::function 版本的重载函数
    void setTaskCompletionCallback(std::function<void(string, void*)> callback);


    // 设置执行函数并唤醒线程
    bool setExecuteFunction(void (*func)(void*), void* arg = nullptr);  // 设置执行函数并立即唤醒线程，返回是否成功


    template<typename Obj, typename RetType, typename... FuncArgs, typename... Args>
    bool setExecuteFunction(Obj* obj, RetType (Obj::*memberFunc)(FuncArgs...), Args&&... args) {
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

        if (!obj || !memberFunc) {
            LOGW("Object or member function pointer is null");
            return false;
        }


        this->m_executeFunc = nullptr;
        this->m_executeFuncArg = nullptr;

        // 创建包装 lambda 来调用成员函数
        this->m_callable = [obj, memberFunc, args...]() {
            (obj->*memberFunc)(convert_argument<FuncArgs>(args)...); // 智能转换参数并调用成员函数
        };
        // 立即唤醒线程执行任务
        pthread_mutex_lock((pthread_mutex_t*)m_taskMutex);
        pthread_cond_signal((pthread_cond_t*)m_taskCV);
        pthread_mutex_unlock((pthread_mutex_t*)m_taskMutex);
        LOGI("zChildThread[%zu] awakened to execute new function", m_threadIndex);
        return true;
    }

    bool setExecuteFunction(std::function<void()> callable);
    
    // 设置 zTask 对象并执行
    bool setExecuteTask(zTask* task);


    // 状态查询
    bool isThreadRunning() const;          // 检查线程是否正在运行
    bool isTaskRunning() const;        // 检查是否正在执行任务

    void* getThreadId() const { return m_threadId; }
    string getThreadName() const { return m_threadName; }
    size_t getThreadIndex() const { return m_threadIndex; }
    
    // 设置方法
    zChildThread* setName(const string& name);
    string getName() const;

    string getTaskName(){
        if (!isTaskRunning()) return "";
        return m_task->getTaskName();
    }


private:
    // 私有方法
    static void* threadEntry(void* arg);  // 静态线程入口函数
    void* threadMain();                   // 线程主函数
};

#endif //OVERT_ZCHILDTHREAD_H
