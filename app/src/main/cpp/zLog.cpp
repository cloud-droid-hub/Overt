//
// Created by Administrator on 2024-05-15.
//

#include "zLog.h"
#include "nonstd_libc.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

// 日志级别对应的Android日志级别
static const int android_log_levels[] = {
    ANDROID_LOG_VERBOSE,  // LOG_LEVEL_VERBOSE
    ANDROID_LOG_DEBUG,    // LOG_LEVEL_DEBUG
    ANDROID_LOG_INFO,     // LOG_LEVEL_INFO
    ANDROID_LOG_WARN,     // LOG_LEVEL_WARN
    ANDROID_LOG_ERROR     // LOG_LEVEL_ERROR
};

// C接口函数实现
extern "C" void zLogPrint(int level, const char* tag, const char* format, ...) {
    // 检查日志级别
    if (level < CURRENT_LOG_LEVEL) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // 计算格式化后的字符串长度
    int len = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);
    
    if (len <= 0) {
        return;
    }
    
    // 分配足够的内存来存储格式化后的字符串
    char* buffer = (char*)nonstd_malloc(len);
    if (buffer == NULL) {
        return;
    }
    
    // 再次初始化可变参数列表
    va_start(args, format);
    vsnprintf(buffer, len, format, args);
    va_end(args);
    
    // 输出到Android日志
    if (level < sizeof(android_log_levels) / sizeof(android_log_levels[0])) {
        __android_log_print(android_log_levels[level], tag, "%s", buffer);
        sleep(0);  // 等待Android日志输出完毕
    }
    
    nonstd_free(buffer);
}
