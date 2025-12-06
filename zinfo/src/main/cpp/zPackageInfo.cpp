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
            "/storage/emulated/0/Android/data"
    };
    for(string dir : dir_list){
        string path = dir  + "/" + string(packageName);
        if(zFile(path).exists()) {
            return true;
        }
    }
    return false;
}

/**
 * 通过路径漏洞越权检测应用是否已安装
 * 这是一种绕过某些检测机制的辅助检测方法
 * @param packageName 要检测的包名
 * @return true表示应用已安装，false表示未安装
 */
bool isAppInstalledByPathHole(const char* packageName) {
    string path = "/sdcard/android/\u200bdata/" + string(packageName);
    if(zFile(path).exists()) {
        return true;
    }
    return false;
}
/**
 * 通过Shell命令越权漏洞检测应用是否已安装
 * 这是一种绕过某些检测机制的辅助检测方法
 * @param packageName 要检测的包名
 * @return true表示应用已安装，false表示未安装
 */
bool isAppInstalledByShellHole(const char* packageName) {
    string cmd = "file /storage/emulated/\342\200\2130/Android/data/" + string(packageName);
    string ret = runShell(cmd);
    LOGI("cmd: %s", ret.c_str());
    if(strstr(ret.c_str(), "cannot open") == nullptr){
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
            // Root管理工具
            {"me.weishu.kernelsu", "KernelSU"},
            {"com.topjohnwu.magisk", "Magisk"},
            {"io.github.vvb2060.magisk", "Magisk Alpha"},
            {"io.github.huskydg.magisk", "Kitsune Mask"},
            {"io.github.cycle1337.kernelsu", "KernelSU"},
            {"com.rifsxd.ksunext", "KernelSU Next"},
            {"com.sukisu.ultra", "SukiSU Ultra"},
            {"me.bmax.apatch", "APatch"},
            {"org.lsposed.manager", "LSPosed"},
            {"de.robv.android.xposed.installer", "Xposed Installer"},

            // 调试工具
            {"com.zhenxi.hunter", "Hunter"},
            {"com.juqing.catchpackhelper", "抓包帮手"},
            {"bin.mt.plus", "MT 管理器"},
            {"moe.haruue.wadb", "无线 adb"},
            {"org.hapjs.debuger", "快应用调试器"},
            {"org.hapjs.mockup", "快应用预览版"},
            {"com.coolapk.market", "DNA-Android"},
            {"com.atominvention.rootchecker", "Root 测试工具"},
            {"moe.haruue.wadb", "网络 adb 调试"},
            {"io.github.vvb2060.xposeddetector", "密钥认证"},
            {"io.github.huskydg.memorydetector", "MemoryDetector"},
            {"com.byxiaorun.detector", "Ruru"},
            {"icu.nullptr.nativetest", "Native Test"},
            {"io.github.vvb2060.mahoshojo", "Momo"},
            {"luna.safe.luna", "Luna"},

            {"ru.maximoff.apktool", "Apktool"},
            {"top.niunaijun.blackdexa64", "BlackDex64"},
            {"formatfa.xposed.Fdex2", "Fdex2"},
            {"me.weishu.exp", "微术实验"},
            {"org.autojs.autojs", "Auto.js"},
            {"net.dinglisch.android.taskerm", "Tasker"},
            {"app.greyshirts.sslcapture", "SSL Capture"},
            {"cn.trinea.android.developertools", "开发者工具"},
            {"com.lefan.apkanaly", "Lefan APK分析"},

            // VPN与代理工具
            {"com.guoshi.httpcanary", "HttpCanary"},
            {"org.charlesproxy.charles", "Charles代理"},
            {"de.blinkt.openvpn", "OpenVPN"},
            {"net.openvpn.openvpn", "OpenVPN Connect"},
            {"com.github.kr328.clash", "Clash for Android"},
            {"com.crosserr.trojan", "Trojan代理"},
            {"com.qi.tiaozhuan", "跳转工具"},
            {"com.qi.huguanproxy", "代理工具"},
            {"com.qi.staticsproxy", "静态代理"},
            {"com.qi.earthnutproxy", "Earthnut代理"},
            {"com.qi.hjproxy", "HJ代理"},
            {"com.linghang520.iphaidtnet", "领航VPN"},
            {"com.linghang520.iphainet", "领航VPN"},
            {"com.linghang520.jlipnet", "领航VPN"},
            {"com.linghang520.ipmnqdtnet", "领航模拟器VPN"},
            {"com.linghang520.lhdtnet", "领航VPN"},
            {"com.whitebunny.vpn", "WhiteBunny VPN"},
            {"com.fishervpn.freevpn", "Fisher VPN"},
            {"com.fastfun.vpn", "FastFun VPN"},
            {"com.dmvpn.vpnfree", "DM VPN"},
            {"com.daxiang.vpn", "大象VPN"},
            {"com.birdvpn.app", "BirdVPN"},
            {"com.avira.vpn", "Avira VPN"},
            {"cn.hm.vpn", "华盟VPN"},
            {"com.ichano.deepipconverter", "Deep IP转换器"},
            {"com.tuziip.tuzi", "兔子IP"},
            {"tool.seagull.v", "海鸥工具"},
            {"com.shansulian.IP.app", "闪速连IP"},
            {"com.hongtuwuyou.wyip", "宏图无忧IP"},
            {"com.fvcorp.flyclient", "飞鱼客户端"},
            {"com.v2cross.shadowrocket", "Shadowrocket"},
            {"com.v2cross.shadowshare", "Shadowsocks Share"},
            {"com.vpnarea", "VPNArea"},
            {"hideme.android.vpn", "HideMe VPN"},
            {"com.free.vpn.proxy.master.app", "VPN Master"},
            {"com.xiaobei.shenlongjiasu", "神龙加速"},
            {"com.tianqiip.sstp", "天齐IP"},
            {"com.fvcorp.android.aijiasuclient", "爱加速客户端"},
            {"com.lishun.flyfish", "飞鱼VPN"},
            {"com.dongguo.feiyu", "东国飞鱼"},
            {"com.longene.cake", "Cake"},
            {"com.xiyan.xiniu", "西燕犀牛"},
            {"org.skylineacc.android.client", "Skyline ACC"},
            {"com.getlantern.lantern", "Lantern（蓝灯）"},
            {"com.psiphon3.subscription", "Psiphon"},
            {"com.shadowrocket", "Shadowrocket"},
            {"com.qiuyou.network", "秋友网络"},
            {"com.v2ray.angel", "V2Ray Angel"},
            {"com.kiwi.vpn", "Kiwi VPN"},
            {"com.cloudvpn", "Cloud VPN"},
            {"com.coolvpn", "Cool VPN"},
            {"com.yunti", "云梯"},
            {"com.fastvpn", "Fast VPN"},
            {"com.vpn.android", "VPN Android"},
            {"com.accelerator.vpn", "加速器VPN"},
            {"com.earthvpn", "Earth VPN"},
            {"com.rocketvpn", "Rocket VPN"},
            {"com.quickvpn", "Quick VPN"},
            {"com.bigcannon.vpn", "Big Cannon VPN"},
            {"com.cloudshield.vpn", "CloudShield VPN"},
            {"com.securebrowser", "安全浏览器"},
            {"com.freevpn", "Free VPN"},
            {"com.superspeed.vpn", "SuperSpeed VPN"},
            {"com.shieldvpn", "Shield VPN"},
            {"com.invisiblevpn", "Invisible VPN"},
            {"com.freenet", "FreeNet"},
            {"com.aispeed", "AI Speed"},
            {"com.v2ray.ang", "V2RayNG"},
            {"com.expressvpn.vpn", "ExpressVPN"},
            {"ch.protonvpn.android", "ProtonVPN"},
            {"com.github.shadowsocks", "Shadowsocks"},
            {"com.surfshark.vpnclient.android", "Surfshark"},
            {"com.windscribe.vpn", "Windscribe"},
            {"com.nordvpn.android", "NordVPN"},
            {"com.freevpnintouch", "FreeVPNInTouch"},
            {"hotspotshield.android.vpn", "Hotspot Shield"},
            {"com.goldenfrog.vyprvpn.app", "VyprVPN"},
            {"com.xy.vpn", "XY VPN"},
            {"com.wl.ufovpn", "UFO VPN"},
            {"com.ifast.virtualvpn", "iFast Virtual VPN"},
            {"com.suxxt.vpnanonymity", "VPN Anonymity"},
            {"com.njh.biubiu", "Biubiu VPN"},
            {"com.netease.uu", "网易UU加速器"},
            {"com.xiongmao886.tun", "熊猫TUN"},
            {"com.zhima.aurora", "芝麻极光"},

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
        }else if(isAppInstalledByPathHole(package_name.c_str())){
            info[package_name]["risk"] = "error";
            info[package_name]["explain"] = "black package name but install[path hole] " + app_name;
        }else if(isAppInstalledByShellHole(package_name.c_str())){
            info[package_name]["risk"] = "error";
            info[package_name]["explain"] = "black package name but install[shell hole] " + app_name;
        }
    }

    // 检查白名单应用是否未安装
    for (auto &[package_name, app_name] : white_map) {
        bool is_installed_by_context = isAppInstalledByContext(env, context,package_name.c_str());
        bool is_installed_by_path = isAppInstalledByPath(package_name.c_str());
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