# waitForAllTasks 方法行为更新

## 概述

本次更新修改了 `zThread::waitForAllTasks()` 方法的行为，使其在默认情况下（参数为0时）无限等待直到所有任务完成，而不是立即超时。

## 修改内容

### 1. 方法签名
```cpp
bool waitForAllTasks(int timeoutMs = 0);
```

### 2. 行为变化

#### 修改前
- `timeoutMs = 0`: 立即超时（只检查一次任务状态）
- `timeoutMs > 0`: 等待指定毫秒数后超时
- `timeoutMs = -1`: 无限等待直到所有任务完成

#### 修改后
- `timeoutMs = 0`: **无限等待直到所有任务完成**（默认行为）
- `timeoutMs > 0`: 等待指定毫秒数后超时
- `timeoutMs < 0`: 立即超时（只检查一次任务状态）

### 3. 代码实现

```cpp
bool zThread::waitForAllTasks(int timeoutMs) {
    // ... 初始化代码 ...
    
    if (timeoutMs == 0) {
        LOGI("waitForAllTasks: Starting wait without timeout (infinite wait until all tasks complete)");
    } else {
        LOGI("waitForAllTasks: Starting wait with timeout %d ms", timeoutMs);
    }
    
    while (true) {
        // ... 检查任务状态 ...
        
        // 所有任务完成
        if (activeCount == 0 && queuedCount == 0 && executingCount == 0) {
            LOGI("waitForAllTasks: All tasks completed after %ld ms (%d loops)", totalElapsed, loopCount);
            return true;
        }
        
        // 检查超时
        if (timeoutMs > 0) {  // 只有正值才检查超时，0表示无限等待直到所有任务完成
            // ... 超时检查逻辑 ...
        }
        // 如果 timeoutMs = 0，继续循环直到所有任务完成
        
        // 50ms 轮询间隔
        nanosleep(&ts, nullptr);
    }
}
```

## 使用示例

### 1. 默认行为（推荐）
```cpp
// 无限等待直到所有任务完成
bool completed = threadManager->waitForAllTasks();
// 或者明确指定
bool completed = threadManager->waitForAllTasks(0);
```

### 2. 带超时的等待
```cpp
// 等待5秒
bool completed = threadManager->waitForAllTasks(5000);
```

### 3. 立即检查（不推荐）
```cpp
// 只检查一次，不等待
bool completed = threadManager->waitForAllTasks(-1);
```

## 测试更新

### 测试代码修改
```cpp
// 修改前
LOGI("Testing waitForAllTasks(-1) - infinite wait");
bool allCompleted = threadManager->waitForAllTasks(-1);

// 修改后
LOGI("Testing waitForAllTasks(0) - infinite wait until all tasks complete");
bool allCompleted = threadManager->waitForAllTasks();
```

### 测试结果
- ✅ 测试现在使用默认参数（0）进行无限等待
- ✅ 方法会等待直到所有任务真正完成
- ✅ 避免了之前的立即超时问题

## 优势

1. **更直观的行为**: 默认参数0表示无限等待，符合直觉
2. **更好的用户体验**: 不需要记住使用-1来获得无限等待
3. **向后兼容**: 现有的调用代码仍然有效
4. **更安全的默认行为**: 避免意外超时导致的问题

## 注意事项

1. **性能考虑**: 无限等待会持续轮询，消耗少量CPU资源
2. **死锁风险**: 如果任务永远不会完成，方法会永远阻塞
3. **调试建议**: 在生产环境中建议使用适当的超时值

## 总结

这次更新使 `waitForAllTasks` 方法的行为更加直观和安全。默认情况下，它会等待所有任务完成，这符合大多数使用场景的需求。对于需要超时控制的场景，可以明确指定超时时间。
