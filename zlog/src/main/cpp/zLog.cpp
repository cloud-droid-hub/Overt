#include <stdlib.h>
#include <unistd.h>
#include "zLog.h"

#define MAX_LOG_BUF_LEN 3000
#define MAX_SEGMENT_LEN 3000


void zLogPrint(int level, const char* tag, const char* file_name, const char* function_name, int line_num, const char* format, ...) {
    if(level < CURRENT_LOG_LEVEL) return;

    va_list args;
    va_start(args, format);

    char* buffer = nullptr;
    int len = vasprintf(&buffer, format, args);
    va_end(args);

    if (len <= 0 || !buffer) return;

    for (int i = 0; i < len; i += MAX_SEGMENT_LEN) {
        __android_log_print(level, tag, "[%s][%s][%d]%.*s", file_name, function_name, line_num, MAX_SEGMENT_LEN, buffer + i);
    }
    sleep(0);
    free(buffer);
}

//char* get_thread_log_buffer() {
//    thread_local char* buffer = []() {
//        char* buf = (char*)malloc(MAX_LOG_BUF_LEN);
//        return buf;
//    }();
//    return buffer;
//}
//
//void zLogPrint(int level, const char* tag, const char* file_name, const char* function_name, int line_num, const char* format, ...) {
//    if (level < CURRENT_LOG_LEVEL) return;
//
////    char buffer[MAX_LOG_BUF_LEN] = {};
//
//    char* buffer = get_thread_log_buffer();
//    if (!buffer) return;
//
//    va_list args;
//    va_start(args, format);
//    int len = vsnprintf(buffer, MAX_LOG_BUF_LEN, format, args);
//    va_end(args);
//
//    if (len <= 0) return;
//    if (len >= MAX_LOG_BUF_LEN) len = MAX_LOG_BUF_LEN - 1;  // 防止越界
//
//    for (int i = 0; i < len; i += MAX_SEGMENT_LEN) {
//        int seg_len = (i + MAX_SEGMENT_LEN > len) ? (len - i) : MAX_SEGMENT_LEN;
//        __android_log_print(level, tag, "[%s][%s][%d]%.*s", file_name, function_name, line_num, seg_len, buffer + i);
//    }
//
//    if (len >= MAX_LOG_BUF_LEN - 1) {
//        __android_log_print(level, tag, "[%s][%s][%d][Log truncated]", file_name, function_name, line_num);
//    }
//}
