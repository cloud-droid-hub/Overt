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
#include "zChildThread.h"

// 前向声明
class zThread;
class zChildThread;

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
    
    // 工作线程对象列表（使用 zChildThread）
    vector<zChildThread*> m_workerThreads;
    
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
    
    // 任务队列读写锁（提升并发性能）
    mutable std::shared_mutex m_taskQueueMutex;
    
    // 任务队列条件变量
    pthread_cond_t m_taskQueueCV;
    
    // 消息队列互斥锁
    pthread_mutex_t m_messageQueueMutex;
    
    // 消息队列条件变量
    pthread_cond_t m_messageQueueCV;
    
    // 活跃任务读写锁（提升并发性能）
    mutable std::shared_mutex m_activeTasksMutex;
    
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
    
    // 工作线程管理方法
    /**
     * 获取指定索引的工作线程
     * @param index 线程索引
     * @return 工作线程指针，如果索引无效则返回nullptr
     */
    zChildThread* getWorkerThread(size_t index);
    
    /**
     * 获取所有工作线程
     * @return 工作线程列表
     */
    const vector<zChildThread*>& getWorkerThreads() const { return m_workerThreads; }
    
    /**
     * 获取最空闲的工作线程（任务队列最小的线程）
     * @return 最空闲的工作线程指针
     */
    zChildThread* getLeastBusyWorker();
    
    /**
     * 获取工作线程数量
     * @return 工作线程数量
     */
    size_t getWorkerThreadCount() const { return m_workerThreads.size(); }
    
    // 高性能队列操作方法（使用读写锁）
    /**
     * 安全地从队列获取下一个任务
     * @param outTask 输出参数，存储获取的任务
     * @return 是否成功获取任务
     */
    bool getNextTask(Task& outTask);
    
    /**
     * 安全地向队列添加任务
     * @param task 要添加的任务
     */
    void addTaskToQueue(const Task& task);
    
    /**
     * 安全地从活跃任务列表中移除任务
     * @param taskId 要移除的任务ID
     * @return 是否成功移除
     */
    bool removeActiveTask(const string& taskId);
    
    /**
     * 安全地向活跃任务列表添加任务
     * @param task 要添加的任务
     */
    void addActiveTask(const Task& task);
    
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
    
    // 静态回调函数（用于 zChildThread 解耦）
    /**
     * 任务完成回调函数
     * @param taskId 完成的任务ID
     * @param userData 用户数据（zThread指针）
     */
    static void onTaskCompleted(const string& taskId, void* userData);
    
    /**
     * 任务提供者回调函数（用于工作窃取）
     * @param outTask 输出任务
     * @param userData 用户数据（zThread指针）
     * @return 是否成功提供任务
     */
    static bool provideTask(Task& outTask, void* userData);
};

// 包装器函数声明
void delayedTaskWrapper(void* arg);
void periodicTaskWrapper(void* arg);

// 工作线程函数声明
void* workerThreadFunction(void* arg);

#endif //OVERT_ZTHREAD_H
