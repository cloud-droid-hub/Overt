package com.example.overt.device;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

public class package_info {

//    android.app.ApplicationPackageManager.getLaunchIntentForPackage

    static boolean isInstall(Context context, String packageName){
        Log.e("lxz", "check install " + packageName);
        PackageManager packageManager = context.getPackageManager();

        try {
            ApplicationInfo applicationInfo = packageManager.getApplicationInfo(packageName, PackageManager.GET_ACTIVITIES);
            Log.e("lxz", "check install true");
            // 应用已安装
            return true;
        } catch (PackageManager.NameNotFoundException e) {
            // 应用未安装
            Log.e("lxz", "check install false");
        }

        Intent intent = packageManager.getLaunchIntentForPackage(packageName);
        if (intent != null) {
            // 应用已安装
            Log.e("lxz", "check install true2");
            return true;
        } else {
            // 应用未安装
            Log.e("lxz", "check install false2");
        }

        return false;
    }

    static public String getAppNameByPackageName(Context context, String packageName) {
        PackageManager packageManager = context.getPackageManager();
        try {
            ApplicationInfo applicationInfo = packageManager.getApplicationInfo(packageName, 0);
            return packageManager.getApplicationLabel(applicationInfo).toString();
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
            return "Unknown";
        }
    }

    static public HashMap<String, String> get_package_info(Context context){

        HashMap<String, String> package_info = new HashMap<String, String>();

        HashMap<String, String> black_map = new HashMap<String, String>();
        black_map.put("com.zhenxi.hunter", "Hunter");
        black_map.put("com.juqing.catchpackhelper", "抓包帮手");
        black_map.put("bin.mt.plus", "MT 管理器");
        black_map.put("moe.haruue.wadb", "无线 adb");
        black_map.put("org.hapjs.debuger", "快应用调试器");
        black_map.put("org.hapjs.mockup", "快应用预览版");
        black_map.put("me.bmax.apatch", "Apatch");
        black_map.put("com.coolapk.market", "DNA-Android");
        black_map.put("com.atominvention.rootchecker", "Root 测试工具");
        black_map.put("moe.haruue.wadb", "网络 adb 调试");
        black_map.put("com.topjohnwu.magisk", "Magisk");
        black_map.put("io.github.vvb2060.xposeddetector", "密钥认证");
        black_map.put("io.github.huskydg.memorydetector", "MemoryDetector");
        black_map.put("com.byxiaorun.detector", "Ruru");
        black_map.put("icu.nullptr.nativetest", "Native Test");
        black_map.put("io.github.vvb2060.mahoshojo", "Momo");
        black_map.put("luna.safe.luna", "Luna");
        black_map.put("org.lsposed.manager", "LSPosed");
        black_map.put("de.blinkt.openvpn", "OpenVPN");
        black_map.put("net.openvpn.openvpn", "OpenVPN Connect");
        black_map.put("com.github.kr328.clash", "Clash for Android");

        HashMap<String, String> white_map = new HashMap<String, String>();
        white_map.put("com.tencent.mobileqq", "QQ");
        white_map.put("com.tencent.mm", "微信");
        white_map.put("com.eg.android.AlipayGphone", "支付宝");
        white_map.put("com.ss.android.ugc.aweme", "抖音");

        // 遍历 black_map
        for(String item : black_map.keySet()){
            if(isInstall(context, item)){
                package_info.put(item, black_map.get(item) + " is exist");
            }
        }

        // 遍历 white_map
        for(String item : white_map.keySet()){
            if(!isInstall(context, item)){
                package_info.put(item, white_map.get(item) + " is not exist");
            }
        }

        return package_info;
    }

}
