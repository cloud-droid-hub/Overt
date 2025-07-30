//
// Created by Administrator on 2024-05-15.
//

#ifndef TESTPOST_LOGQUEUE_H
#define TESTPOST_LOGQUEUE_H

#include <string>
#include <android/log.h>

// 全局日志开关 - 可以通过修改这个宏来控制日志输出
#define ENABLE_LOGGING

// 日志级别定义
#define LOG_LEVEL_VERBOSE 0
#define LOG_LEVEL_DEBUG   1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_WARN    3
#define LOG_LEVEL_ERROR   4

// 当前日志级别 - 可以通过修改这个值来控制日志输出级别
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

// 日志标签
#define LOG_TAG "Overt"

// 日志宏定义
#ifdef ENABLE_LOGGING
    #define LOGV(...) zLogPrint(LOG_LEVEL_VERBOSE, LOG_TAG, __VA_ARGS__)
    #define LOGD(...) zLogPrint(LOG_LEVEL_DEBUG, LOG_TAG, __VA_ARGS__)
    #define LOGI(...) zLogPrint(LOG_LEVEL_INFO, LOG_TAG, __VA_ARGS__)
    #define LOGW(...) zLogPrint(LOG_LEVEL_WARN, LOG_TAG, __VA_ARGS__)
    #define LOGE(...) zLogPrint(LOG_LEVEL_ERROR, LOG_TAG, __VA_ARGS__)
    
    // 带标签的日志
    #define LOGT(tag, ...) zLogPrint(LOG_LEVEL_DEBUG, tag, __VA_ARGS__)
#else
    #define LOGV(...)
    #define LOGD(...)
    #define LOGI(...)
    #define LOGW(...)
    #define LOGE(...)
    #define LOGT(...)
#endif

// 兼容旧的DEBUG宏
#ifdef DEBUG
    #define LOGT_OLD(...) zLogPrint(LOG_LEVEL_DEBUG, LOG_TAG, __VA_ARGS__)
#else
    #define LOGT_OLD(...)
#endif

// C接口函数声明
extern "C" {
    void zLogPrint(int level, const char* tag, const char* format, ...);
}

#endif //TESTPOST_LOGQUEUE_H
