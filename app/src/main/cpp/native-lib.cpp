#include <jni.h>
#include <chrono>
#include <algorithm>
#include <dirent.h>

#include "zLog.h"
#include "zDevice.h"
#include "system_setting_info.h"
#include "class_loader_info.h"
#include "zUtil.h"
#include "package_info.h"
#include "root_file_info.h"
#include "mounts_info.h"
#include "system_prop_info.h"
#include "linker_info.h"
#include "time_info.h"
#include "zFile.h"
#include "maps_info.h"
#include "zLinker.h"
#include "task_info.h"
#include "port_info.h"
#include "tee_info.h"
#include "zJavaVm.h"
#include "config.h"
#include "ssl_info.h"
#include "zJson.h"
#include "zBroadCast.h"
#include "local_network_info.h"

// 定义线程睡眠时间常量（秒）
#define OVERT_SLEEP_TIME 10

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
        bind_self_to_least_used_big_core();

        // 提升线程优先级，确保信息收集的及时性
        raise_thread_priority();

        // 记录开始时间，用于计算执行耗时
        time_t start_time = time(nullptr);

        // 收集本地网络信息 - 检测同一网络中的其他Overt设备
        zDevice::getInstance()->update_device_info("local_network_info", get_local_network_info());
        
        // 收集任务信息 - 检测Frida等调试工具注入的进程
        zDevice::getInstance()->update_device_info("task_info", get_task_info());
        
        // 收集内存映射信息 - 检测关键系统库是否被篡改
        zDevice::getInstance()->update_device_info("maps_info",get_maps_info());
        
        // 收集Root文件信息 - 检测Root相关文件
        zDevice::getInstance()->update_device_info("root_file_info", get_root_file_info());
        
        // 收集挂载点信息 - 检测异常的文件系统挂载
        zDevice::getInstance()->update_device_info("mounts_info", get_mounts_info());
        
        // 收集系统属性信息 - 检测系统配置异常
        zDevice::getInstance()->update_device_info("system_prop_info", get_system_prop_info());
        
        // 收集链接器信息 - 检测动态链接库加载异常
        zDevice::getInstance()->update_device_info("linker_info", get_linker_info());
        
        // 收集端口信息 - 检测网络端口异常
        zDevice::getInstance()->update_device_info("port_info", get_port_info());
        
        // 收集类加载器信息 - 检测Java层异常
        zDevice::getInstance()->update_device_info("class_loader_info", get_class_loader_info());
        
        // 收集包信息 - 检测已安装应用异常
        zDevice::getInstance()->update_device_info("package_info", get_package_info());
        
        // 收集系统设置信息 - 检测系统设置异常
        zDevice::getInstance()->update_device_info("system_setting_info", get_system_setting_info());
        
        // 收集TEE信息 - 检测可信执行环境异常
        zDevice::getInstance()->update_device_info("tee_info", get_tee_info());

        // 收集SSL信息 - 检测SSL证书异常
        zDevice::getInstance()->update_device_info("ssl_info", get_ssl_info());
        
        // 收集时间信息 - 检测系统时间异常
        zDevice::getInstance()->update_device_info("time_info", get_time_info());

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

/**
 * 库初始化函数，使用constructor属性确保在main函数之前执行
 * 创建并启动设备信息收集线程
 */


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
    jobject result = cmap_to_jmap_nested_3(env, zDevice::getInstance()->get_device_info());
    LOGI("get_device_info: conversion completed successfully");
    
    // 清空设备信息缓存，避免重复返回
    zDevice::getInstance()->clear_device_info();
    return result;
}