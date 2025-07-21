//
// Created by Administrator on 2024-05-15.
//

#ifndef TESTPOST_LOGQUEUE_H
#define TESTPOST_LOGQUEUE_H



//#include "config.h"

#define DEBUG


#define LOGS(...)  zLogStore(__VA_ARGS__)// Define LOGE as an empty macro when DEBUG is not defined


//extern "C" void zLogPrint(const char *format, ...);
//
//extern "C" void zLogStore(const char *format, ...);

#ifdef DEBUG
#define LOGT(...) zLogPrint(__VA_ARGS__)
#else
#define LOGT(...)
#endif

#ifdef DEBUG
#define LOGE(...)  // Define LOGE as an empty macro when DEBUG is not defined
//#define LOGE(...)  zLogPrint(__VA_ARGS__);
//#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "lxz", __VA_ARGS__)
#else
#define LOGE(...)  // Define LOGE as an empty macro when DEBUG is not defined
#endif

//
//class zLog {
//public:
//
//    static string path;
//
//    static vector<string> vector;
//
//    static size_t maxSize;
//
//    void enqueue(const string& str);
//
//    string dequeue();
//
//    static void print(string str);
//
//    static void store(string str);
//
//    static void print(const char *format, ...);
//
//    static string getLogText();
//
//};

#endif //TESTPOST_LOGQUEUE_H
