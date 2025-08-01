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
    #define LOGV(...) zLogPrint(LOG_LEVEL_VERBOSE, LOG_TAG, __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #define LOGD(...) zLogPrint(LOG_LEVEL_DEBUG, LOG_TAG, __FILE_NAME__, __FUNCTION__,__LINE__, ##__VA_ARGS__)
    #define LOGI(...) zLogPrint(LOG_LEVEL_INFO, LOG_TAG, __FILE_NAME__, __FUNCTION__,__LINE__, ##__VA_ARGS__)
    #define LOGW(...) zLogPrint(LOG_LEVEL_WARN, LOG_TAG, __FILE_NAME__, __FUNCTION__,__LINE__, ##__VA_ARGS__)
    #define LOGE(...) zLogPrint(LOG_LEVEL_ERROR, LOG_TAG, __FILE_NAME__, __FUNCTION__,__LINE__, ##__VA_ARGS__)

#else
    #define LOGV(...)
    #define LOGD(...)
    #define LOGI(...)
    #define LOGW(...)
    #define LOGE(...)
#endif


void zLogPrint(int level, const char* tag, const char* file_name, const char* function_name, int line_num, const char* format, ...);


#endif //TESTPOST_LOGQUEUE_H
