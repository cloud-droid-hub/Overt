//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZJAVAVM_H
#define OVERT_ZJAVAVM_H

#include <jni.h>
#include <asm-generic/mman.h>
#include <sys/mman.h>

#include "zLog.h"
#include "config.h"

#define PAGE_START(x)  ((x) & PAGE_MASK)

class zJavaVm {
private:
    zJavaVm();
    zJavaVm(const zJavaVm&) = delete;
    zJavaVm& operator = (const zJavaVm&) = delete;
    static zJavaVm* instance;
    JavaVM* jvm = nullptr;
    JNIEnv *env = nullptr;
    jobject context = nullptr;
    jobject custom_context = nullptr;
    jobject class_loader = nullptr;

public:

    // Static method to get the singleton instance
    static zJavaVm* getInstance();

    // 只能在主线程调用
    void init();

    JavaVM* getJvm();

    JNIEnv* getEnv();

    jobject getContext();

    jobject getClassLoader();

    jclass findClass(const char* className);

    void exit();

};


#endif //OVERT_ZJAVAVM_H
