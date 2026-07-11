#include <jni.h>
#include <bits/glibc-syscalls.h>

#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zManager.h"
#include "zThreadPool.h"
#include "zProcMaps.h"
#include "zProcInfo.h"
#include "zPortInfo.h"
#include "zSideChannelInfo.h"
#include "zJson.h"
#include "zBinder.h"
#include "zShell.h"
#include "zLinker.h"


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
}

void __attribute__((constructor)) init_(void){
    LOGI("init_ start");
    LOGI("init_ over");
}

map<string, map<string, string>> get_maps_info();
map<string, map<string, string>> get_task_info();
map<string, map<string, string>> get_net_tcp_info();

static int run_port_detector() {
    map<string, map<string, string>> info = get_port_info();
    return info.find("frida") == info.end() ? 0 : 1;
}

static int run_net_tcp_detector() {
    map<string, map<string, string>> info = get_net_tcp_info();
    for (const auto& item : info) {
        auto explain = item.second.find("explain");
        if (explain != item.second.end() && explain->second == "find frida port")
            return 1;
    }
    return 0;
}

static int run_task_detector() {
    return get_task_info().empty() ? 0 : 1;
}

static int run_soinfo_detector() {
    zLinker* linker = zLinker::getInstance();
    if (linker == nullptr)
        return 2;

    vector<string> libraries = linker->get_libpath_list();
    if (libraries.empty())
        return 2;

    for (const string& path : libraries) {
        if (strstr(path.c_str(), "frida") != nullptr)
            return 1;
    }
    return 0;
}

static int run_crc_detector(const char* library_name) {
    zLinker* linker = zLinker::getInstance();
    if (linker == nullptr)
        return 2;

    zElf library = linker->find_lib(library_name);
    if (library.elf_file_ptr == nullptr || library.elf_mem_ptr == nullptr)
        return 2;

    zProcMaps maps;
    LibraryMapping* mapping = maps.find_so_by_name(library_name);
    if (mapping == nullptr || mapping->address_range_start == nullptr)
        return 2;

    return zLinker::check_lib_crc(library_name) ? 1 : 0;
}

static int run_maps_layout_detector() {
    const char* libraries[] = {"libart.so", "libc.so", "libinput.so"};
    int checked = 0;
    zProcMaps maps;

    for (const char* library_name : libraries) {
        LibraryMapping* library = maps.find_so_by_name(library_name);
        if (library == nullptr)
            continue;

        checked++;
        if (library->segments.size() != 4)
            return 1;
        if (library->segments[0].permissions != "r--p" ||
            (library->segments[1].permissions != "r-xp" && library->segments[1].permissions != "--xp") ||
            (library->segments[2].permissions != "r--p" && library->segments[2].permissions != "rw-p") ||
            (library->segments[3].permissions != "rw-p" && library->segments[3].permissions != "r--p"))
            return 1;
    }

    return checked == 0 ? 2 : 0;
}

static int side_channel_baseline = -1;

static int run_side_channel_detector() {
    if (side_channel_baseline < 0)
        return 2;
    int current = measure_side_channel_error_count();
    int delta = abs(current - side_channel_baseline);
    LOGI("side channel baseline=%d current=%d delta=%d", side_channel_baseline, current, delta);
    return delta > 1000 ? 1 : 0;
}

static int run_child_stability_detector() {
    string output = runShell("printf strongfrida-child-ok");
    return output == "strongfrida-child-ok" ? 0 : 1;
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
	side_channel_baseline = measure_side_channel_error_count();
	LOGI("side channel baseline=%d", side_channel_baseline);

    LOGI("JNI_OnLoad over");
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_overt_MainActivity_runDetector(JNIEnv *env, jobject thiz,
                                                jstring selector_value) {
    if (selector_value == nullptr)
        return 2;

    const char* selector = env->GetStringUTFChars(selector_value, nullptr);
    if (selector == nullptr)
        return 2;

    int result = 2;
    if (strcmp(selector, "port-connect") == 0)
        result = run_port_detector();
    else if (strcmp(selector, "proc-net-tcp") == 0)
        result = run_net_tcp_detector();
    else if (strcmp(selector, "task-name") == 0)
        result = run_task_detector();
    else if (strcmp(selector, "soinfo-name") == 0)
        result = run_soinfo_detector();
    else if (strcmp(selector, "libc-crc") == 0)
        result = run_crc_detector("libc.so");
    else if (strcmp(selector, "libart-crc") == 0)
        result = run_crc_detector("libart.so");
    else if (strcmp(selector, "libinput-crc") == 0)
        result = run_crc_detector("libinput.so");
    else if (strcmp(selector, "maps-layout") == 0)
        result = run_maps_layout_detector();
    else if (strcmp(selector, "syscall-timing") == 0)
        result = run_side_channel_detector();
    else if (strcmp(selector, "child-stability") == 0)
        result = run_child_stability_detector();

    env->ReleaseStringUTFChars(selector_value, selector);
    return result;
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
