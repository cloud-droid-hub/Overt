//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZJAVAVM_H
#define OVERT_ZJAVAVM_H


#include <jni.h>
#include <asm-generic/mman.h>
#include <sys/mman.h>
#include "zLog.h"

#define PAGE_START(x)  ((x) & PAGE_MASK)

class zJavaVm {
private:
    zJavaVm();
    zJavaVm(const zJavaVm&) = delete;
    zJavaVm& operator = (const zJavaVm&) = delete;
    static zJavaVm* instance;
    JavaVM* jvm = nullptr;
    JNIEnv *env = nullptr;

public:

    // Static method to get the singleton instance
    static zJavaVm* getInstance();

    JavaVM* getJvm();

    JNIEnv* getEnv();

    void exit();

};


#endif //OVERT_ZJAVAVM_H
