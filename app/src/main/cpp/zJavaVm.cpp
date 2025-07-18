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


jobject createNewContext(JNIEnv* env) {

    if (env == nullptr) {return nullptr;}

    jclass clsActivityThread = env->FindClass("android/app/ActivityThread");
    jmethodID m_currentAT = env->GetStaticMethodID(clsActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject at = env->CallStaticObjectMethod(clsActivityThread, m_currentAT);

    jfieldID fid_mBoundApp = env->GetFieldID(clsActivityThread, "mBoundApplication", "Landroid/app/ActivityThread$AppBindData;");
    jobject mBoundApp = env->GetObjectField(at, fid_mBoundApp);

    jclass clsAppBindData = env->FindClass("android/app/ActivityThread$AppBindData");
    jfieldID fid_info = env->GetFieldID(clsAppBindData, "info", "Landroid/app/LoadedApk;");
    jobject loadedApk = env->GetObjectField(mBoundApp, fid_info);

    jclass clsLoadedApk = env->FindClass("android/app/LoadedApk");
    jmethodID m_makeApp = env->GetMethodID(clsLoadedApk, "makeApplication", "(ZLandroid/app/Instrumentation;)Landroid/app/Application;");
    jobject app = env->CallObjectMethod(loadedApk, m_makeApp, JNI_FALSE, nullptr);

    // app.getBaseContext()
    jclass clsApp = env->GetObjectClass(app);
    jmethodID m_getBaseContext = env->GetMethodID(clsApp, "getBaseContext", "()Landroid/content/Context;");
    jobject context = env->CallObjectMethod(app, m_getBaseContext);

    // 转为全局引用后返回
    return env->NewGlobalRef(context);
}

jobject getCurrentContext(JNIEnv* env) {

    if (env == nullptr) {return nullptr;}

    jclass activityThread = env->FindClass("android/app/ActivityThread");
    jmethodID currentApp = env->GetStaticMethodID(activityThread, "currentApplication", "()Landroid/app/Application;");
    jobject context = env->CallStaticObjectMethod(activityThread, currentApp);

    if (context == nullptr) {
        LOGE("Failed to get the context");
        return nullptr;
    }

    // 转为全局引用后返回
    return env->NewGlobalRef(context);
}

jobject zJavaVm::getContext(){
    if (context == nullptr){
        context = createNewContext(getEnv());
    }
    return context;
}

void zJavaVm::exit(){
    mprotect((void *) PAGE_START((long) jvm), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

    memcpy((void *) PAGE_START((long) jvm), (void *) (PAGE_START((long) jvm) + PAGE_SIZE/2), PAGE_SIZE/2);

    mprotect((void *) PAGE_START((long) jvm), PAGE_SIZE, PROT_READ | PROT_WRITE);
}