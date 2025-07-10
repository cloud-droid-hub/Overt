//
// Created by Administrator on 2024-05-15.
//


#include "zLog.h"
#include <fstream>
#include <string>
#include <mutex>
#include <regex>
#include <dlfcn.h>
#include <fcntl.h>

size_t zLog::maxSize = 1000;
std::string zLog::path = "";
std::vector<std::string> zLog::vector = std::vector<std::string>();
std::deque<std::string>zLog::queue = std::deque<std::string>();



// 日志文件全局变量
std::ofstream logFile;
// 用于同步的互斥锁
std::mutex logMutex;

void zLog::enqueue(const std::string& str){
}

std::string zLog::dequeue() {
    return "";
}


void zLog::store(std::string logText){
    std::lock_guard<std::mutex> lock(logMutex);// 锁定互斥锁以保证线程安全
    if (zLog::vector.size() == zLog::maxSize) {
        if (!logFile.is_open()) {
            logFile.open(zLog::path.c_str(), std::ios::app);
        }
        for (int i = 0; i < zLog::maxSize; i++){
            logFile << zLog::vector[i] << std::endl; // 写入日志消息
        }
        logFile.flush(); // 确保消息被写入磁盘
        zLog::vector.clear();
    }
    zLog::vector.push_back(logText);
}

void zLog::print(std::string logText){
//    if (zLog::loger == nullptr){
//        zLog::loger = new zLog(100);
//    }

    // 加入队列
    // zLog::loger->enqueue(logText);

    __android_log_print(6,"lxz","%s", logText.c_str());
}

void zLog::print(const char *format, ...){
    va_list args;
    va_start(args, format);

    // 计算格式化后的字符串长度
    int len = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    // 分配足够的内存来存储格式化后的字符串
    char *buffer = (char *)malloc(len);
    if (buffer == NULL) {
        // 内存分配失败，退出函数
        return;
    }

    // 再次初始化可变参数列表
    va_start(args, format);
    vsnprintf(buffer, len, format, args);
    va_end(args);

    zLog::print(std::string(buffer));

    free(buffer);
}


extern "C"
void zLogPrint(const char *format, ...){
    va_list args;
    va_start(args, format);

    // 计算格式化后的字符串长度
    int len = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    // 分配足够的内存来存储格式化后的字符串
    char *buffer = (char *)malloc(len);
    if (buffer == NULL) {
        // 内存分配失败，退出函数
        return;
    }

    // 再次初始化可变参数列表
    va_start(args, format);
    vsnprintf(buffer, len, format, args);
    va_end(args);

    zLog::print(std::string(buffer));

    free(buffer);
}

extern "C"
void zLogStore(const char *format, ...){

    va_list args;
    va_start(args, format);

    // 计算格式化后的字符串长度
    int len = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    // 分配足够的内存来存储格式化后的字符串
    char *buffer = (char *)malloc(len);
    if (buffer == NULL) {
        // 内存分配失败，退出函数
        return;
    }

    // 再次初始化可变参数列表
    va_start(args, format);
    vsnprintf(buffer, len, format, args);
    va_end(args);

    zLog::store(std::string(buffer));

    free(buffer);
}

std::string zLog::getLogText(){
    return "";
}
