//
// Created by lxz on 2025/6/16.
//



#include "zJavaVm.h"
#include "zFile.h"

#include "package_info.h"

bool isAppInstalledByContext(JNIEnv *env, jobject context, const char* packageNameCStr) {
    // 1. 将 C 字符串转换为 jstring
    jstring packageName = env->NewStringUTF(packageNameCStr);

    // 2. 获取 Context 类
    jclass contextClass = env->GetObjectClass(context);
    jmethodID getPackageManager = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject packageManager = env->CallObjectMethod(context, getPackageManager);

    // 3. 获取 PackageManager 类及方法
    jclass pmClass = env->GetObjectClass(packageManager);
    jmethodID getApplicationInfo = env->GetMethodID(pmClass, "getApplicationInfo", "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;");
    jmethodID getLaunchIntentForPackage = env->GetMethodID(pmClass, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");

    // 4. 先调用 getApplicationInfo
    jboolean installed = false;
    jobject appInfo = env->CallObjectMethod(packageManager, getApplicationInfo, packageName, 0);
    if (!env->ExceptionCheck()) {
        installed = true;
    } else {
        env->ExceptionClear();
    }

    // 5. 如果失败，再尝试 getLaunchIntentForPackage
    if (!installed) {
        jobject intent = env->CallObjectMethod(packageManager, getLaunchIntentForPackage, packageName);
        if (intent != nullptr) {
            installed = true;
        }
    }

    // 6. 清理局部引用
    env->DeleteLocalRef(packageName);

    return installed;
}

bool isAppInstalledByPath(const char* packageName) {
    vector<string> dir_list = {
            "/data/data",
            "/data/user/0",
            "/data/user_de/0",
            "/storage/emulated/0/Android/data/"
    };
    for(string dir : dir_list){
        string path = "/storage/emulated/0/Android/data/" + string(packageName);
        if(zFile(path).exists()) {
            return true;
        }
    }
    return false;
}


map<string, map<string, string>> get_package_info(JNIEnv *env, jobject context){
    map<string, map<string, string>> info;

    map<string, string> black_map = {
            {"com.zhenxi.hunter", "Hunter"},
            {"com.juqing.catchpackhelper", "抓包帮手"},
            {"bin.mt.plus", "MT 管理器"},
            {"moe.haruue.wadb", "无线 adb"},
            {"org.hapjs.debuger", "快应用调试器"},
            {"org.hapjs.mockup", "快应用预览版"},
            {"me.bmax.apatch", "Apatch"},
            {"com.coolapk.market", "DNA-Android"},
            {"com.atominvention.rootchecker", "Root 测试工具"},
            {"moe.haruue.wadb", "网络 adb 调试"},
            {"com.topjohnwu.magisk", "Magisk"},
            {"io.github.vvb2060.xposeddetector", "密钥认证"},
            {"io.github.huskydg.memorydetector", "MemoryDetector"},
            {"com.byxiaorun.detector", "Ruru"},
            {"icu.nullptr.nativetest", "Native Test"},
            {"io.github.vvb2060.mahoshojo", "Momo"},
            {"luna.safe.luna", "Luna"},
            {"org.lsposed.manager", "LSPosed"},
            {"de.blinkt.openvpn", "OpenVPN"},
            {"net.openvpn.openvpn", "OpenVPN Connect"},
            {"com.github.kr328.clash", "Clash for Android"},
            {"me.weishu.kernelsu", "KernelSU"},
    };

    map<string, string> white_map = {
            {"com.tencent.mm", "微信"},
            {"com.eg.android.AlipayGphone", "支付宝"},
    };

    for (auto &[package_name, app_name] : black_map) {
        if(isAppInstalledByPath(package_name.c_str())){
            info[package_name]["risk"] = "error";
            info[package_name]["explain"] = "black package name but install " + app_name;
        }else if(isAppInstalledByContext(env, context, package_name.c_str())){
            info[package_name]["risk"] = "error";
            info[package_name]["explain"] = "black package name but install " + app_name;
        }
    }

    for (auto &[package_name, app_name] : white_map) {
        bool is_installed_by_path = isAppInstalledByPath(package_name.c_str());
        bool is_installed_by_context = isAppInstalledByContext(env, context,package_name.c_str());
        if(!is_installed_by_path && !is_installed_by_context){
            info[package_name]["risk"] = "warn";
            info[package_name]["explain"] = "white package name but uninstall " + app_name;
        }
    }

    return info;
};

map<string, map<string, string>> get_package_info(){
    return get_package_info(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
};