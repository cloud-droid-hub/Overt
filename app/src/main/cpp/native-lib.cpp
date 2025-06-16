#include <jni.h>
#include <string>
#include <sys/system_properties.h>
#include <unistd.h>
#include <asm-generic/fcntl.h>
#include <linux/fcntl.h>
#include <sys/stat.h>
#include "libc.h"
#include "string.h"
#include <android/log.h>
#include <map>
#include <vector>
#include <bits/glibc-syscalls.h>
#include <linux/stat.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <sys/syscall.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <limits.h> // PATH_MAX
#include <cstring>
#include <sys/types.h>
#include <optional>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <ctime>
#include <android/log.h>
#include "root_file_info.h"
#include "mounts_info.h"
#include "system_prop_info.h"
#include "classloader.h"
#include "zElfEditor.h"
#include "time_info.h"
#include "linker_info.h"
#include "zElf.h"
#include "zLinker.h"

#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

static std::map<std::string, std::vector<std::string>> device_info;


void test(){
    //    zElfEditor android_runtime_mem = zElfEditor("libandroid_runtime.so");
//    LOGE("android_runtime_mem %p", android_runtime_mem.elf_addr);
//
//    zElfEditor android_runtime_file = zElfEditor("/system/lib64/libandroid_runtime.so");
//    LOGE("android_runtime_file %p", android_runtime_file.elf_addr);
//
//    Elf64_Addr runtime_offset = android_runtime_file.find_symbol("_ZN7android14AndroidRuntime10getRuntimeEv");
//
//    auto GetAndroidRuntime = (void*(*)())(android_runtime_mem.elf_addr + runtime_offset);
//
//    LOGE("GetAndroidRuntime %p", GetAndroidRuntime);
//
//    void* runtime = GetAndroidRuntime();
//
//    LOGE("runtime %p", runtime);
//
//    char* mOptions_ptr = (char*)runtime+8;
//    LOGE("mOptions_ptr %p", mOptions_ptr);
//
//    void* vector_base = mOptions_ptr;
//
//    int mOptions_size = *(int*)(mOptions_ptr+0x10);
//    LOGE("mOptions_size %d", mOptions_size);
//
//    JavaVMOption* option = *(JavaVMOption**)(mOptions_ptr+8);
//    for(int i = 0; i < mOptions_size; i++){
//        LOGE("optionString[%d] %p", i , option);
//        LOGE("optionString[%d] %p %p", i , option, option->optionString);
//        LOGE("optionString[%d] %p %p %s", i , option, option->optionString, option->optionString);
//        option++;
//    }

}

extern "C" JNIEXPORT
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    __android_log_print(6, "lxz", "JNI_OnLoad");



    return JNI_VERSION_1_6;
}

jobject cmap_to_jmap(JNIEnv *env, std::map<std::string, std::string> cmap){
    jobject jobjectMap = env->NewObject(env->FindClass("java/util/HashMap"), env->GetMethodID(env->FindClass("java/util/HashMap"), "<init>", "()V"));
    for (auto &entry : cmap) {
        env->CallObjectMethod(jobjectMap, env->GetMethodID(env->FindClass("java/util/HashMap"), "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"), env->NewStringUTF(entry.first.c_str()), env->NewStringUTF(entry.second.c_str()));
    }
    return jobjectMap;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_device_DeviceInfoProvider_get_1root_1file_1info(JNIEnv *env, jobject thiz) {
    // TODO: implement get_root_file_info()
    std::map<std::string, std::string> root_file_info = get_root_file_info();
    return cmap_to_jmap(env, root_file_info);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_device_DeviceInfoProvider_get_1mounts_1info(JNIEnv *env, jobject thiz) {
    // TODO: implement get_mounts_info()
    std::map<std::string, std::string> mounts_info = get_mounts_info();
    return cmap_to_jmap(env, mounts_info);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_device_DeviceInfoProvider_get_1system_1prop_1info(JNIEnv *env, jobject thiz) {
    // TODO: implement get_system_prop_info()
    std::map<std::string, std::string> system_prop_info = get_system_prop_info();
    return cmap_to_jmap(env, system_prop_info);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_device_DeviceInfoProvider_get_1class_1loader_1info(JNIEnv *env, jobject thiz) {
    if (env == nullptr) {
        LOGE("get_class_loader_info: env is null");
        return nullptr;
    }

    std::map<std::string, std::string> class_loader_info;
    
    // 检查异常
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        LOGE("get_class_loader_info: Exception occurred before traversal");
        return nullptr;
    }

    // 遍历类加载器
    traverseClassLoader(env);
    
    // 检查异常
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        LOGE("get_class_loader_info: Exception occurred during traversal");
        return nullptr;
    }

    // 处理类加载器列表
    for(const std::string& str : classLoaderStringList) {
        if (str.empty()) {
            continue;
        }

        if(strstr(str.c_str(), "LspModuleClassLoader")) {
            class_loader_info[str] = "black";
        }
        if(strstr(str.c_str(), "InMemoryDexClassLoader") && strstr(str.c_str(), "nativeLibraryDirectories=[/system/lib64, /system_ext/lib64]")) {
            class_loader_info[str] = "black";
        }
        if(strstr(str.c_str(), "InMemoryDexClassLoader") && strstr(str.c_str(), "nativeLibraryDirectories=[/system/lib64, /system/system_ext/lib64]")) {
            class_loader_info[str] = "black";
        }
    }

    return cmap_to_jmap(env, class_loader_info);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_device_DeviceInfoProvider_get_1class_1info(JNIEnv *env, jobject thiz) {
    std::map<std::string, std::string> class_info;

    std::vector<std::string> black_name_list = {
//            "xposed", "lsposed", "lspd"
        "XposedHooker"
    };

    for(std::string className : classNameList){
        std::transform(className.begin(), className.end(), className.begin(), [](unsigned char c) { return std::tolower(c); });
        for(std::string black_name: black_name_list){
            std::transform(black_name.begin(), black_name.end(), black_name.begin(), [](unsigned char c) { return std::tolower(c); });
            if(strstr(className.c_str(), black_name.c_str()) != 0){
                LOGE("className %s", className.c_str());
                class_info[className] = "black";
            }
        }
    }
    return cmap_to_jmap(env, class_info);
}


extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_device_DeviceInfoProvider_get_1time_1info(JNIEnv *env, jobject thiz) {
    // TODO: implement get_time_info()
    std::map<std::string, std::string> info = get_time_info();
    return cmap_to_jmap(env, info);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_device_DeviceInfoProvider_get_1linker_1info(JNIEnv *env, jobject thiz) {
    // TODO: implement get_linker_info()
    std::map<std::string, std::string> info = get_linker_info();
    return cmap_to_jmap(env, info);
}