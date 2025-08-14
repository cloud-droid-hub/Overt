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
#include <unordered_map>
#include <sys/time.h>
#include <functional>

// ========================================================================================
// 线程安全数据包装器 - 提供原子操作和锁保护的数据访问
// ========================================================================================

/**
 * 线程安全的队列包装器
 * 提供原子操作的队列访问方法
 */
template<typename T>
class ThreadSafeQueue {
private:
    mutable std::shared_mutex m_mutex;
    std::queue<T> m_queue;
    std::atomic<size_t> m_size{0};  // 原子的大小计数器，避免频繁加锁
    
    // 用于阻塞等待的条件变量（可选）
    mutable pthread_cond_t m_condition;
    mutable pthread_mutex_t m_conditionMutex;

public:
    /**
     * 构造函数 - 初始化条件变量
     */
    ThreadSafeQueue() {
        pthread_cond_init(&m_condition, nullptr);
        pthread_mutex_init(&m_conditionMutex, nullptr);
    }
    
    /**
     * 析构函数 - 销毁条件变量
     */
    ~ThreadSafeQueue() {
        pthread_cond_destroy(&m_condition);
        pthread_mutex_destroy(&m_conditionMutex);
    }
    
    /**
     * 向队列添加元素
     * @param item 要添加的元素
     */
    void push(const T& item) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_queue.push(item);
        m_size.fetch_add(1);
        
        // 通知等待的线程
        pthread_mutex_lock(&m_conditionMutex);
        pthread_cond_signal(&m_condition);
        pthread_mutex_unlock(&m_conditionMutex);
    }

    /**
     * 从队列取出元素
     * @param item 输出参数，存储取出的元素
     * @return 是否成功取出元素
     */
    bool pop(T& item) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        item = m_queue.front();
        m_queue.pop();
        m_size.fetch_sub(1);
        return true;
    }

    /**
     * 获取队列大小（原子操作，无需加锁）
     * @return 队列大小
     */
    size_t size() const {
        return m_size.load();
    }

    /**
     * 检查队列是否为空（原子操作）
     * @return 是否为空
     */
    bool empty() const {
        return m_size.load() == 0;
    }

    /**
     * 清空队列
     */
    void clear() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            m_queue.pop();
        }
        m_size.store(0);
    }

    /**
     * 按条件移除元素（支持任务取消功能）
     * @param predicate 判断条件函数
     * @return 移除的元素数量
     */
    template<typename Predicate>
    size_t remove_if(Predicate predicate) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        std::queue<T> tempQueue;
        size_t removedCount = 0;
        
        // 遍历队列，保留不符合条件的元素
        while (!m_queue.empty()) {
            T item = m_queue.front();
            m_queue.pop();
            
            if (predicate(item)) {
                removedCount++;
            } else {
                tempQueue.push(item);
            }
        }
        
        // 将保留的元素放回队列
        m_queue = std::move(tempQueue);
        m_size.fetch_sub(removedCount);
        
        return removedCount;
    }
    
    /**
     * 阻塞等待并取出元素
     * @param item 输出参数，存储取出的元素
     * @param timeoutMs 超时时间（毫秒），-1表示无限等待
     * @return 是否成功取出元素
     */
    bool wait_and_pop(T& item, int timeoutMs = -1) {
        pthread_mutex_lock(&m_conditionMutex);
        
        if (timeoutMs > 0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += timeoutMs * 1000000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec += ts.tv_nsec / 1000000000;
                ts.tv_nsec %= 1000000000;
            }
            
            while (empty()) {
                if (pthread_cond_timedwait(&m_condition, &m_conditionMutex, &ts) != 0) {
                    pthread_mutex_unlock(&m_conditionMutex);
                    return false;  // 超时
                }
            }
        } else {
            while (empty()) {
                pthread_cond_wait(&m_condition, &m_conditionMutex);
            }
        }
        
        pthread_mutex_unlock(&m_conditionMutex);
        
        // 尝试取出元素
        return pop(item);
    }
};

/**
 * 线程安全的映射包装器
 * 提供原子操作的映射访问方法
 */
template<typename K, typename V>
class ThreadSafeMap {
private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<K, V> m_map;
    std::atomic<size_t> m_size{0};

public:
    /**
     * 插入或更新元素
     * @param key 键
     * @param value 值
     */
    void set(const K& key, const V& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        bool isNew = (m_map.find(key) == m_map.end());
        m_map[key] = value;
        if (isNew) {
            m_size.fetch_add(1);
        }
    }

    /**
     * 移除元素
     * @param key 键
     * @return 是否成功移除
     */
    bool remove(const K& key) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            m_map.erase(it);
            m_size.fetch_sub(1);
            return true;
        }
        return false;
    }

    /**
     * 检查元素是否存在
     * @param key 键
     * @return 是否存在
     */
    bool contains(const K& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_map.find(key) != m_map.end();
    }

    /**
     * 获取映射大小（原子操作）
     * @return 映射大小
     */
    size_t size() const {
        return m_size.load();
    }

    /**
     * 执行函数对所有元素（读取操作）
     * @param func 要执行的函数
     */
    void for_each(std::function<void(const K&, const V&)> func) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        for (const auto& pair : m_map) {
            func(pair.first, pair.second);
        }
    }
};

/**
 * 线程安全的状态管理器
 * 封装原子状态变量和相关操作
 */
template<typename StateType>
class ThreadSafeState {
private:
    std::atomic<StateType> m_state;

public:
    /**
     * 构造函数
     * @param initialState 初始状态
     */
    explicit ThreadSafeState(StateType initialState) : m_state(initialState) {}

    /**
     * 设置状态（原子操作）
     * @param newState 新状态
     */
    void set(StateType newState) {
        m_state.store(newState);
    }

    /**
     * 获取状态（原子操作）
     * @return 当前状态
     */
    StateType get() const {
        return m_state.load();
    }

    /**
     * 比较并交换状态（原子操作）
     * @param expected 期望的当前状态
     * @param desired 期望设置的新状态
     * @return 是否成功交换
     */
    bool compare_exchange(StateType& expected, StateType desired) {
        return m_state.compare_exchange_strong(expected, desired);
    }
};

/**
 * 线程安全的计数器
 * 提供原子操作的计数功能
 */
class ThreadSafeCounter {
private:
    std::atomic<size_t> m_count{0};

public:
    /**
     * 增加计数
     * @param delta 增加的数量，默认为1
     * @return 增加后的值
     */
    size_t add(size_t delta = 1) {
        return m_count.fetch_add(delta) + delta;
    }

    /**
     * 减少计数
     * @param delta 减少的数量，默认为1
     * @return 减少后的值
     */
    size_t sub(size_t delta = 1) {
        return m_count.fetch_sub(delta) - delta;
    }

    /**
     * 获取当前计数
     * @return 当前计数值
     */
    size_t get() const {
        return m_count.load();
    }

    /**
     * 设置计数值
     * @param value 新的计数值
     */
    void set(size_t value) {
        m_count.store(value);
    }

    /**
     * 重置计数为0
     */
    void reset() {
        m_count.store(0);
    }
};

// ========================================================================================
// 枚举定义
// ========================================================================================

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
    std::function<void()> callable; // std::function 可调用对象
    TaskPriority priority; // 任务优先级
    string taskId;        // 任务ID
    struct timeval createTime; // 创建时间

    // 默认构造函数
    Task() : func(nullptr), arg(nullptr), priority(TaskPriority::NORMAL) {
        gettimeofday(&createTime, nullptr);
    }
    
    // 使用函数指针的构造函数
    Task(void (*f)(void*), void* a, TaskPriority p = TaskPriority::NORMAL, const string& id = "")
        : func(f), arg(a), priority(p), taskId(id) {
        gettimeofday(&createTime, nullptr);
    }
    
    // 使用 std::function 的构造函数
    Task(std::function<void()> c, TaskPriority p = TaskPriority::NORMAL, const string& id = "")
        : func(nullptr), arg(nullptr), callable(c), priority(p), taskId(id) {
        gettimeofday(&createTime, nullptr);
    }
    
    // 执行任务的方法
    void execute() const {
        if (callable) {
            callable();  // 调用 std::function
        } else if (func) {
            func(arg);    // 调用函数指针
        }
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
 * 负责管理自己的状态和任务执行，不管理任务队列
 */
class zChildThread {
private:
    // 线程基本信息
    pthread_t m_threadId;           // 线程ID
    string m_threadName;            // 线程名称
    size_t m_threadIndex;           // 线程索引
    TaskPriority m_threadPriority;  // 线程优先级
    
    // 回调函数（实现解耦）
    TaskCompletionCallback m_completionCallback;   // 任务完成回调
    TaskProviderCallback m_taskProviderCallback;   // 任务提供者回调（从主线程获取任务）
    void* m_callbackUserData;                      // 回调用户数据
    
    // 线程状态（使用线程安全包装器）
    ThreadSafeState<WorkerState> m_state;           // 线程安全的状态管理
    ThreadSafeState<bool> m_running;                // 线程安全运行标志
    
    // 统计信息（使用线程安全计数器）
    ThreadSafeCounter m_processedTasks;     // 已处理任务数
    ThreadSafeCounter m_currentTaskCount;   // 当前任务数（现在总是0或1）
    
    // 线程同步
    pthread_cond_t m_taskCV;                        // 任务条件变量
    pthread_mutex_t m_taskMutex;                    // 任务互斥锁
    
    // 活跃时间管理
    struct timeval m_lastActiveTime;                // 最后活跃时间
    mutable std::shared_mutex m_lastActiveTimeMutex; // 活跃时间读写锁

public:
    /**
     * 构造函数
     * @param index 线程索引
     * @param name 线程名称
     * @param completionCallback 任务完成回调
     * @param taskProviderCallback 任务提供者回调
     * @param userData 回调用户数据
     */
    zChildThread(size_t index, const string& name, 
                 TaskCompletionCallback completionCallback,
                 TaskProviderCallback taskProviderCallback,
                 void* userData);
    
    /**
     * 析构函数
     */
    ~zChildThread();
    
    // 线程生命周期管理
    bool start();                    // 启动线程
    void stop(bool waitForCompletion = true);  // 停止线程
    void join();                     // 等待线程结束
    
    // 任务执行（不再管理队列）
    bool executeTask(const Task& task);  // 直接执行指定任务
    
    // 状态查询
    bool isRunning() const { return m_running.get(); }
    WorkerState getState() const { return m_state.get(); }
    pthread_t getThreadId() const { return m_threadId; }
    string getThreadName() const { return m_threadName; }
    size_t getThreadIndex() const { return m_threadIndex; }
    struct timeval getLastActiveTime() const;
    size_t getProcessedTaskCount() const { return m_processedTasks.get(); }
    
    // 线程配置
    bool setAffinity(int cpuId);     // 设置CPU亲和性
    bool setPriority(int priority);  // 设置线程优先级
    double getCpuUsage() const;      // 获取CPU使用率
    size_t getMemoryUsage() const;   // 获取内存使用量
    
    // 链式调用方法
    zChildThread* setName(const string& name);
    string getName() const;
    zChildThread* setLevel(TaskPriority priority);
    TaskPriority getLevel() const;

private:
    // 私有方法
    static void* threadEntry(void* arg);  // 静态线程入口函数
    void* threadMain();                   // 线程主函数
    void processTask(const Task& task);   // 处理任务
    void updateLastActiveTime();          // 更新活跃时间
    
    // 尝试从主线程获取任务
    bool tryGetTask(Task& outTask);
};

#endif //OVERT_ZCHILDTHREAD_H
