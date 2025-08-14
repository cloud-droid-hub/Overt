#include <jni.h>

#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zFile.h"
#include "zLinker.h"
#include "zJavaVm.h"
#include "zJson.h"
#include "zBroadCast.h"
#include "zManager.h"

#include "zRootFileInfo.h"
#include "zMountsInfo.h"
#include "zSystemPropInfo.h"
#include "zLinkerInfo.h"
#include "zTimeInfo.h"
#include "zPackageInfo.h"
#include "zClassLoaderInfo.h"
#include "zSystemSettingInfo.h"
#include "zMapsInfo.h"
#include "zTaskInfo.h"
#include "zPortInfo.h"
#include "zTeeInfo.h"
#include "zSslInfo.h"
#include "zLocalNetworkInfo.h"
#include "zThread.h"
#include "zLogcatInfo.h"


// 定义线程睡眠时间常量（秒）
#define OVERT_SLEEP_TIME 10

// 0 zConfig
// 1 zLog															        依赖等级 0
// 2 zLibc zLibcUtil											            依赖等级 0、1
// 3 zStdString zStdVector zStdMap zStdUtil					                依赖等级 0、1、2
// 4 zFile zHttps zCrc zTee zJson zJavaVm zElf zClassLoader zBroadCast		依赖等级 0、1、2、3
// 5、maps_info mounts_info	package_info ...								依赖等级 0、1、2、3、4
// 6、device_info

/**
 * 主要的设备信息收集线程函数
 * 该函数在后台持续运行，定期收集各种设备信息并更新到设备信息管理器
 * @param arg 线程参数（未使用）
 * @return 线程返回值（始终为nullptr）
 */
void* overt_thread(void* arg) {
    LOGD("overt_thread started");

    while (true) {
        LOGI("thread_func: processing device info updates");

        // 绑定到当前空闲的大核，提高性能
        zManager::getInstance()->bind_self_to_least_used_big_core();

        // 提升线程优先级，确保信息收集的及时性
        zManager::getInstance()->raise_thread_priority();

        // 记录开始时间，用于计算执行耗时
        time_t start_time = time(nullptr);

        // 收集SSL信息 - 检测SSL证书异常
        zManager::getInstance()->update_device_info("ssl_info", get_ssl_info());

        // 收集本地网络信息 - 检测同一网络中的其他Overt设备
        zManager::getInstance()->update_device_info("local_network_info", get_local_network_info());

        // 收集任务信息 - 检测Frida等调试工具注入的进程
        zManager::getInstance()->update_device_info("task_info", get_task_info());

        // 收集内存映射信息 - 检测关键系统库是否被篡改
        zManager::getInstance()->update_device_info("maps_info", get_maps_info());

        // 收集Root文件信息 - 检测Root相关文件
        zManager::getInstance()->update_device_info("root_file_info", get_root_file_info());

        // 收集挂载点信息 - 检测异常的文件系统挂载
        zManager::getInstance()->update_device_info("mounts_info", get_mounts_info());

        // 收集系统属性信息 - 检测系统配置异常
        zManager::getInstance()->update_device_info("system_prop_info", get_system_prop_info());

        // 收集链接器信息 - 检测动态链接库加载异常
        zManager::getInstance()->update_device_info("linker_info", get_linker_info());

        // 收集端口信息 - 检测网络端口异常
        zManager::getInstance()->update_device_info("port_info", get_port_info());

        // 收集类加载器信息 - 检测Java层异常
        zManager::getInstance()->update_device_info("class_loader_info", get_class_loader_info());

        // 收集包信息 - 检测已安装应用异常
        zManager::getInstance()->update_device_info("package_info", get_package_info());

        // 收集系统设置信息 - 检测系统设置异常
        zManager::getInstance()->update_device_info("system_setting_info", get_system_setting_info());

        // 收集TEE信息 - 检测可信执行环境异常
        zManager::getInstance()->update_device_info("tee_info", get_tee_info());

        // 收集时间信息 - 检测系统时间异常
        zManager::getInstance()->update_device_info("time_info", get_time_info());


        zManager::getInstance()->update_device_info("logcat_info", get_logcat_info());

        // 通知Java层更新设备信息
        JNIEnv *env = zJavaVm::getInstance()->getEnv();

        if(env == nullptr){
            LOGE("thread_func: env is null");
            continue;
        }

        // 查找MainActivity类
        jclass activity_class = zJavaVm::getInstance()->findClass("com/example/overt/MainActivity");
        if (activity_class == nullptr) {
            LOGE("thread_func: activity_class is null");
            continue;
        }

        // 获取Java层的回调方法ID
        jmethodID method_id = env->GetStaticMethodID(activity_class, "onDeviceInfoUpdated", "()V");
        if (method_id == nullptr){
            LOGE("thread_func: method_id onDeviceInfoUpdated is null");
            continue;
        }

        // 调用Java层方法通知信息更新完成
        LOGI("thread_func: calling onDeviceInfoUpdated");
        env->CallStaticVoidMethod(activity_class, method_id);

        // 计算执行耗时
        time_t end_time = time(nullptr);

        LOGE("end_time - start_time %ld", end_time - start_time);
        
        // 根据执行时间调整睡眠时间，确保固定的执行间隔
        sleep(OVERT_SLEEP_TIME > (end_time - start_time) ? OVERT_SLEEP_TIME - (end_time - start_time) : 0);
    }
    return nullptr;
}



void __attribute__((constructor)) init_(void){
    LOGI("init_ start");

    // 创建设备信息收集线程
    pthread_t tid;
    if (pthread_create(&tid, nullptr, overt_thread, nullptr) != 0) {
        LOGE("pthread_create failed");
        return;
    }

    LOGI("init_ over");
}

/**
 * JNI库加载时的回调函数
 * @param vm Java虚拟机指针
 * @param reserved 保留参数
 * @return JNI版本号
 */
extern "C" JNIEXPORT
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad called");
    return JNI_VERSION_1_6;
}


// 将 map<string, string> 转换为 Java Map<String, String>
jobject cmap_to_jmap(JNIEnv *env, const map<string, string>& cmap){
    LOGD("cmap_to_jmap called, map size: %zu", cmap.size());
    LOGI("cmap_to_jmap: starting conversion, map size=%zu", cmap.size());
    LOGD("cmap_to_jmap: map type info - is nonstd::map: %s", typeid(cmap).name());

    // 遍历并打印map内容
    LOGD("cmap_to_jmap: map contents:");
    for (const auto &entry : cmap) {
        LOGD("cmap_to_jmap: key='%s', value='%s'", entry.first.c_str(), entry.second.c_str());
    }

    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGD("cmap_to_jmap: Failed to find HashMap class");
        return nullptr;
    }

    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGD("cmap_to_jmap: Failed to find HashMap constructor");
        return nullptr;
    }

    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGD("cmap_to_jmap: Failed to find HashMap put method");
        return nullptr;
    }

    // 创建HashMap对象
    jobject jobjectMap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jobjectMap) {
        LOGD("cmap_to_jmap: Failed to create HashMap object");
        return nullptr;
    }

    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGD("cmap_to_jmap: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    // 填充数据
    for (const auto &entry : cmap) {
        LOGD("cmap_to_jmap: processing entry: key='%s', value='%s'",
             entry.first.c_str(), entry.second.c_str());

        jstring jkey = env->NewStringUTF(entry.first.c_str());
        jstring jvalue = env->NewStringUTF(entry.second.c_str());

        if (!jkey || !jvalue) {
            LOGD("cmap_to_jmap: Failed to create string objects");
            if (jkey) env->DeleteLocalRef(jkey);
            if (jvalue) env->DeleteLocalRef(jvalue);
            continue;
        }

        env->CallObjectMethod(jobjectMap, putMethod, jkey, jvalue);

        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGD("cmap_to_jmap: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }

        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }

    LOGI("cmap_to_jmap: conversion completed successfully");
    return jobjectMap;
}

// 将 map<string, map<string, string>> 转换为 Java Map<String, Map<String, String>>
jobject cmap_to_jmap_nested(JNIEnv* env, const map<string, map<string, string>>& cmap) {
    LOGD("cmap_to_jmap_nested called, map size: %zu", cmap.size());
    LOGI("cmap_to_jmap_nested: starting conversion, map size=%zu", cmap.size());
    LOGD("cmap_to_jmap_nested: map type info - is nonstd::map: %s", typeid(cmap).name());

    // 遍历并打印map内容
    LOGD("cmap_to_jmap_nested: map contents:");
    for (const auto &entry : cmap) {
        LOGD("cmap_to_jmap_nested: key='%s', inner map size=%zu", entry.first.c_str(), entry.second.size());
        for (const auto &inner_entry : entry.second) {
            LOGD("cmap_to_jmap_nested: inner key='%s', value='%s'", inner_entry.first.c_str(), inner_entry.second.c_str());
        }
    }

    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap class");
        return nullptr;
    }

    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap constructor");
        return nullptr;
    }

    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap put method");
        return nullptr;
    }

    // 创建HashMap对象
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jmap) {
        LOGE("cmap_to_jmap_nested: Failed to create HashMap object");
        return nullptr;
    }

    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGE("cmap_to_jmap_nested: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    // 填充数据
    for (const auto& entry : cmap) {
        LOGD("cmap_to_jmap_nested: processing entry: key='%s', inner map size=%zu",
             entry.first.c_str(), entry.second.size());

        jstring jkey = env->NewStringUTF(entry.first.c_str());
        if (!jkey) {
            LOGE("cmap_to_jmap_nested: Failed to create key string");
            continue;
        }

        jobject jvalue = cmap_to_jmap(env, entry.second); // 调用之前定义的 cmap_to_jmap 方法
        if (!jvalue) {
            LOGE("cmap_to_jmap_nested: Failed to convert inner map");
            env->DeleteLocalRef(jkey);
            continue;
        }

        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);

        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap_nested: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }

        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }

    LOGI("cmap_to_jmap_nested: conversion completed successfully");
    return jmap;
}

// 主函数：将 map<string, map<string, map<string, string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
jobject cmap_to_jmap_nested_3(JNIEnv* env, const map<string, map<string, map<string, string>>>& cmap) {
    LOGD("cmap_to_jmap_nested_3 called, map size: %zu", cmap.size());
    LOGI("cmap_to_jmap_nested_3: starting conversion, map size=%zu", cmap.size());
    LOGD("cmap_to_jmap_nested_3: map type info - is nonstd::map: %s", typeid(cmap).name());

    // 遍历并打印map内容
    LOGD("cmap_to_jmap_nested_3: map contents:");
    for (const auto &entry : cmap) {
        LOGD("cmap_to_jmap_nested_3: key='%s', inner map size=%zu", entry.first.c_str(), entry.second.size());
        for (const auto &inner_entry : entry.second) {
            LOGD("cmap_to_jmap_nested_3: inner key='%s', inner inner map size=%zu", inner_entry.first.c_str(), inner_entry.second.size());
            for (const auto &inner_inner_entry : inner_entry.second) {
                LOGD("cmap_to_jmap_nested_3: inner inner key='%s', value='%s'", inner_inner_entry.first.c_str(), inner_inner_entry.second.c_str());
            }
        }
    }

    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap class");
        return nullptr;
    }

    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap constructor");
        return nullptr;
    }

    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap put method");
        return nullptr;
    }

    // 创建HashMap对象
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jmap) {
        LOGE("cmap_to_jmap_nested_3: Failed to create HashMap object");
        return nullptr;
    }

    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGE("cmap_to_jmap_nested_3: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    // 填充数据
    for (const auto& entry : cmap) {
        LOGD("cmap_to_jmap_nested_3: processing entry: key='%s', inner map size=%zu",
             entry.first.c_str(), entry.second.size());

        jstring jkey = env->NewStringUTF(entry.first.c_str());
        if (!jkey) {
            LOGE("cmap_to_jmap_nested_3: Failed to create key string");
            continue;
        }

        jobject jvalue = cmap_to_jmap_nested(env, entry.second);
        if (!jvalue) {
            LOGE("cmap_to_jmap_nested_3: Failed to convert inner map");
            env->DeleteLocalRef(jkey);
            continue;
        }

        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);

        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap_nested_3: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }

        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }

    LOGI("cmap_to_jmap_nested_3: conversion completed successfully");
    return jmap;
}

/**
 * JNI函数：获取设备信息
 * 将C++层的设备信息转换为Java对象并返回给Java层
 * @param env JNI环境指针
 * @param thiz 调用对象（未使用）
 * @return Java Map对象，包含所有收集到的设备信息
 */
extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_MainActivity_get_1device_1info(JNIEnv *env, jobject thiz) {
    LOGI("get_device_info: starting JNI call");

    // 将C++的三层嵌套Map转换为Java对象
    jobject result = cmap_to_jmap_nested_3(env, zManager::getInstance()->get_device_info());
    LOGI("get_device_info: conversion completed successfully");

    // 清空设备信息缓存，避免重复返回
    zManager::getInstance()->clear_device_info();
    return result;
}