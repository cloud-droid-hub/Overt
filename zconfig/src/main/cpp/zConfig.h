#ifndef CONFIG_H
#define CONFIG_H

// 通过这个宏让模块宏接受配置宏的控制
#define ZCONFIG_ENABLE 1

// 通过这个宏来控制标准 API 和 非标准 API 的使用
#define ZCONFIG_ENABLE_NONSTD_API 1

// 全局日志开关 - 可以通过修改这个宏来控制日志输出
#define ZCONFIG_ENABLE_LOGGING 1

// 日志标签
#define LOG_TAG "Overt"

// 日志级别定义
#define LOG_LEVEL_VERBOSE 2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_INFO    4
#define LOG_LEVEL_WARN    5
#define LOG_LEVEL_ERROR   6

// 当前日志级别 - 可以通过修改这个值来控制日志输出级别
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

#endif // CONFIG_H