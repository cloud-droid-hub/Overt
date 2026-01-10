#include <jni.h>

#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zManager.h"
#include "zThreadPool.h"
#include "zProcMaps.h"
#include "zProcInfo.h"
#include "zJson.h"
#include "zBinder.h"

// 0 zConfig
// 1 zLog															        依赖等级 0
// 2 zLibc zLibcUtil											            依赖等级 0、1
// 3 zStdString zStdVector zStdMap zStdUtil					                依赖等级 0、1、2
// 4 zFile zHttps zCrc zTee zJson zJavaVm zElf zClassLoader zThreadPool		依赖等级 0、1、2、3
// 5 zMapsInfo zProcInfo zPackageInfo ...								    依赖等级 0、1、2、3、4
// 6 zManager                                                               依赖等级 0、1、2、3、4、5

static string get_process_name(){
    zFile file = zFile("/proc/self/cmdline");
    return file.readAllText();
    for(string line : file.readAllLines()){
        return line;
    }
    return "";
}

void __attribute__((constructor)) init_(void){
    LOGI("init_ start");
    string processName = get_process_name();
    if(string_end_with(processName.c_str(), ".Server")){
    }else{
        zThreadPool::getInstance()->addTask("round_tasks", zManager::getInstance(), &zManager::round_tasks);
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

    LOGI("JNI_OnLoad over");
    return JNI_VERSION_1_6;
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_example_overt_ServerStarter_getSharedMemoryFd(JNIEnv *env, jclass clazz) {
    // TODO: implement getSharedMemoryFd()
    zBinder* binder = zBinder::getInstance();
    if (!binder->isInitialized()) {
        // 如果还没初始化，尝试初始化
        int fd = binder->createSharedMemory();
        return fd;
    }
    return binder->getFd();
}

std::string fdListenerCallback(std::string msg){
    if(msg == "get_isoloated_process_info"){
        map<string, map<string, string>> info = get_proc_info();
        zJson json = info;
        return json.dump().c_str();
    }
    return "";
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_overt_Server_startFdListener(JNIEnv *env, jobject thiz, jint fd) {
    // TODO: implement startFdListener()

    // 映射共享内存
    zBinder* binder = zBinder::getInstance();

    // 启动 isolated 进程的消息处理循环线程
    binder->startServerMessageLoop(fd, fdListenerCallback);

    LOGI("Fd listener started successfully");
    return 0;
}