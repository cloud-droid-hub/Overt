//
// Created by lxz on 2025/8/7.
//

#ifndef OVERT_ZTHREAD_H
#define OVERT_ZTHREAD_H

#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"
#include <shared_mutex>
#include <pthread.h>
#include <sys/time.h>

// 前向声明
class zThread;

// 线程状态枚举
enum class ThreadState {
    IDLE,       // 空闲
    RUNNING,    // 运行中
    TERMINATED  // 已终止
};

// 任务优先级枚举
enum class TaskPriority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2
};

// 任务结构体
struct Task {
    void (*func)(void*);  // 任务函数指针
    void* arg;            // 任务参数
    TaskPriority priority; // 任务优先级
    string taskId;        // 任务ID
    struct timeval createTime; // 创建时间
    
    Task() : func(nullptr), arg(nullptr), priority(TaskPriority::NORMAL) {
        gettimeofday(&createTime, nullptr);
    }
};

// 线程信息结构体
struct ThreadInfo {
    pthread_t threadId;           // 线程ID
    string name;                  // 线程名称
    ThreadState state;            // 线程状态
    struct timeval lastActiveTime; // 最后活跃时间
    
    ThreadInfo() : threadId(0), state(ThreadState::IDLE) {
        gettimeofday(&lastActiveTime, nullptr);
    }
};

// 延迟任务包装器
struct DelayedTaskWrapper {
    void (*originalFunc)(void*);
    void* originalArg;
    int delayMs;
    
    DelayedTaskWrapper(void (*func)(void*), void* arg, int delay) 
        : originalFunc(func), originalArg(arg), delayMs(delay) {}
};

// 周期性任务包装器
struct PeriodicTaskWrapper {
    void (*originalFunc)(void*);
    void* originalArg;
    int intervalMs;
    string taskId;
    zThread* threadManager;
    
    PeriodicTaskWrapper(void (*func)(void*), void* arg, int interval, const string& id, zThread* manager)
        : originalFunc(func), originalArg(arg), intervalMs(interval), taskId(id), threadManager(manager) {}
};

/**
 * 线程管理类
 * 采用单例模式，提供线程池管理、任务调度、同步机制等功能
 * 支持延迟任务、周期性任务、读写锁等高级功能
 */
class zThread {
private:
    // 私有构造函数，防止外部实例化
    zThread();
    
    // 禁用拷贝构造函数
    zThread(const zThread&) = delete;
    
    // 禁用赋值操作符
    zThread& operator = (const zThread&) = delete;
    
    // 单例实例指针
    static zThread* instance;
    
    // 线程池名称
    string m_name;
    
    // 最大线程数
    size_t m_maxThreads;
    
    // 线程池运行状态
    bool m_running;
    
    // 活跃线程数
    size_t m_activeThreads;
    
    // 工作线程列表
    vector<pthread_t> m_workerThreads;
    
    // 线程信息列表
    vector<ThreadInfo> m_threadInfo;
    
    // 任务队列
    vector<Task> m_taskQueue;
    
    // 活跃任务映射
    map<string, Task> m_activeTasks;
    
    // 消息队列
    vector<string> m_messageQueue;
    
    // 互斥锁列表
    vector<pthread_mutex_t> m_mutexes;
    
    // 条件变量列表
    vector<pthread_cond_t> m_conditionVariables;
    
    // 读写锁列表
    vector<std::shared_mutex*> m_sharedMutexes;
    
    // 信号量列表
    vector<int*> m_semaphores;
    
    // 任务队列互斥锁
    pthread_mutex_t m_taskQueueMutex;
    
    // 任务队列条件变量
    pthread_cond_t m_taskQueueCV;
    
    // 消息队列互斥锁
    pthread_mutex_t m_messageQueueMutex;
    
    // 消息队列条件变量
    pthread_cond_t m_messageQueueCV;
    
    // 活跃任务互斥锁
    pthread_mutex_t m_activeTasksMutex;
    
    // 任务队列读锁（用于统计）
    mutable std::shared_mutex m_taskQueueReadMutex;
    
    // 活跃任务读锁（用于统计）
    mutable std::shared_mutex m_activeTasksReadMutex;
    
    // 私有方法
    void processTask(Task* task);
    void updateThreadState(size_t threadIndex, ThreadState state);
    void cleanupTerminatedThreads();
    string generateTaskId() const;
    void logThreadEvent(const string& event, pthread_t threadId) const;
    
    // 友元函数声明
    friend void* workerThreadFunction(void* arg);
    
public:
    /**
     * 获取单例实例
     * @return zThread单例指针
     */
    static zThread* getInstance();
    
    /**
     * 析构函数
     */
    ~zThread();
    
    // 线程池管理方法
    bool startThreadPool(size_t threadCount = 4);
    void stopThreadPool(bool waitForCompletion = true);
    bool isThreadPoolRunning() const;
    size_t getActiveThreadCount() const;
    
    // 任务管理方法
    bool submitTask(void (*func)(void*), void* arg, TaskPriority priority = TaskPriority::NORMAL, const string& taskId = "");
    bool submitDelayedTask(void (*func)(void*), void* arg, int delayMs, const string& taskId = "");
    bool submitPeriodicTask(void (*func)(void*), void* arg, int intervalMs, const string& taskId = "");
    bool cancelTask(const string& taskId);
    bool waitForAllTasks(int timeoutMs = -1);
    
    // 任务统计方法（非 const，因为需要锁定互斥锁）
    size_t getPendingTaskCount();
    size_t getQueuedTaskCount();
    size_t getExecutingTaskCount();
    size_t getActiveTaskCount();
    
    // 任务检查方法
    bool isTaskActive(const string& taskId);
    
    // 线程同步
    /**
     * 创建互斥锁
     * @return 互斥锁指针
     */
    pthread_mutex_t* createMutex();
    
    /**
     * 创建条件变量
     * @return 条件变量指针
     */
    pthread_cond_t* createConditionVariable();
    
    /**
     * 创建读写锁
     * @return 读写锁指针
     */
    std::shared_mutex* createSharedMutex();
    
    /**
     * 创建信号量
     * @param initialCount 初始计数
     * @return 信号量指针
     */
    int* createSemaphore(int initialCount);
    
    // 线程间通信
    /**
     * 发送消息到指定线程
     * @param threadId 目标线程ID
     * @param message 消息内容
     * @return 是否发送成功
     */
    bool sendMessage(pthread_t threadId, const string& message);
    
    /**
     * 接收消息
     * @param timeoutMs 超时时间（毫秒）
     * @return 接收到的消息
     */
    string receiveMessage(int timeoutMs = -1);
    
    /**
     * 广播消息到所有线程
     * @param message 消息内容
     */
    void broadcastMessage(const string& message);
    
    // 线程监控
    /**
     * 获取线程状态
     * @param threadId 线程ID
     * @return 线程状态
     */
    ThreadState getThreadState(pthread_t threadId) const;
    
    /**
     * 获取线程CPU使用率
     * @param threadId 线程ID
     * @return CPU使用率（百分比）
     */
    double getThreadCpuUsage(pthread_t threadId) const;
    
    /**
     * 获取线程内存使用量
     * @param threadId 线程ID
     * @return 内存使用量（字节）
     */
    size_t getThreadMemoryUsage(pthread_t threadId) const;
    
    /**
     * 获取线程信息列表
     * @return 线程信息列表
     */
    vector<ThreadInfo> getThreadInfo() const;
    
    // 工具方法
    /**
     * 获取当前线程ID
     * @return 当前线程ID
     */
    static pthread_t getCurrentThreadId();
    
    /**
     * 获取当前线程名称
     * @return 当前线程名称
     */
    static string getCurrentThreadName();
    
    /**
     * 设置当前线程名称
     * @param name 线程名称
     */
    static void setCurrentThreadName(const string& name);
    
    /**
     * 线程睡眠
     * @param milliseconds 睡眠时间（毫秒）
     */
    static void sleep(int milliseconds);
    
    /**
     * 线程让出CPU
     */
    static void yield();
};

// 包装器函数声明
void delayedTaskWrapper(void* arg);
void periodicTaskWrapper(void* arg);

// 工作线程函数声明
void* workerThreadFunction(void* arg);

#endif //OVERT_ZTHREAD_H
