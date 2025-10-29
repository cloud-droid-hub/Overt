//
// Created by lxz on 2025/6/16.
//

#include "zJavaVm.h"
#include "zFile.h"

#include "zPackageInfo.h"
#include "zShell.h"

/**
 * 通过Context检测应用是否已安装
 * 使用Android PackageManager API检测指定包名的应用是否已安装
 * 采用两种检测方式：getApplicationInfo和getLaunchIntentForPackage
 * @param env JNI环境指针
 * @param context Android上下文对象
 * @param packageNameCStr 要检测的包名
 * @return true表示应用已安装，false表示未安装
 */
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

/**
 * 通过文件路径检测应用是否已安装
 * 检查应用数据目录是否存在，判断应用是否已安装
 * 主要检查/data/data、/data/user/0、/data/user_de/0等目录
 * @param packageName 要检测的包名
 * @return true表示应用已安装，false表示未安装
 */
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

/**
 * 通过Shell命令检测应用是否已安装
 * 使用file命令检查应用数据目录，通过命令输出判断应用是否存在
 * 这是一种绕过某些检测机制的辅助检测方法
 * @param packageName 要检测的包名
 * @return true表示应用已安装，false表示未安装
 */
bool isAppInstalledByShellHole(const char* packageName) {
    string cmd = "file /storage/emulated/\342\200\2130/Android/data/" + string(packageName);
    string ret = runShell(cmd);
    LOGI("cmd: %s", ret.c_str());
    if(strstr(ret.c_str(), "No such file or directory") == nullptr){
        return true;
    }
    return false;
}

/**
 * 获取包信息的主函数（带JNI参数）
 * 检测系统中安装的应用程序包，识别可疑的调试工具和Root框架
 * 通过多种方式检测应用安装状态，包括PackageManager、文件系统、Shell命令等
 * @param env JNI环境指针
 * @param context Android上下文对象
 * @return 包含检测结果的Map，格式：{包名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_package_info(JNIEnv *env, jobject context){
    map<string, map<string, string>> info;

    // 定义黑名单应用包名和对应的应用名称
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

    // 定义白名单应用包名和对应的应用名称
    map<string, string> white_map = {
            {"com.tencent.mm", "微信"},
            {"com.eg.android.AlipayGphone", "支付宝"},
    };

    // 检查黑名单应用是否已安装
    for (auto &[package_name, app_name] : black_map) {
        if(isAppInstalledByContext(env, context, package_name.c_str())){
            info[package_name]["risk"] = "error";
            info[package_name]["explain"] = "black package name but install[pms] " + app_name;
        }else if(isAppInstalledByPath(package_name.c_str())){
            info[package_name]["risk"] = "error";
            info[package_name]["explain"] = "black package name but install[file] " + app_name;
        }else if(isAppInstalledByShellHole(package_name.c_str())){
            info[package_name]["risk"] = "error";
            info[package_name]["explain"] = "black package name but install[shell hole] " + app_name;
        }
    }

    // 检查白名单应用是否未安装
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

/**
 * 获取包信息的主函数（无参数版本）
 * 自动获取JNI环境和上下文对象，调用带参数的版本
 * @return 包含检测结果的Map，格式：{包名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_package_info(){
    return get_package_info(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
};