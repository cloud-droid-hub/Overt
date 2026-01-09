//
// Created by lxz on 2025/8/7.
//
#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zFile.h"
#include "zLinker.h"
#include "zJavaVm.h"
#include "zJson.h"
#include "zBroadCast.h"

#include "zLocalNetworkInfo.h"
#include "zRootStateInfo.h"
#include "zSystemPropInfo.h"
#include "zLinkerInfo.h"
#include "zPortInfo.h"
#include "zClassLoaderInfo.h"
#include "zPackageInfo.h"
#include "zSystemSettingInfo.h"
#include "zTeeInfo.h"
#include "zTimeInfo.h"
#include "zSslInfo.h"
#include "zProcInfo.h"
#include "zSideChannelInfo.h"
#include "zHttps.h"
#include "zBinder.h"

static string get_process_name(){
    zFile file = zFile("/proc/self/cmdline");
    return file.readAllText();
}

void __attribute__((constructor)) init_(void){
    LOGI("zInfo init - Starting comprehensive tests");

    string processName = get_process_name();
    LOGI("processName: %s", processName.c_str());

    if(string_end_with(processName.c_str(), ".Server")){
        LOGI("isolated 进程已启动，等待接收共享内存 fd");
        // isolated 进程的初始化在收到 fd 时完成
    }else{
        LOGI("主进程已启动，初始化共享内存");
        // 主进程立即创建共享内存
        zBinder* binder = zBinder::getInstance();
        int fd = binder->createSharedMemory();
        if (fd >= 0) {
            LOGI("主进程共享内存初始化完成，fd=%d", fd);
            // 启动消息循环线程（等待 Service 连接后开始发送）
            binder->startMainMessageLoop();
        } else {
            LOGE("主进程共享内存初始化失败");
        }
    }

//    // 启动UDP广播发送器，向端口7476发送"overt"消息
//    zBroadCast::getInstance()->start_udp_broadcast_sender( 7476, "overt");
//
//    // 启动UDP广播监听器，监听端口7476，使用on_receive回调处理接收到的消息
//    zBroadCast::getInstance()->start_udp_broadcast_listener(7476, on_receive);

    // startOvertServer(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());

//    get_side_channel_info();

//    // 收集SSL信息 - 检测SSL证书异常
//    get_ssl_info();
//
//    // 收集本地网络信息 - 检测同一网络中的其他Overt设备
//    get_local_network_info();


//    // 收集Root文件信息 - 检测Root相关文件
//    get_root_file_info();
//
    // 收集挂载点信息 - 检测异常的文件系统挂载
//    get_proc_info();
//
//    // 收集系统属性信息 - 检测系统配置异常
//    get_system_prop_info();

    // 收集链接器信息 - 检测动态链接库加载异常
//    get_linker_info();

//    // 收集端口信息 - 检测网络端口异常
//    get_port_info();
//
//    // 收集类加载器信息 - 检测Java层异常
//    get_class_loader_info();
//
//    // 收集包信息 - 检测已安装应用异常
//    get_package_info();
//
//    // 收集系统设置信息 - 检测系统设置异常
//    get_system_setting_info();

    // 收集TEE信息 - 检测可信执行环境异常
//    get_tee_info();

//
//    // 收集时间信息 - 检测系统时间异常
//    get_time_info();


//    string qq_location_url = "https://r.inews.qq.com/api/ip2city";
//    string qq_location_url_fingerprint_sha256 = "A58095F1C26CA01A5AAC2666DCAA66182BE423BE47973BBD1F3CCFF9ACA59D14";
//    zHttps https_client(5);
//    HttpsRequest request(qq_location_url, "GET", 3);
//    HttpsResponse response = https_client.performRequest(request);


//    string pinduoduo_time_url = "https://api.pinduoduo.com/api/server/_stm";
//    string pinduoduo_time_fingerprint_sha256 = "604D2DE1AD32FF364041831DE23CBFC2C48AD5DEF8E665103691B6472D07D4D0";
//    zHttps https_client2(5);
//    HttpsRequest request2(pinduoduo_time_url, "GET", 3);
//    HttpsResponse response2 = https_client2.performRequest(request2);

    LOGI("zInfo init - All tests completed successfully");
}

extern "C" JNIEXPORT
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad called");

    LOGI("JNI_OnLoad over");
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_zinfo_Main_getSharedMemoryFd(JNIEnv *env, jobject thiz) {
    // TODO: implement getSharedMemoryFd()
    zBinder* binder = zBinder::getInstance();
    if (!binder->isInitialized()) {
        // 如果还没初始化，尝试初始化
        int fd = binder->createSharedMemory();
        return fd;
    }
    return binder->getFd();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_zinfo_ServerStarter_getSharedMemoryFd(JNIEnv *env, jclass clazz) {
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
    if(msg == "hello"){

        map<string, map<string, string>> info = get_proc_info();

        zJson json = info;

        return json.dump().c_str();
    }
    return "";
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_zinfo_Server_startFdListener(JNIEnv *env, jobject thiz, jint fd) {
    // TODO: implement startFdListener()
    LOGI("Starting fd listener, fd=%d", fd);

    // 映射共享内存
    zBinder* binder = zBinder::getInstance();

    // 启动 isolated 进程的消息处理循环线程
    binder->startServerMessageLoop(fd, fdListenerCallback);

    LOGI("Fd listener started successfully");
    return 0;
}

