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
#include "zThreadPool.h"

// 定义线程睡眠时间常量（秒）
#define OVERT_SLEEP_TIME 10

// 0 zConfig
// 1 zLog															        依赖等级 0
// 2 zLibc zLibcUtil											            依赖等级 0、1
// 3 zStdString zStdVector zStdMap zStdUtil					                依赖等级 0、1、2
// 4 zFile zHttps zCrc zTee zJson zJavaVm zElf zClassLoader zBroadCast		依赖等级 0、1、2、3
// 5、maps_info mounts_info	package_info ...								依赖等级 0、1、2、3、4
// 6、device_info

void __attribute__((constructor)) init_(void){
    LOGI("init_ start");

    zThreadPool::getInstance()->addTask("add_tasks", zManager::getInstance(), &zManager::add_tasks);

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
