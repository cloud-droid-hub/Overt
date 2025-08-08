# zThread 类 - shared_mutex 读写锁功能

## 概述

`zThread` 类现在支持 `std::shared_mutex` 读写锁功能，提供了更高效的并发控制机制。

## shared_mutex 的优势

1. **读写分离**: 允许多个读者同时访问，但写者独占访问
2. **性能优化**: 在读多写少的场景下，性能显著优于普通互斥锁
3. **C++17 标准**: 使用现代 C++ 标准库，类型安全

## 使用方法

### 1. 创建读写锁

```cpp
zThread threadManager("MyManager");
std::shared_mutex* sharedMutex = threadManager.createSharedMutex();
```

### 2. 读者线程（共享锁）

```cpp
void readerTask(void* arg) {
    std::shared_mutex* sharedMutex = static_cast<std::shared_mutex*>(arg);
    
    // 获取读锁（共享锁）
    std::shared_lock<std::shared_mutex> lock(*sharedMutex);
    
    // 执行读取操作
    // 多个读者可以同时执行这里
    LOGI("Reader thread %lu acquired read lock", (unsigned long)zThread::getCurrentThreadId());
    
    // 模拟读取操作
    zThread::sleep(500);
    
    // 锁会在作用域结束时自动释放
}
```

### 3. 写者线程（独占锁）

```cpp
void writerTask(void* arg) {
    std::shared_mutex* sharedMutex = static_cast<std::shared_mutex*>(arg);
    
    // 获取写锁（独占锁）
    std::unique_lock<std::shared_mutex> lock(*sharedMutex);
    
    // 执行写入操作
    // 写者独占访问，其他读者和写者都会被阻塞
    LOGI("Writer thread %lu acquired write lock", (unsigned long)zThread::getCurrentThreadId());
    
    // 模拟写入操作
    zThread::sleep(1000);
    
    // 锁会在作用域结束时自动释放
}
```

### 4. 完整示例

```cpp
#include "zThread.h"

// 读者任务
void readerTask(void* arg) {
    std::shared_mutex* sharedMutex = static_cast<std::shared_mutex*>(arg);
    LOGI("Reader thread %lu started", (unsigned long)zThread::getCurrentThreadId());
    
    // 获取读锁
    std::shared_lock<std::shared_mutex> lock(*sharedMutex);
    LOGI("Reader thread %lu acquired read lock", (unsigned long)zThread::getCurrentThreadId());
    
    // 模拟读取操作
    zThread::sleep(500);
    
    LOGI("Reader thread %lu released read lock", (unsigned long)zThread::getCurrentThreadId());
}

// 写者任务
void writerTask(void* arg) {
    std::shared_mutex* sharedMutex = static_cast<std::shared_mutex*>(arg);
    LOGI("Writer thread %lu started", (unsigned long)zThread::getCurrentThreadId());
    
    // 获取写锁
    std::unique_lock<std::shared_mutex> lock(*sharedMutex);
    LOGI("Writer thread %lu acquired write lock", (unsigned long)zThread::getCurrentThreadId());
    
    // 模拟写入操作
    zThread::sleep(1000);
    
    LOGI("Writer thread %lu released write lock", (unsigned long)zThread::getCurrentThreadId());
}

int main() {
    zThread threadManager("TestManager", 8);
    threadManager.startThreadPool(4);
    
    // 创建读写锁
    std::shared_mutex* sharedMutex = threadManager.createSharedMutex();
    
    // 创建多个读者线程
    for (int i = 0; i < 3; ++i) {
        threadManager.submitTask(readerTask, sharedMutex, zThread::TaskPriority::NORMAL, "reader_" + std::to_string(i));
    }
    
    // 创建写者线程
    threadManager.submitTask(writerTask, sharedMutex, zThread::TaskPriority::HIGH, "writer");
    
    // 等待任务完成
    zThread::sleep(5000);
    
    threadManager.stopThreadPool(true);
    return 0;
}
```

## 使用场景

### 1. 缓存系统
```cpp
// 多个线程读取缓存
std::shared_lock<std::shared_mutex> readLock(cacheMutex);
// 读取缓存数据
```

```cpp
// 单个线程更新缓存
std::unique_lock<std::shared_mutex> writeLock(cacheMutex);
// 更新缓存数据
```

### 2. 配置管理
```cpp
// 多个线程读取配置
std::shared_lock<std::shared_mutex> readLock(configMutex);
// 读取配置信息
```

```cpp
// 单个线程更新配置
std::unique_lock<std::shared_mutex> writeLock(configMutex);
// 更新配置信息
```

### 3. 数据库连接池
```cpp
// 多个线程获取连接
std::shared_lock<std::shared_mutex> readLock(connectionPoolMutex);
// 获取数据库连接
```

```cpp
// 管理线程维护连接池
std::unique_lock<std::shared_mutex> writeLock(connectionPoolMutex);
// 添加或移除连接
```

## 注意事项

1. **锁的粒度**: 尽量缩小锁的作用域，避免长时间持有锁
2. **死锁预防**: 确保锁的获取顺序一致
3. **性能考虑**: 在读多写少的场景下使用 shared_mutex 效果最佳
4. **内存管理**: shared_mutex 对象由 zThread 管理器自动清理

## 与普通互斥锁的对比

| 特性 | std::mutex | std::shared_mutex |
|------|------------|-------------------|
| 读者并发 | ❌ 串行访问 | ✅ 并发访问 |
| 写者并发 | ❌ 串行访问 | ❌ 串行访问 |
| 性能（读多写少） | 较低 | 较高 |
| 内存开销 | 较小 | 较大 |
| 复杂度 | 简单 | 中等 |

## 编译要求

- C++17 或更高版本
- 包含 `<shared_mutex>` 头文件
- 支持 pthread 的编译环境

## 总结

`shared_mutex` 为 `zThread` 类提供了强大的读写锁功能，特别适合读多写少的并发场景。通过合理使用，可以显著提升应用程序的并发性能。

## 性能优化 - 使用 shared_mutex 进行任务统计

### 问题背景

在原始的 `getPendingTaskCount()` 方法中，使用了普通的 `pthread_mutex` 来保护任务队列的读取操作：

```cpp
// 原始实现（性能较低）
size_t zThread::getPendingTaskCount() const {
    pthread_mutex_lock(&m_taskQueueMutex);
    size_t count = m_taskQueue.size();
    pthread_mutex_unlock(&m_taskQueueMutex);
    return count;
}
```

这种实现存在以下问题：
1. **串行访问**: 多个线程无法同时读取任务统计信息
2. **性能瓶颈**: 在高并发场景下，频繁的统计查询会造成性能问题
3. **锁竞争**: 读取操作与写入操作使用相同的锁，造成不必要的竞争

### 优化方案

使用 `std::shared_mutex` 实现读写分离：

```cpp
// 优化后的实现（支持并发读取）
size_t zThread::getPendingTaskCount() const {
    // 使用共享锁读取任务队列大小
    std::shared_lock<std::shared_mutex> queueLock(m_taskQueueReadMutex);
    size_t queueCount = m_taskQueue.size();
    queueLock.unlock();
    
    // 使用共享锁读取活跃任务数量
    std::shared_lock<std::shared_mutex> activeLock(m_activeTasksReadMutex);
    size_t activeTasksCount = m_activeTasks.size();
    activeLock.unlock();
    
    // 返回总的待处理任务数：队列中的 + 正在执行的
    return queueCount + activeTasksCount;
}
```

### 新增的统计方法

为了更好地支持任务监控，新增了以下方法：

```cpp
// 获取队列中的任务数
size_t getQueuedTaskCount() const;

// 获取正在执行的任务数
size_t getExecutingTaskCount() const;

// 获取活跃任务的总数
size_t getActiveTaskCount() const;

// 获取总的待处理任务数（队列中的 + 正在执行的）
size_t getPendingTaskCount() const;
```

### 性能对比

| 场景 | 原始实现 | 优化后实现 |
|------|----------|------------|
| 单线程读取 | 正常 | 正常 |
| 多线程并发读取 | 串行执行，性能低 | 并行执行，性能高 |
| 读取与写入并发 | 相互阻塞 | 读取不阻塞写入 |
| 内存开销 | 较小 | 稍大（读写锁开销） |

### 使用示例

```cpp
zThread threadManager("TestManager");
threadManager.startThreadPool(4);

// 提交一些任务
for (int i = 0; i < 10; ++i) {
    threadManager.submitTask(testTask, &i);
}

// 多个线程可以同时读取统计信息
std::thread reader1([&]() {
    LOGI("Reader1 - 队列任务数: %zu", threadManager.getQueuedTaskCount());
});

std::thread reader2([&]() {
    LOGI("Reader2 - 活跃任务数: %zu", threadManager.getActiveTaskCount());
});

std::thread reader3([&]() {
    LOGI("Reader3 - 总待处理任务数: %zu", threadManager.getPendingTaskCount());
});

// 所有读取操作可以并发执行，不会相互阻塞
reader1.join();
reader2.join();
reader3.join();
```

### 实现细节

1. **新增成员变量**:
   ```cpp
   mutable std::shared_mutex m_taskQueueReadMutex;    // 任务队列读取锁
   mutable std::shared_mutex m_activeTasksReadMutex;  // 活跃任务读取锁
   ```

2. **使用 `mutable` 关键字**: 允许在 `const` 方法中修改这些锁

3. **RAII 锁管理**: 使用 `std::shared_lock` 确保锁的自动释放

4. **分离读写操作**: 写入操作仍使用原有的 `pthread_mutex`，读取操作使用 `shared_mutex`

### 注意事项

1. **内存开销**: `shared_mutex` 比普通 `mutex` 占用更多内存
2. **编译要求**: 需要 C++17 或更高版本
3. **使用场景**: 适合读多写少的场景，如果写入频繁，性能提升可能不明显
4. **线程安全**: 确保在写入操作时使用独占锁，读取操作时使用共享锁
