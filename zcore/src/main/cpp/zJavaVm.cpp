//
// Created by lxz on 2025/7/10.
//

#include <jni.h>

#include "zElf.h"
#include "zLinker.h"
#include "zLog.h"
#include "zLibc.h"
#include "zJavaVm.h"
#include "syscall.h"

// 静态单例实例指针
zJavaVm* zJavaVm::instance = nullptr;

/**
 * zJavaVm构造函数
 * 初始化Java虚拟机实例，通过动态链接库获取JVM指针
 * 使用ELF解析技术从libart.so中获取JNI_GetCreatedJavaVMs函数
 */
zJavaVm::zJavaVm() {
    LOGD("Constructor called");

    // 通过链接器查找libart.so库
    zElf libart = zLinker::getInstance()->find_lib("libart.so");

    // 从libart.so中获取JNI_GetCreatedJavaVMs函数指针
    auto *JNI_GetCreatedJavaVMs  = (jint (*)(JavaVM **, jsize, jsize *))libart.find_symbol("JNI_GetCreatedJavaVMs");
    LOGI("JNI_GetCreatedJavaVMs: %p", JNI_GetCreatedJavaVMs);
    if (JNI_GetCreatedJavaVMs == nullptr) {
        LOGE("GetCreatedJavaVMs not found");
        return;
    }

    // 获取已创建的Java虚拟机实例
    JavaVM* vms[1];
    jsize num_vms = 0;
    if (JNI_GetCreatedJavaVMs(vms, 1, &num_vms) != JNI_OK || num_vms == 0) {
        LOGE("GetCreatedJavaVMs failed");
        return;
    }

    // 保存JVM实例指针
    jvm = vms[0];
    LOGI("JVM initialized successfully");
}






/**
 * 获取Java虚拟机指针
 * @return JavaVM指针
 */
JavaVM* zJavaVm::getJvm(){
    LOGD("getJvm called");
    return jvm;
}

/**
 * 获取JNI环境指针
 * 将当前线程附加到JVM并获取JNI环境，支持多线程安全
 * @return JNIEnv指针，失败时返回nullptr
 */
JNIEnv* zJavaVm::getEnv(){
    LOGI("getEnv called");
    if(jvm == nullptr){
        LOGE("JVM is not initialized");
        return nullptr;
    }

    // 获取当前线程ID

    int tid = __syscall0(SYS_gettid);

    // 先检查是否已经有当前线程的JNIEnv
    {
        std::lock_guard<std::mutex> lock(env_map_mutex);
        auto it = thread_env_map.find(tid);
        if (it != thread_env_map.end()) {
            LOGI("getEnv: Found existing JNIEnv for thread %p", it->second);
            return it->second;
        }
    }
    
    // 当前线程没有JNIEnv，需要创建新的
    JNIEnv* env = nullptr;
    if (jvm->AttachCurrentThread((JNIEnv **) &env, nullptr) != JNI_OK) {
        LOGE("Failed to attach current thread to JVM");
        return nullptr;
    }
    
    // 将新的JNIEnv添加到映射表中
    {
        std::lock_guard<std::mutex> lock(env_map_mutex);
        thread_env_map[tid] = env;
        LOGI("getEnv: Created new JNIEnv %p for thread %d", env, tid);
    }
    
    return env;
}

/**
 * 创建新的Android上下文对象
 * 通过反射机制从ActivityThread中获取Application实例并创建Context
 * @param env JNI环境指针
 * @return 全局引用的Context对象，失败时返回nullptr
 */
jobject createNewContext(JNIEnv* env) {
    LOGD("createNewContext called");
    if (env == nullptr) {
        LOGD("createNewContext: env is null");
        return nullptr;
    }

    // 步骤1: 获取ActivityThread实例
    jclass clsActivityThread = env->FindClass("android/app/ActivityThread");
    if (clsActivityThread == nullptr) {
        LOGE("Failed to find ActivityThread class");
        return nullptr;
    }
    
    // 获取currentActivityThread静态方法
    jmethodID m_currentAT = env->GetStaticMethodID(clsActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    if (m_currentAT == nullptr) {
        LOGE("Failed to find currentActivityThread method");
        return nullptr;
    }
    
    // 调用静态方法获取当前ActivityThread实例
    jobject at = env->CallStaticObjectMethod(clsActivityThread, m_currentAT);
    if (at == nullptr) {
        LOGE("Failed to get current ActivityThread");
        return nullptr;
    }

    // 步骤2: 获取mBoundApplication字段
    jfieldID fid_mBoundApp = env->GetFieldID(clsActivityThread, "mBoundApplication", "Landroid/app/ActivityThread$AppBindData;");
    if (fid_mBoundApp == nullptr) {
        LOGE("Failed to find mBoundApplication field");
        env->DeleteLocalRef(at);
        return nullptr;
    }
    
    // 获取mBoundApplication对象
    jobject mBoundApp = env->GetObjectField(at, fid_mBoundApp);
    if (mBoundApp == nullptr) {
        LOGE("Failed to get mBoundApplication");
        env->DeleteLocalRef(at);
        return nullptr;
    }

    // 步骤3: 获取LoadedApk信息
    jclass clsAppBindData = env->FindClass("android/app/ActivityThread$AppBindData");
    if (clsAppBindData == nullptr) {
        LOGE("Failed to find AppBindData class");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        return nullptr;
    }
    
    // 获取info字段（LoadedApk对象）
    jfieldID fid_info = env->GetFieldID(clsAppBindData, "info", "Landroid/app/LoadedApk;");
    if (fid_info == nullptr) {
        LOGE("Failed to find info field");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        return nullptr;
    }
    
    // 获取LoadedApk对象
    jobject loadedApk = env->GetObjectField(mBoundApp, fid_info);
    if (loadedApk == nullptr) {
        LOGE("Failed to get LoadedApk");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        return nullptr;
    }

    // 步骤4: 创建Application实例
    jclass clsLoadedApk = env->FindClass("android/app/LoadedApk");
    if (clsLoadedApk == nullptr) {
        LOGE("Failed to find LoadedApk class");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        return nullptr;
    }
    
    // 获取makeApplication方法
    jmethodID m_makeApp = env->GetMethodID(clsLoadedApk, "makeApplication", "(ZLandroid/app/Instrumentation;)Landroid/app/Application;");
    if (m_makeApp == nullptr) {
        LOGE("Failed to find makeApplication method");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        return nullptr;
    }
    
    // 调用makeApplication创建Application实例
    jobject app = env->CallObjectMethod(loadedApk, m_makeApp, JNI_FALSE, nullptr);
    if (app == nullptr) {
        LOGE("Failed to create Application instance");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        return nullptr;
    }

    // 步骤5: 获取Application的base context
    jclass clsApp = env->GetObjectClass(app);
    if (clsApp == nullptr) {
        LOGE("Failed to get Application class");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        env->DeleteLocalRef(app);
        return nullptr;
    }
    
    // 获取getBaseContext方法
    jmethodID m_getBaseContext = env->GetMethodID(clsApp, "getBaseContext", "()Landroid/content/Context;");
    if (m_getBaseContext == nullptr) {
        LOGE("Failed to find getBaseContext method");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        env->DeleteLocalRef(app);
        return nullptr;
    }
    
    // 调用getBaseContext获取Context对象
    jobject context = env->CallObjectMethod(app, m_getBaseContext);
    if (context == nullptr) {
        LOGE("Failed to get base context");
        env->DeleteLocalRef(at);
        env->DeleteLocalRef(mBoundApp);
        env->DeleteLocalRef(loadedApk);
        env->DeleteLocalRef(app);
        return nullptr;
    }

    // 步骤6: 初始化Application（如果需要）
    // 检查Application是否已经初始化
    jmethodID m_onCreate = env->GetMethodID(clsApp, "onCreate", "()V");
    if (m_onCreate != nullptr) {
        // 可以在这里调用onCreate如果需要的话
        // env->CallVoidMethod(app, m_onCreate);
        LOGD("Application onCreate method found");
    }

    // 步骤7: 设置Application到ActivityThread
    // 注意：当代码执行时机比较早的时候，如果不进行绑定，TEE检查时获取证书会失败！
    jfieldID fid_mInitialApplication = env->GetFieldID(clsActivityThread, "mInitialApplication", "Landroid/app/Application;");
    if (fid_mInitialApplication != nullptr) {
        jobject currentInitialApp = env->GetObjectField(at, fid_mInitialApplication);
        if (currentInitialApp == nullptr) {
            // 只有当mInitialApplication为空时才设置
            env->SetObjectField(at, fid_mInitialApplication, app);
            LOGD("Set mInitialApplication");
        }
        if (currentInitialApp != nullptr) {
            env->DeleteLocalRef(currentInitialApp);
        }
    }

    // 步骤8: 清理局部引用，避免内存泄漏
    env->DeleteLocalRef(at);
    env->DeleteLocalRef(mBoundApp);
    env->DeleteLocalRef(loadedApk);
    env->DeleteLocalRef(app);
    env->DeleteLocalRef(clsApp);

    // 步骤9: 转为全局引用后返回，确保跨线程使用安全
    jobject globalContext = env->NewGlobalRef(context);
    env->DeleteLocalRef(context);
    
    LOGI("Successfully created new context");
    return globalContext;
}

/**
 * 获取当前应用的Context对象
 * 通过ActivityThread.currentApplication()获取当前Application实例
 * @param env JNI环境指针
 * @return 全局引用的Context对象，失败时返回nullptr
 */
jobject getCurrentContext(JNIEnv* env) {
    LOGD("getCurrentContext called");
    if (env == nullptr) {
        LOGD("getCurrentContext: env is null");
        return nullptr;
    }

    // 获取ActivityThread类
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    // 获取currentApplication静态方法
    jmethodID currentApp = env->GetStaticMethodID(activityThread, "currentApplication", "()Landroid/app/Application;");
    // 调用静态方法获取当前Application
    jobject context = env->CallStaticObjectMethod(activityThread, currentApp);

    if (context == nullptr) {
        LOGE("Failed to get the context");
        return nullptr;
    }

    // 转为全局引用后返回，确保跨线程使用安全
    return env->NewGlobalRef(context);
}

/**
 * 获取应用的类加载器
 * 通过Context对象获取ClassLoader实例
 * @param env JNI环境指针
 * @param context Android上下文对象
 * @return 全局引用的ClassLoader对象，失败时返回nullptr
 */
jobject getAppClassLoader(JNIEnv* env, jobject context) {
    LOGD("getAppClassLoader called");
    if (context == nullptr) {
        LOGD("getAppClassLoader: context is null");
        return nullptr;
    }

    // 获取Context类
    jclass ctxCls = env->GetObjectClass(context);
    // 获取getClassLoader方法
    jmethodID getClassLoader = env->GetMethodID(
        ctxCls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    // 调用方法获取ClassLoader
    jobject loader = env->CallObjectMethod(context, getClassLoader);

    // 返回全局引用，调用者如果需要长期持有请转GlobalRef
    return env->NewGlobalRef(loader);
}

/**
 * 通过类加载器加载指定类
 * 使用ClassLoader.loadClass()方法动态加载类
 * @param env JNI环境指针
 * @param classLoader 类加载器对象
 * @param className 类名（全限定名）
 * @return 加载的类对象，失败时返回nullptr
 */
jclass loadClassFromLoader(JNIEnv* env, jobject classLoader, const char* className) {
    LOGD("loadClassFromLoader called with className: %s", className);
    if (classLoader == nullptr || className == nullptr) {
        LOGD("loadClassFromLoader: classLoader or className is null");
        return nullptr;
    }

    // 获取ClassLoader类
    jclass loaderCls = env->FindClass("java/lang/ClassLoader");
    // 获取loadClass方法
    jmethodID loadClass = env->GetMethodID(
        loaderCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    // 创建类名字符串
    jstring jName = env->NewStringUTF(className);
    // 调用loadClass方法加载类
    jclass clazz = (jclass) env->CallObjectMethod(classLoader, loadClass, jName);
    // 清理局部引用
    env->DeleteLocalRef(jName);

    return clazz;   // 调用者决定是否提升为GlobalRef
}

/**
 * 获取Android上下文对象
 * 优先使用当前Context，失败时创建新的Context
 * @return 全局引用的Context对象
 */
jobject zJavaVm::getContext() {
    LOGD("getContext called");
    if (getEnv() == nullptr) {
        LOGE("Failed to get the environment");
        return nullptr;
    }
    
    if (context == nullptr) {
        // 尝试获取当前Context
        context = getCurrentContext(getEnv());
        if (context == nullptr) {
            // 如果获取失败，则尝试创建自定义Context，并缓存
            if (custom_context == nullptr) {
                custom_context = createNewContext(getEnv());
            }
            return custom_context;
        } else if (custom_context != nullptr) {
            // 如果context恢复可用，释放custom_context
            getEnv()->DeleteGlobalRef(custom_context);
            custom_context = nullptr;
        }
    }
    return context;
}

/**
 * 获取应用的类加载器
 * 缓存ClassLoader实例，避免重复获取
 * @return 全局引用的ClassLoader对象
 */
jobject zJavaVm::getClassLoader(){
    LOGD("getClassLoader called");
    if(class_loader == nullptr){
        class_loader = getAppClassLoader(getEnv(), getContext());
    }
    return class_loader;
}

/**
 * 在子线程中获取非系统类
 * 子线程中获取非系统类必须通过这个方法，使用应用的ClassLoader
 * @param className 类名（全限定名）
 * @return 加载的类对象，失败时返回nullptr
 */
jclass zJavaVm::findClass(const char* className){
    LOGD("findClass called with className: %s", className);
    return loadClassFromLoader(getEnv(), getClassLoader(), className);
}

/**
 * 清理当前线程的JNIEnv
 * 在线程退出时调用，避免内存泄漏
 */
void zJavaVm::cleanupCurrentThreadEnv(){
    LOGD("cleanupCurrentThreadEnv called");
    if(jvm == nullptr){
        LOGD("cleanupCurrentThreadEnv: JVM is not initialized");
        return;
    }
    
    // 获取当前线程ID
    int tid = __syscall0(SYS_gettid);
    
    // 从映射表中移除当前线程的JNIEnv
    {
        std::lock_guard<std::mutex> lock(env_map_mutex);
        auto it = thread_env_map.find(tid);
        if (it != thread_env_map.end()) {
            LOGI("cleanupCurrentThreadEnv: Cleaning up JNIEnv %p for thread %d", it->second, tid);
            thread_env_map.erase(it);
        }
    }
    
    // 从JVM中分离当前线程
    jvm->DetachCurrentThread();
    LOGI("cleanupCurrentThreadEnv: Thread detached from JVM");
}

/**
 * 退出JVM
 * 通过内存操作使JVM退出，用于清理资源
 */
void zJavaVm::exit(){
    LOGD("exit called");
    
    // 清理所有线程的JNIEnv
    {
        std::lock_guard<std::mutex> lock(env_map_mutex);
        for (auto& pair : thread_env_map) {
            LOGI("exit: Cleaning up JNIEnv %p for thread %d", pair.second, pair.first);
        }
        thread_env_map.clear();
    }
    
    // 修改内存页权限为可读写执行
    mprotect((void *) PAGE_START((long) jvm), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

    // 复制内存页内容
    memcpy((void *) PAGE_START((long) jvm), (void *) (PAGE_START((long) jvm) + PAGE_SIZE/2), PAGE_SIZE/2);

    // 恢复内存页权限为可读写
    mprotect((void *) PAGE_START((long) jvm), PAGE_SIZE, PROT_READ | PROT_WRITE);
}