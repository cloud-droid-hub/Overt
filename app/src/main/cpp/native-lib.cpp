#include <jni.h>
#include <string>
#include <sys/system_properties.h>
#include <unistd.h>
#include <asm-generic/fcntl.h>
#include <linux/fcntl.h>
#include <sys/stat.h>
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
#include "time_info.h"
#include "linker_info.h"
#include "zElf.h"
#include "zLinker.h"
#include "package_info.h"
#include "device_info.h"
#include "class_loader_info.h"


#define PAGE_START(x)  ((x) & PAGE_MASK)
#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

void resampleTouchState(void *thiz, void *a, void *b, void *msg) {
    LOGE("resampleTouchState is called");
}

extern "C" JNIEXPORT
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    __android_log_print(6, "lxz", "JNI_OnLoad");

//    zLinker::get_maps_base("libart.so");


    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6)!= JNI_OK) {
        LOGE("Failed to get the environment");
        return -1;
    }

    device_info["package_info"] = get_package_info();
    device_info["root_file_info"] = get_root_file_info();
    device_info["mounts_info"] = get_mounts_info();
    device_info["class_loader_info"] = get_class_loader_info(env);
    device_info["class_info"] = get_class_info(env);
    device_info["system_prop_info"] = get_system_prop_info();
    device_info["linker_info"] = get_linker_info();
    device_info["time_info"] = get_time_info();

    return JNI_VERSION_1_6;
}

// 将 std::map<std::string, std::string> 转换为 Java Map<String, String>
jobject cmap_to_jmap(JNIEnv *env, std::map<std::string, std::string> cmap){
    jobject jobjectMap = env->NewObject(env->FindClass("java/util/HashMap"), env->GetMethodID(env->FindClass("java/util/HashMap"), "<init>", "()V"));
    for (auto &entry : cmap) {
        env->CallObjectMethod(jobjectMap, env->GetMethodID(env->FindClass("java/util/HashMap"), "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"), env->NewStringUTF(entry.first.c_str()), env->NewStringUTF(entry.second.c_str()));
    }
    return jobjectMap;
}

// 将 std::map<std::string, std::map<std::string, std::string>> 转换为 Java Map<String, Map<String, String>>
jobject cmap_to_jmap_nested(JNIEnv* env, const std::map<std::string, std::map<std::string, std::string>>& cmap) {
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);

    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    for (const auto& entry : cmap) {
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        jobject jvalue = cmap_to_jmap(env, entry.second); // 调用之前定义的 cmap_to_jmap 方法
        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    return jmap;
}

// 主函数：将 std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
jobject cmap_to_jmap_nested_3(JNIEnv* env, const std::map<std::string, std::map<std::string, std::map<std::string, std::string>>>& cmap) {
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);

    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    for (const auto& entry : cmap) {
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        jobject jvalue = cmap_to_jmap_nested(env, entry.second);
        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    return jmap;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_overt_device_DeviceInfoProvider_get_1device_1info(JNIEnv *env, jobject thiz) {
    // TODO: implement get_device_info()
    return cmap_to_jmap_nested_3(env,device_info);
}