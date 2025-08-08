# zThread 类 - 任务包装器解决方案

## 问题背景

在实现延迟任务和周期性任务时，遇到了 lambda 表达式无法转换为函数指针的问题：

```cpp
// 错误的实现（编译错误）
bool zThread::submitDelayedTask(void (*func)(void*), void* arg, int delayMs, const string& taskId) {
    auto delayedFunc = [func, arg, delayMs](void*) {  // ❌ 无法转换为 void(*)(void*)
        // 延迟逻辑
        if (func) func(arg);
    };
    
    return submitTask(delayedFunc, nullptr, TaskPriority::NORMAL, taskId);
}
```

## 解决方案

使用任务包装器结构体和静态包装器函数来解决这个问题：

### 1. 包装器结构体

```cpp
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
```

### 2. 静态包装器函数

```cpp
// 延迟任务包装器函数
static void delayedTaskWrapper(void* arg) {
    DelayedTaskWrapper* wrapper = static_cast<DelayedTaskWrapper*>(arg);
    
    // 延迟执行
    struct timespec ts;
    ts.tv_sec = wrapper->delayMs / 1000;
    ts.tv_nsec = (wrapper->delayMs % 1000) * 1000000;
    nanosleep(&ts, nullptr);
    
    // 执行原始任务
    if (wrapper->originalFunc) {
        wrapper->originalFunc(wrapper->originalArg);
    }
    
    // 清理包装器
    delete wrapper;
}

// 周期性任务包装器函数
static void periodicTaskWrapper(void* arg) {
    PeriodicTaskWrapper* wrapper = static_cast<PeriodicTaskWrapper*>(arg);
    
    while (wrapper->threadManager->isThreadPoolRunning()) {
        // 执行原始任务
        if (wrapper->originalFunc) {
            wrapper->originalFunc(wrapper->originalArg);
        }
        
        // 等待间隔时间
        struct timespec ts;
        ts.tv_sec = wrapper->intervalMs / 1000;
        ts.tv_nsec = (wrapper->intervalMs % 1000) * 1000000;
        nanosleep(&ts, nullptr);
    }
    
    // 清理包装器
    delete wrapper;
}
```

### 3. 更新后的方法实现

```cpp
bool zThread::submitDelayedTask(void (*func)(void*), void* arg, int delayMs, const string& taskId) {
    // 创建延迟任务包装器
    DelayedTaskWrapper* wrapper = new DelayedTaskWrapper(func, arg, delayMs);
    
    return submitTask(delayedTaskWrapper, wrapper, TaskPriority::NORMAL, taskId);
}

bool zThread::submitPeriodicTask(void (*func)(void*), void* arg, int intervalMs, const string& taskId) {
    string actualTaskId = taskId.empty() ? generateTaskId() : taskId;
    
    // 创建周期性任务包装器
    PeriodicTaskWrapper* wrapper = new PeriodicTaskWrapper(func, arg, intervalMs, actualTaskId, this);
    
    return submitTask(periodicTaskWrapper, wrapper, TaskPriority::NORMAL, actualTaskId);
}
```

## 优势

1. **类型安全**: 使用函数指针，编译时类型检查
2. **内存管理**: 包装器在任务完成后自动清理
3. **灵活性**: 可以传递任意函数指针和参数
4. **兼容性**: 不依赖 C++11 lambda 或 std::function

## 使用示例

```cpp
// 定义任务函数
void myTask(void* arg) {
    int* data = static_cast<int*>(arg);
    LOGI("Task executed with data: %d", *data);
}

// 使用延迟任务
int data = 100;
threadManager.submitDelayedTask(myTask, &data, 2000, "delayed_task");

// 使用周期性任务
int periodicData = 200;
threadManager.submitPeriodicTask(myTask, &periodicData, 1000, "periodic_task");
```

## 注意事项

1. **内存管理**: 包装器对象在任务完成后自动删除
2. **线程安全**: 包装器在任务执行线程中创建和销毁
3. **参数传递**: 原始参数通过包装器传递，确保类型安全
4. **错误处理**: 在包装器函数中检查函数指针是否有效

## 与 lambda 方案的对比

| 特性 | Lambda 方案 | 包装器方案 |
|------|-------------|------------|
| 编译要求 | C++11+ | C++98+ |
| 类型安全 | 高 | 高 |
| 内存开销 | 较小 | 稍大 |
| 调试友好 | 中等 | 高 |
| 兼容性 | 现代编译器 | 所有编译器 |

## 总结

包装器方案提供了一个简单、高效且兼容性好的解决方案，避免了 lambda 表达式转换问题，同时保持了代码的清晰性和可维护性。

