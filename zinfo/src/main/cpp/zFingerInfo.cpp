//
// Created by lxz on 2025/6/12.
//

#include <jni.h>
#include <media/NdkMediaDrm.h>
#include <regex>
#include <sys/sysinfo.h>
#include <sys/statfs.h>
#include "zLibc.h"
#include "zLog.h"
#include "zFile.h"
#include "zJavaVm.h"
#include "zTeeCert.h"

string hash16(const string& input) {
    const char base32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : input) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    string out;
    out.reserve(16);
    for (int i = 0; i < 16; ++i) {
        out.push_back(base32[(h >> (i * 5)) & 0x1F]);
    }
    return out;
}

string get_android_id(JNIEnv* env, jobject context) {
    if (!context) return "";

    jclass clsContext = env->GetObjectClass(context);
    if (!clsContext) return "";

    // Context.getContentResolver()
    jmethodID midGetResolver = env->GetMethodID(
            clsContext,
            "getContentResolver",
            "()Landroid/content/ContentResolver;"
    );
    if (!midGetResolver) return "";

    jobject resolver = env->CallObjectMethod(context, midGetResolver);
    if (!resolver) return "";

    jclass clsSecure = env->FindClass("android/provider/Settings$Secure");
    if (!clsSecure) return "";

    jmethodID midGetString = env->GetStaticMethodID(
            clsSecure,
            "getString",
            "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;"
    );
    if (!midGetString) return "";

    jstring jKey = env->NewStringUTF("android_id");

    jstring result = (jstring)env->CallStaticObjectMethod(
            clsSecure,
            midGetString,
            resolver,
            jKey
    );

    env->DeleteLocalRef(jKey);

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return "";
    }

    const char* chars = env->GetStringUTFChars(result, nullptr);
    string ret(chars);
    env->ReleaseStringUTFChars(result, chars);

    return ret;
}

string get_store_finger(){
    struct statfs64 buf={};
    if (statfs64("/storage/emulated/0", &buf) == -1) {
        LOGE("statfs64系统信息失败");
        return "";
    }
    // 拼接指纹字符串
    char fingerprint[512];
    snprintf(fingerprint, sizeof(fingerprint),
             "%llu_%llu_%llu_%llu_%d_%d_%llu",
             (unsigned long long)buf.f_type,
             (unsigned long long)buf.f_bsize,
             (unsigned long long)buf.f_blocks,
             (unsigned long long)buf.f_files,
             buf.f_fsid.__val[0],
             buf.f_fsid.__val[1],
             (unsigned long long)buf.f_namelen);

    LOGI("设备指纹: %s", fingerprint);
    return fingerprint;
}

string get_drm_id(){
    const uint8_t uuid[] = {0xed,0xef,0x8b,0xa9,0x79,0xd6,0x4a,0xce,
                            0xa3,0xc8,0x27,0xdc,0xd5,0x1d,0x21,0xed
    };

    // 检查是否支持该加密方案
    if (!AMediaDrm_isCryptoSchemeSupported(uuid, nullptr)) {
        LOGE("Widevine DRM scheme not supported on this device");
        return "";
    }

    AMediaDrm *mediaDrm = AMediaDrm_createByUUID(uuid);
    if (mediaDrm == nullptr) {
        LOGE("Failed to create AMediaDrm");
        return "";
    }

    // 获取 deviceUniqueId
    AMediaDrmByteArray aMediaDrmByteArray = {nullptr, 0};
    media_status_t status = AMediaDrm_getPropertyByteArray(mediaDrm, PROPERTY_DEVICE_UNIQUE_ID, &aMediaDrmByteArray);
    if (status != AMEDIA_OK) {
        // 常见错误码：
        // -2002: AMEDIA_DRM_NOT_PROVISIONED (需要配置)
        // -2003: AMEDIA_DRM_DEVICE_REVOKED (设备被撤销)
        // -10003: 可能是其他错误
        LOGE("Failed to get device unique ID, status: %d (AMEDIA_OK=%d). "
             "This may indicate the device needs provisioning or DRM is not available.",
             status, AMEDIA_OK);
        AMediaDrm_release(mediaDrm);
        return "";
    }

    if (aMediaDrmByteArray.ptr == nullptr || aMediaDrmByteArray.length == 0) {
        LOGE("Device unique ID is null or empty, ptr: %p, length: %zu",
             aMediaDrmByteArray.ptr, aMediaDrmByteArray.length);
        AMediaDrm_release(mediaDrm);
        return "";
    }

    LOGI("Successfully got device unique ID, length: %zu", aMediaDrmByteArray.length);

    // 32 太长了，压缩一下
    // 将 32 字节压缩为 16 个字符：每 4 个字节异或合并为 1 个字节，然后转为 16 个十六进制字符
    // 32 字节 -> 8 字节 -> 16 个十六进制字符
    uint8_t compressed[8] = {0};
    size_t compressedLen = aMediaDrmByteArray.length / 4;
    if (compressedLen > 8) compressedLen = 8;

    for (size_t i = 0; i < compressedLen; i++) {
        uint8_t result = 0;
        for (size_t j = 0; j < 4 && (i * 4 + j) < aMediaDrmByteArray.length; j++) {
            result ^= aMediaDrmByteArray.ptr[i * 4 + j];
        }
        compressed[i] = result;
    }

    // 转为 16 个十六进制字符
    string hexStr;
    hexStr.reserve(16);
    for (size_t i = 0; i < compressedLen; i++) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", compressed[i]);
        hexStr += hex;
    }

    LOGI("DRM device unique ID (compressed): %s", hexStr.c_str());

    // 释放资源
    // 注意：AMediaDrm_getPropertyByteArray 返回的内存由 API 管理，无需手动释放
    AMediaDrm_release(mediaDrm);

    return hexStr;
}

string get_package_base_apk_path(JNIEnv* env, jobject context, const string& pkg) {
    if (env == nullptr || context == nullptr || pkg.empty()) {
        return "";
    }

    string result;

    // Context.getPackageManager()
    jclass contextCls = env->GetObjectClass(context);
    jmethodID midGetPM = env->GetMethodID(
            contextCls,
            "getPackageManager",
            "()Landroid/content/pm/PackageManager;"
    );
    jobject pm = env->CallObjectMethod(context, midGetPM);
    env->DeleteLocalRef(contextCls);

    if (pm == nullptr) {
        return "";
    }

    // PackageManager.getApplicationInfo(String, int)
    jclass pmCls = env->GetObjectClass(pm);
    jmethodID midGetAI = env->GetMethodID(
            pmCls,
            "getApplicationInfo",
            "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;"
    );

    jstring jPkg = env->NewStringUTF(pkg.c_str());
    jobject ai = env->CallObjectMethod(pm, midGetAI, jPkg, 0);
    env->DeleteLocalRef(jPkg);
    env->DeleteLocalRef(pmCls);
    env->DeleteLocalRef(pm);

    if (env->ExceptionCheck() || ai == nullptr) {
        env->ExceptionClear(); // 包不存在 / 不可见
        return "";
    }

    // ApplicationInfo.sourceDir
    jclass aiCls = env->GetObjectClass(ai);
    jfieldID fidSourceDir = env->GetFieldID(
            aiCls,
            "sourceDir",
            "Ljava/lang/String;"
    );

    jstring sourceDir = (jstring) env->GetObjectField(ai, fidSourceDir);
    if (sourceDir != nullptr) {
        const char* base_apk_path = env->GetStringUTFChars(sourceDir, nullptr);
        result = base_apk_path;
        env->DeleteLocalRef(sourceDir);
    }

    env->DeleteLocalRef(aiCls);
    env->DeleteLocalRef(ai);

    return result;
}


string get_app_specific_dir_finger(string path) {
    std::cmatch match;
    std::regex rx(R"(/data/app/(?:~~[^/]+/)?[^/-]+-([^/=]+)==/)");
    if (std::regex_search(path.c_str(), match, rx)) {
        return match[1].str().c_str();
    }
    return "";
}

string get_boot_id() {
    zFile boot_id = zFile("/proc/sys/kernel/random/boot_id");
    if(!boot_id.exists()){
        return "";
    }
    string s = boot_id.readLine();
    if (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.erase(s.find_last_not_of("\r\n") + 1);
    }
    return s;
}

map<string, map<string, string>> get_finger_info() {
    LOGI("get_time_info: starting...");

    map<string, map<string, string>> info;

    string android_id = get_android_id(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
    info["android_id"]["risk"] = "safe";
    info["android_id"]["explain"] = android_id;

    string store_finger =get_store_finger();
    info["store_finger"]["risk"] = "safe";
    info["store_finger"]["explain"] = hash16(store_finger);

    string drm_id =get_drm_id();
    if(drm_id.empty()){
        info["drm_id"]["risk"] = "error";
        info["drm_id"]["explain"] = "drm_id is empty";
    }else{
        info["drm_id"]["risk"] = "safe";
        info["drm_id"]["explain"] = drm_id;
    }

    string base_apk_path = get_package_base_apk_path(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext(), "com.tencent.mm");

    string weixin_finger = get_app_specific_dir_finger(base_apk_path);
    if(weixin_finger.empty()){
        info["weixin_finger"]["risk"] = "warn";
        info["weixin_finger"]["explain"] = "weixin_finger is empty";
    }else{
        info["weixin_finger"]["risk"] = "safe";
        info["weixin_finger"]["explain"] = hash16(base_apk_path);
    }

    string boot_id = get_boot_id();
    if(boot_id.empty()){
        info["boot_id"]["risk"] = "error";
        info["boot_id"]["explain"] = "boot_id is empty";
    }else{
        info["boot_id"]["risk"] = "safe";
        info["boot_id"]["explain"] = boot_id;
    }

    return info;
}
