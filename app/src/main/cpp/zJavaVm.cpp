//
// Created by lxz on 2025/7/10.
//

#include <jni.h>
#include "zJavaVm.h"
#include "zElf.h"
#include "zLinker.h"
#include "zLog.h"
#include "nonstd_libc.h"

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

    // 1. 获取 ActivityThread 实例
    jclass clsActivityThread = env->FindClass("android/app/ActivityThread");
    if (clsActivityThread == nullptr) {
        LOGE("Failed to find ActivityThread class");
        return nullptr;
    }
    
    jmethodID m_currentAT = env->GetStaticMethodID(clsActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    if (m_currentAT == nullptr) {
        LOGE("Failed to find currentActivityThread method");
        return nullptr;
    }
    
    jobject at = env->CallStaticObjectMethod(clsActivityThread, m_currentAT);
    if (at == nullptr) {
        LOGE("Failed to get current ActivityThread");
        return nullptr;
    }

    // 2. 获取 mBoundApplication 字段
    jfieldID fid_mBoundApp = env->GetFieldID(clsActivityThread, "mBoundApplication", "Landroid/app/ActivityThread$AppBindData;");
    if (fid_mBoundApp == nullptr) {
        LOGE("Failed to find mBoundApplication field");
        env->DeleteLocalRef(at);
        return nullptr;
    }
    
    jobject mBoundApp = env->GetObjectField(at, fid_mBoundApp);
    if (mBoundApp == nullptr) {
        LOGE("Failed to get mBoundApplication");
        env->DeleteLocalRef(at);
        return nullptr;
    }

    // 3. 获取 LoadedApk 信息
    jclass clsAppBindData = env->FindClass("android/app/ActivityThread$AppBindData");
    if (clsAppBindData == nullptr) {
        LOGE("Failed to find AppBindData class");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        return nullptr;
    }
    
    jfieldID fid_info = env->GetFieldID(clsAppBindData, "info", "Landroid/app/LoadedApk;");
    if (fid_info == nullptr) {
        LOGE("Failed to find info field");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        return nullptr;
    }
    
    jobject loadedApk = env->GetObjectField(mBoundApp, fid_info);
    if (loadedApk == nullptr) {
        LOGE("Failed to get LoadedApk");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        return nullptr;
    }

    // 4. 创建 Application 实例
    jclass clsLoadedApk = env->FindClass("android/app/LoadedApk");
    if (clsLoadedApk == nullptr) {
        LOGE("Failed to find LoadedApk class");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        return nullptr;
    }
    
    jmethodID m_makeApp = env->GetMethodID(clsLoadedApk, "makeApplication", "(ZLandroid/app/Instrumentation;)Landroid/app/Application;");
    if (m_makeApp == nullptr) {
        LOGE("Failed to find makeApplication method");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        return nullptr;
    }
    
    jobject app = env->CallObjectMethod(loadedApk, m_makeApp, JNI_FALSE, nullptr);
    if (app == nullptr) {
        LOGE("Failed to create Application instance");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        return nullptr;
    }

    // 5. 获取 Application 的 base context
    jclass clsApp = env->GetObjectClass(app);
    if (clsApp == nullptr) {
        LOGE("Failed to get Application class");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        env->DeleteLocalRef(app);
        return nullptr;
    }
    
    jmethodID m_getBaseContext = env->GetMethodID(clsApp, "getBaseContext", "()Landroid/content/Context;");
    if (m_getBaseContext == nullptr) {
        LOGE("Failed to find getBaseContext method");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        env->DeleteLocalRef(app);
        return nullptr;
    }
    
    jobject context = env->CallObjectMethod(app, m_getBaseContext);
    if (context == nullptr) {
        LOGE("Failed to get base context");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        env->DeleteLocalRef(app);
        return nullptr;
    }

    // 6. 初始化 Application（如果需要）
    // 检查 Application 是否已经初始化
    jmethodID m_onCreate = env->GetMethodID(clsApp, "onCreate", "()V");
    if (m_onCreate != nullptr) {
        // 可以在这里调用 onCreate 如果需要的话
        // env->CallVoidMethod(app, m_onCreate);
        LOGD("Application onCreate method found");
    }

    // 7. 设置 Application 到 ActivityThread（可选）
    jfieldID fid_mInitialApplication = env->GetFieldID(clsActivityThread, "mInitialApplication", "Landroid/app/Application;");
    if (fid_mInitialApplication != nullptr) {
        jobject currentInitialApp = env->GetObjectField(at, fid_mInitialApplication);
        if (currentInitialApp == nullptr) {
            // 只有当 mInitialApplication 为空时才设置
            env->SetObjectField(at, fid_mInitialApplication, app);
            LOGD("Set mInitialApplication");
        }
        if (currentInitialApp != nullptr) {
            env->DeleteLocalRef(currentInitialApp);
        }
    }

    // 8. 清理局部引用
    env->DeleteLocalRef(at);
    env->DeleteLocalRef(mBoundApp);
    env->DeleteLocalRef(loadedApk);
    env->DeleteLocalRef(app);
    env->DeleteLocalRef(clsApp);

    // 9. 转为全局引用后返回
    jobject globalContext = env->NewGlobalRef(context);
    env->DeleteLocalRef(context);
    
    LOGD("Successfully created new context");
    return globalContext;
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

    nonstd_memcpy((void *) PAGE_START((long) jvm), (void *) (PAGE_START((long) jvm) + PAGE_SIZE/2), PAGE_SIZE/2);

    mprotect((void *) PAGE_START((long) jvm), PAGE_SIZE, PROT_READ | PROT_WRITE);
}