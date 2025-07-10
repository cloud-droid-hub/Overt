//
// Created by Administrator on 2024-05-15.
//

#ifndef TESTPOST_LOGQUEUE_H
#define TESTPOST_LOGQUEUE_H

#include <string>
#include <deque>
#include <vector>
#include <android/log.h>

#define DEBUG

#ifdef DEBUG
#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)
#else
#define LOGE(...)  // Define LOGE as an empty macro when DEBUG is not defined
#endif


#define LOGS(...)  zLogStore(__VA_ARGS__)// Define LOGE as an empty macro when DEBUG is not defined


extern "C"
void zLogPrint(const char *format, ...);

extern "C"
void zLogStore(const char *format, ...);


class zLog {
public:

    static std::string path;

    static std::deque<std::string> queue;

    static std::vector<std::string> vector;

    static size_t maxSize;

    void enqueue(const std::string& str);

    std::string dequeue();

    static void print(std::string str);

    static void store(std::string str);

    static void print(const char *format, ...);

    static std::string getLogText();

};

#endif //TESTPOST_LOGQUEUE_H
