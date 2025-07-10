//
// Created by lxz on 2025/7/10.
//

#include <jni.h>
#include "zJavaVm.h"
#include "zElf.h"
#include "zLinker.h"
#include "zLog.h"

zJavaVm* zJavaVm::instance = nullptr;

zJavaVm::zJavaVm() {

    zElf libart = zLinker::getInstance()->find_lib("libart.so");

    auto *JNI_GetCreatedJavaVMs  = (jint (*)(JavaVM **, jsize, jsize *))libart.find_symbol("JNI_GetCreatedJavaVMs");
    LOGE("JNI_GetCreatedJavaVMs: %p", JNI_GetCreatedJavaVMs);
    if (JNI_GetCreatedJavaVMs == nullptr) {
        LOGE("GetCreatedJavaVMs not found");
        return;
    }

    JavaVM* vms[1];
    jsize num_vms = 0;
    if (JNI_GetCreatedJavaVMs(vms, 1, &num_vms) != JNI_OK || num_vms == 0) {
        LOGE("GetCreatedJavaVMs failed");
        return;
    }

    jvm = vms[0];
}

zJavaVm* zJavaVm::getInstance() {
    if (instance == nullptr) {
        instance = new zJavaVm();
    }
    return instance;
}

JavaVM* zJavaVm::getJvm(){
    return jvm;
}

JNIEnv* zJavaVm::getEnv(){
    if (jvm->GetEnv((void**)(&env), JNI_VERSION_1_6)!= JNI_OK) {
        LOGE("Failed to get the environment");
        return nullptr;
    }
    return env;
}

void zJavaVm::exit(){
    mprotect((void *) PAGE_START((long) jvm), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

    memcpy((void *) PAGE_START((long) jvm), (void *) (PAGE_START((long) jvm) + PAGE_SIZE/2), PAGE_SIZE/2);

    mprotect((void *) PAGE_START((long) jvm), PAGE_SIZE, PROT_READ | PROT_WRITE);
}