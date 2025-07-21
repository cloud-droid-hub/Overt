# 实时日志系统使用说明

## 概述

这是一个基于C++的实时日志系统，支持Android平台，提供以下功能：

- 实时日志输出到Android logcat
- 多级别日志支持
- 全局开关控制
- 支持格式化字符串

## 主要特性

### 1. 全局开关控制
通过修改 `zLog.h` 中的宏来控制日志系统：

```cpp
// 启用/禁用所有日志
#define ENABLE_LOGGING

// 控制日志级别
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
```

### 2. 支持的日志格式

#### 格式1: 带参数的日志
```cpp
LOGE("[JNI] kpg: %p", kpg);
LOGD("变量值: %d", value);
LOGI("字符串: %s", str);
```

#### 格式2: 不带参数的日志
```cpp
LOGE("[JNI] kpg");
LOGD("调试信息");
LOGW("警告信息");
```

## 日志级别

系统支持5个日志级别：

- `LOGV` - Verbose (详细)
- `LOGD` - Debug (调试)
- `LOGI` - Info (信息)
- `LOGW` - Warn (警告)
- `LOGE` - Error (错误)

## 使用方法

### 1. 基本使用

```cpp
#include "zLog.h"

// 直接使用日志宏
LOGE("[JNI] kpg: %p", kpg);
LOGE("[JNI] kpg");
LOGD("调试信息: %d", 123);
```

### 2. 带自定义标签的日志

```cpp
LOGT("CustomTag", "这是自定义标签的日志");
```

## C接口函数

系统提供了C接口函数，方便在纯C代码中使用：

```cpp
// 日志输出
void zLogPrint(int level, const char* tag, const char* format, ...);
```

## 配置选项

### 1. 全局开关
在 `zLog.h` 中修改：

```cpp
// 启用日志系统
#define ENABLE_LOGGING

// 禁用日志系统（所有日志宏都会变成空操作）
// #undef ENABLE_LOGGING
```

### 2. 日志级别控制
```cpp
// 只显示错误和警告
#define CURRENT_LOG_LEVEL LOG_LEVEL_WARN

// 显示所有日志
#define CURRENT_LOG_LEVEL LOG_LEVEL_VERBOSE
```

### 3. 默认日志标签
```cpp
#define LOG_TAG "Overt"  // 修改默认标签
```

## 性能特性

- **轻量级**: 只输出到logcat，无文件I/O开销
- **高效**: 直接调用Android日志API
- **简单**: 无需初始化或清理

## 注意事项

1. **性能**: 日志输出对性能影响很小
2. **调试**: 可以通过logcat查看所有日志输出
3. **级别**: 合理设置日志级别可以控制输出量

## 示例代码

参考 `log_example.cpp` 文件中的完整使用示例。

## 编译要求

- C++11 或更高版本
- Android NDK
- 需要链接 `log` 库

在 `CMakeLists.txt` 中添加：
```cmake
find_library(log-lib log)
target_link_libraries(your_target ${log-lib})
``` 