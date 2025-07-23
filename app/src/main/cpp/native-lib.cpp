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

void __attribute__((constructor)) init_(void){
    LOGE("init_ start");

    zDevice::getInstance()->get_device_info()["task_info"] = get_task_info();
    zDevice::getInstance()->get_device_info()["maps_info"] = get_maps_info();
    zDevice::getInstance()->get_device_info()["class_loader_info"] = get_class_loader_info();
    zDevice::getInstance()->get_device_info()["class_info"] = get_class_info();
    zDevice::getInstance()->get_device_info()["root_file_info"] = get_root_file_info();
    zDevice::getInstance()->get_device_info()["mounts_info"] = get_mounts_info();
    zDevice::getInstance()->get_device_info()["system_prop_info"] = get_system_prop_info();
    zDevice::getInstance()->get_device_info()["linker_info"] = get_linker_info();
    zDevice::getInstance()->get_device_info()["time_info"] = get_time_info();
    zDevice::getInstance()->get_device_info()["port_info"] = get_port_info();
    zDevice::getInstance()->get_device_info()["package_info"] = get_package_info();
    zDevice::getInstance()->get_device_info()["system_setting_info"] = get_system_setting_info();
    zDevice::getInstance()->get_device_info()["tee_info"] = get_tee_info();

    LOGE("init_ over");
}

extern "C" JNIEXPORT
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGE("JNI_OnLoad");
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_MainActivity_get_1device_1info(JNIEnv *env, jobject thiz) {
    LOGE("get_device_info: starting JNI call");
    jobject result = cmap_to_jmap_nested_3(env, zDevice::getInstance()->get_device_info());
    LOGE("get_device_info: conversion completed successfully");
    return result;
}