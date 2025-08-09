//
// Created by Assistant on 2025/8/7.
// zChildThread - 工作线程类，每个线程作为独立对象管理自己的状态和任务
//

#ifndef OVERT_ZCHILDTHREAD_H
#define OVERT_ZCHILDTHREAD_H

#include "zLog.h"
#include "zStd.h"
#include <pthread.h>
#include <shared_mutex>
#include <atomic>
#include <queue>
#include <sys/time.h>

// 任务优先级枚举
enum class TaskPriority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2
};

// 线程状态枚举（zThread 使用）
enum class ThreadState {
    IDLE,       // 空闲
    RUNNING,    // 运行中
    TERMINATED  // 已终止
};

// 工作线程状态枚举（zChildThread 使用）
enum class WorkerState {
    IDLE,           // 空闲
    RUNNING,        // 运行中
    WAITING,        // 等待任务
    TERMINATED      // 已终止
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

// 任务完成回调函数类型
typedef void (*TaskCompletionCallback)(const string& taskId, void* userData);

// 任务提供者回调函数类型（用于工作窃取）
typedef bool (*TaskProviderCallback)(Task& outTask, void* userData);

// 周期性任务包装器
struct PeriodicTaskWrapper {
    void (*originalFunc)(void*);
    void* originalArg;
    int intervalMs;
    string taskId;
    TaskCompletionCallback completionCallback;  // 完成回调
    void* callbackUserData;                     // 回调用户数据
    
    PeriodicTaskWrapper(void (*func)(void*), void* arg, int interval, const string& id, 
                       TaskCompletionCallback callback = nullptr, void* userData = nullptr)
        : originalFunc(func), originalArg(arg), intervalMs(interval), taskId(id)
        , completionCallback(callback), callbackUserData(userData) {}
};

/**
 * 工作线程类 - 每个工作线程作为独立对象
 * 负责管理自己的状态、任务队列和生命周期
 */
class zChildThread {
private:
    // 线程基本信息
    pthread_t m_threadId;           // 线程ID
    string m_threadName;            // 线程名称
    size_t m_threadIndex;           // 线程索引
    
    // 回调函数（实现解耦）
    TaskCompletionCallback m_completionCallback;   // 任务完成回调
    TaskProviderCallback m_taskProviderCallback;   // 任务提供者回调（用于工作窃取）
    void* m_callbackUserData;                      // 回调用户数据
    
    // 线程状态
    std::atomic<WorkerState> m_state;       // 原子状态变量
    std::atomic<bool> m_running;            // 运行标志
    struct timeval m_lastActiveTime;        // 最后活跃时间
    
    // 任务队列（每个线程有自己的本地队列）
    std::queue<Task> m_localTaskQueue;      // 本地任务队列
    mutable std::shared_mutex m_queueMutex; // 队列锁
    pthread_cond_t m_queueCV;               // 队列条件变量
    pthread_mutex_t m_queueCondMutex;       // 条件变量专用锁
    
    // 统计信息
    std::atomic<size_t> m_processedTasks;   // 已处理任务数
    std::atomic<size_t> m_currentTaskCount; // 当前任务数
    
    // 私有方法
    void* threadMain();                     // 线程主函数
    void processTask(const Task& task);     // 处理任务
    void updateLastActiveTime();            // 更新活跃时间
    
    // 静态线程入口函数
    static void* threadEntry(void* arg);

public:
    /**
     * 构造函数
     * @param index 线程索引
     * @param name 线程名称
     * @param completionCallback 任务完成回调函数
     * @param taskProviderCallback 任务提供者回调函数（用于工作窃取）
     * @param userData 回调函数的用户数据
     */
    zChildThread(size_t index, const string& name, 
                TaskCompletionCallback completionCallback = nullptr,
                TaskProviderCallback taskProviderCallback = nullptr,
                void* userData = nullptr);
    
    /**
     * 析构函数
     */
    ~zChildThread();
    
    // 禁用拷贝
    zChildThread(const zChildThread&) = delete;
    zChildThread& operator=(const zChildThread&) = delete;
    
    // 线程生命周期管理
    /**
     * 启动线程
     * @return 是否启动成功
     */
    bool start();
    
    /**
     * 停止线程
     * @param waitForCompletion 是否等待任务完成
     */
    void stop(bool waitForCompletion = true);
    
    /**
     * 等待线程结束
     */
    void join();
    
    // 任务管理
    /**
     * 添加任务到本地队列
     * @param task 任务对象
     * @return 是否添加成功
     */
    bool addTask(const Task& task);
    
    /**
     * 尝试从外部获取任务（工作窃取算法）
     * 通过回调函数从管理器获取任务
     * @return 是否获取成功
     */
    bool tryGetExternalTask();
    
    /**
     * 获取当前任务队列大小
     * @return 队列大小
     */
    size_t getQueueSize() const;
    
    /**
     * 检查是否有任务
     * @return 是否有任务
     */
    bool hasTasks() const;
    
    // 状态查询
    /**
     * 获取线程状态
     * @return 当前状态
     */
    WorkerState getState() const { return m_state.load(); }
    
    /**
     * 获取线程ID
     * @return 线程ID
     */
    pthread_t getThreadId() const { return m_threadId; }
    
    /**
     * 获取线程名称
     * @return 线程名称
     */
    const string& getThreadName() const { return m_threadName; }
    
    /**
     * 获取线程索引
     * @return 线程索引
     */
    size_t getThreadIndex() const { return m_threadIndex; }
    
    /**
     * 是否正在运行
     * @return 运行状态
     */
    bool isRunning() const { return m_running.load(); }
    
    /**
     * 获取已处理任务数
     * @return 已处理任务数
     */
    size_t getProcessedTaskCount() const { return m_processedTasks.load(); }
    
    /**
     * 获取最后活跃时间
     * @return 最后活跃时间
     */
    struct timeval getLastActiveTime() const { return m_lastActiveTime; }
    
    // 高级功能
    /**
     * 设置线程亲和性（绑定到特定CPU核心）
     * @param cpuId CPU核心ID
     * @return 是否设置成功
     */
    bool setAffinity(int cpuId);
    
    /**
     * 设置线程优先级
     * @param priority 优先级
     * @return 是否设置成功
     */
    bool setPriority(int priority);
    
    /**
     * 获取线程CPU使用率
     * @return CPU使用率（百分比）
     */
    double getCpuUsage() const;
    
    /**
     * 获取线程内存使用量
     * @return 内存使用量（字节）
     */
    size_t getMemoryUsage() const;
};

#endif //OVERT_ZCHILDTHREAD_H
