//
// Created by lxz on 2025/8/7.
//

#ifndef OVERT_ZTHREAD_H
#define OVERT_ZTHREAD_H

// 启用类型转换调试日志
#define DEBUG_TYPE_CONVERSION

#include "zChildThread.h"
#include "zStdUtil.h"
#include <shared_mutex>
#include <functional>
#include <utility>
#include <tuple>
#include <type_traits>
#include <atomic>
#include <errno.h>

// 前向声明
class zThread;

// ========================================================================================
// 辅助宏和函数
// ========================================================================================

// 辅助宏：简化字符串参数的使用
#define SUBMIT_TASK(threadMgr, func, priority, taskId, ...) \
    threadMgr->submitTask(reinterpret_cast<void*>(func), priority, taskId, ##__VA_ARGS__)

// 字符串转换辅助函数
inline string STR(const char* s) { return string(s); }

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
    
    // 任务队列（使用线程安全包装器）
    ThreadSafeQueue<Task> m_taskQueue;
    
    // 活跃任务映射（使用线程安全包装器）
    ThreadSafeMap<string, Task> m_activeTasks;
    
    // 消息队列（使用线程安全包装器）
    ThreadSafeQueue<string> m_messageQueue;
    
    // 互斥锁列表
    vector<pthread_mutex_t> m_mutexes;
    
    // 条件变量列表
    vector<pthread_cond_t> m_conditionVariables;
    
    // 读写锁列表
    vector<std::shared_mutex*> m_sharedMutexes;
    
    // 信号量列表
    vector<int*> m_semaphores;
    
    // 任务队列条件变量（保留用于通知机制）
    pthread_cond_t m_taskQueueCV;
    
    // 任务完成条件变量 - 用于 waitForAllTasks 通知
    pthread_cond_t m_taskCompletionCV;
    pthread_mutex_t m_taskCompletionMutex;
    
    // 工作线程列表锁
    mutable std::shared_mutex m_workerThreadsMutex;
    
    // 任务完成回调机制
    std::function<void()> m_taskCompletionCallback;
    mutable std::shared_mutex m_callbackMutex;
    
    // 注意：消息队列现在使用 ThreadSafeQueue，不再需要额外的锁和条件变量
    
    // 活跃任务读写锁（提升并发性能）
    // 注意：活跃任务映射现在使用 ThreadSafeMap，不需要额外的锁
    
    // 私有方法
    void processTask(Task* task);
    void updateThreadState(size_t threadIndex, ThreadState state);
    void cleanupTerminatedThreads();
    string generateTaskId() const;
    void logThreadEvent(const string& event, pthread_t threadId) const;
    void checkAndTriggerCompletionCallback();
    
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

//    // 任务管理方法
//    bool submitTask(std::function<void()> callable, TaskPriority priority = TaskPriority::NORMAL, const string& taskId = "");
//
private:
    // 辅助模板：获取函数参数类型（定义为静态成员）
    template <typename T>
    struct function_traits;
    
    // 针对函数指针 void(*)(Args...)
    template <typename... Args>
    struct function_traits<void(*)(Args...)> {
        using args_tuple = std::tuple<Args...>;
        static constexpr size_t arg_count = sizeof...(Args);
        
        template <size_t I>
        using arg_type = std::tuple_element_t<I, args_tuple>;
    };
    
    // 智能参数转换：处理常见的类型转换情况（静态函数）
    template <typename TargetType, typename InputType>
    static decltype(auto) convert_argument(InputType&& input) {
        using DecayedInput = std::decay_t<InputType>;
        using DecayedTarget = std::decay_t<TargetType>;
        
        // 调试日志：显示类型信息
        #ifdef DEBUG_TYPE_CONVERSION
        LOGI("[TYPE_DEBUG] TargetType: %s, InputType: %s", 
             typeid(DecayedTarget).name(), typeid(DecayedInput).name());
        #endif
        
        // 情况1：目标是 string，输入是 const char* 或 char*
        if constexpr (std::is_same_v<DecayedTarget, string> && 
                     (std::is_same_v<DecayedInput, const char*> || 
                      std::is_same_v<DecayedInput, char*>)) {
            #ifdef DEBUG_TYPE_CONVERSION
            LOGI("[TYPE_DEBUG] Converting const char* '%s' to string", input);
            #endif
            return string(input);
        }
        // 情况2：目标是 char*，输入是字符串字面量 (const char[N])
        else if constexpr (std::is_same_v<DecayedTarget, char*> && 
                          std::is_array_v<std::remove_reference_t<InputType>> &&
                          std::is_same_v<std::remove_extent_t<std::remove_reference_t<InputType>>, const char>) {
            #ifdef DEBUG_TYPE_CONVERSION
            LOGI("[TYPE_DEBUG] Converting const char[N] '%s' to char* (const_cast)", input);
            #endif
            // 注意：这是不安全的转换，仅用于演示
            return const_cast<char*>(input);
        }
        // 情况3：目标是 const char*，输入是字符串字面量
        else if constexpr (std::is_same_v<DecayedTarget, const char*> && 
                          std::is_array_v<std::remove_reference_t<InputType>> &&
                          std::is_same_v<std::remove_extent_t<std::remove_reference_t<InputType>>, const char>) {
            #ifdef DEBUG_TYPE_CONVERSION
            LOGI("[TYPE_DEBUG] Converting const char[N] '%s' to const char*", input);
            #endif
            return static_cast<const char*>(input);
        }
        else {
            #ifdef DEBUG_TYPE_CONVERSION
            LOGI("[TYPE_DEBUG] Passing argument as-is (no conversion needed)");
            #endif
            // 其他情况直接传递，保持原始类型
            return std::forward<InputType>(input);
        }
    }
    
    // 使用索引序列展开参数并进行智能转换（静态函数）
    template <typename FuncType, typename... Args, size_t... I>
    static auto create_converted_callable(void* func, std::index_sequence<I...>, Args&&... args) {
        using traits = function_traits<FuncType>;
        auto typedFunc = reinterpret_cast<FuncType>(func);
        
        #ifdef DEBUG_TYPE_CONVERSION
        LOGI("[TYPE_DEBUG] create_converted_callable: FuncType=%s, argCount=%zu", 
             typeid(FuncType).name(), sizeof...(Args));
        #endif
        
        return [typedFunc, args...]() {
            #ifdef DEBUG_TYPE_CONVERSION
            LOGI("[TYPE_DEBUG] Executing lambda with %zu arguments", sizeof...(I));
            #endif
            // 对每个参数进行智能转换
            typedFunc(convert_argument<typename traits::template arg_type<I>>(args)...);
        };
    }

public:

    // 【新增】成员函数版本 - 支持对象成员函数调用
    template<typename Obj, typename RetType, typename... FuncArgs, typename... Args>
    zChildThread* submitTaskMember(Obj* obj, RetType (Obj::*memberFunc)(FuncArgs...), Args&&... args) {
        if (!obj || !memberFunc) {
            LOGW("Object or member function pointer is null");
            return nullptr;
        }
        
        // 创建一个新的 zChildThread 对象（不自动启动）
        static size_t taskCounter = 0;
        size_t taskIndex = ++taskCounter;
        string defaultTaskName = "MemberTask_" + itoa(taskIndex, 10);
        
        zChildThread* taskThread = new zChildThread(
            taskIndex, 
            defaultTaskName,
            &zThread::onTaskCompleted,
            &zThread::provideTask,
            this
        );
        
        // 使用对象地址作为 taskId
        char addressStr[32];
        snprintf(addressStr, sizeof(addressStr), "%p", (void*)taskThread);
        string taskId = addressStr;
        
        #ifdef DEBUG_TYPE_CONVERSION
        LOGI("[TYPE_DEBUG] submitTaskMember called: taskName=%s, taskId=%s, argCount=%zu", 
             defaultTaskName.c_str(), taskId.c_str(), sizeof...(Args));
        #endif
        
        // 创建任务对象
        Task task;
        task.taskId = taskId;
        task.priority = TaskPriority::NORMAL;  // 默认优先级
        gettimeofday(&task.createTime, nullptr);
        
        // 创建包装 lambda 来调用成员函数
        task.callable = [obj, memberFunc, args...]() {
            #ifdef DEBUG_TYPE_CONVERSION
            LOGI("[TYPE_DEBUG] Calling member function with smart conversion");
            #endif
            // 智能转换参数并调用成员函数
            (obj->*memberFunc)(convert_argument<FuncArgs>(args)...);
        };
        
        // 将任务添加到活跃任务列表用于跟踪
        addActiveTask(task);
        
        // 将任务添加到中央队列（新的架构）
        addTaskToQueue(task);
        
        // 将线程添加到管理列表
        {
            std::unique_lock<std::shared_mutex> lock(m_workerThreadsMutex);
            m_workerThreads.push_back(taskThread);
        }
        
        return taskThread;  // 返回线程指针用于链式调用
    }

    // 【新增】明确函数签名的版本 - 返回 zChildThread* 用于链式调用
    template<typename FuncType, typename... Args>
    zChildThread* submitTaskTyped(FuncType funcPtr, Args&&... args) {
        
        // 创建一个新的 zChildThread 对象（不自动启动）
        static size_t taskCounter = 0;
        size_t taskIndex = ++taskCounter;
        string defaultTaskName = "Task_" + itoa(taskIndex, 10);
        
        zChildThread* taskThread = new zChildThread(
            taskIndex, 
            defaultTaskName,
            &zThread::onTaskCompleted,
            &zThread::provideTask,
            this
        );
        
        // 使用对象地址作为 taskId
        char addressStr[32];
        snprintf(addressStr, sizeof(addressStr), "%p", (void*)taskThread);
        string taskId = addressStr;
        
        constexpr size_t argCount = sizeof...(Args);
        
        #ifdef DEBUG_TYPE_CONVERSION
        LOGI("[TYPE_DEBUG] submitTaskTyped called: taskName=%s, taskId=%s, argCount=%zu, FuncType=%s", 
             defaultTaskName.c_str(), taskId.c_str(), sizeof...(Args), typeid(FuncType).name());
        #endif
        
        // 创建任务对象
        Task task;
        task.taskId = taskId;
        task.priority = TaskPriority::NORMAL;  // 默认优先级
        gettimeofday(&task.createTime, nullptr);
        
        // 检测是否是函数指针类型
        if constexpr (std::is_function_v<std::remove_pointer_t<FuncType>>) {
            // 这是一个函数指针，使用智能转换逻辑
            void* func = reinterpret_cast<void*>(funcPtr);
            if constexpr (argCount == 0) {
                task.callable = [funcPtr]() {
                    funcPtr();
                };
            } else {
                task.callable = create_converted_callable<FuncType>(func, std::make_index_sequence<argCount>{}, std::forward<Args>(args)...);
            }
        } else {
            // 这是 lambda/std::function，直接调用
            #ifdef DEBUG_TYPE_CONVERSION
            LOGI("[TYPE_DEBUG] Detected lambda/std::function - direct call");
            #endif
            
            task.callable = [funcPtr, args...]() {
                if constexpr (argCount == 0) {
                    funcPtr();
                } else {
                    funcPtr(args...);
                }
            };
        }
        
        // 将任务添加到活跃任务列表用于跟踪
        addActiveTask(task);
        
        // 将任务添加到中央队列（新的架构）
        addTaskToQueue(task);
        
        // 将线程添加到管理列表
        {
            std::unique_lock<std::shared_mutex> lock(m_workerThreadsMutex);
            m_workerThreads.push_back(taskThread);
        }
        
        return taskThread;  // 返回线程指针用于链式调用
    }


    bool cancelTask(const string& taskId);
    bool waitForAllTasks(int timeoutMs = 0);
    
    /**
     * 设置任务完成回调函数
     * @param callback 当所有任务完成时调用的回调函数
     */
    void setTaskCompletionCallback(std::function<void()> callback);
    
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
