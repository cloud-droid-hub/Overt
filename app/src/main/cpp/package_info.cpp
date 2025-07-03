//
// Created by lxz on 2025/6/16.
//

#include "package_info.h"

#include "root_file_info.h"
#include "device_info.h"

std::map<std::string, std::string> get_package_info(){

    std::map<std::string, std::string> info;

    const char* paths[] = {
            "/sbin/su",
            "/system/bin/su",
            "/system/xbin/su",
            "/data/local/xbin/su",
            "/data/local/bin/su",
            "/system/sd/xbin/su",
            "/system/bin/failsafe/su",
            "/data/local/su",
            "/system/xbin/mu",
            "/system_ext/bin/su",
            "/apex/com.android.runtime/bin/suu"
    };


    std::map<std::string, std::string> black_map = {
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
            {"com.tencent.mobileqq", "QQ"},
            {"com.tencent.mm", "微信"},
            {"com.eg.android.AlipayGphone", "支付宝"},

    };

    std::map<std::string, std::string> white_map = {
            {"com.tencent.mobileqq", "QQ"},
            {"com.tencent.mm", "微信"},
            {"com.eg.android.AlipayGphone", "支付宝"},
            {"com.ss.android.ugc.aweme", "抖音"},
    };

    for (auto &[key, value] : black_map) {
        std::string path = "/storage/emulated/0/Android/data/" + key;
        if (check_file_exist_2(path)) {
            info[key] = "exist";
        }
    }

    for (auto &[key, value] : white_map) {
        std::string path = "/storage/emulated/0/Android/data/" + key;
        if (!check_file_exist_2(path)) {
            info[key] = "absent";
        }
    }

    return info;
}