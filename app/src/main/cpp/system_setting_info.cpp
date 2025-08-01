//
// Created by lxz on 2025/7/10.
//


#include <jni.h>


#include "zLog.h"
#include "zJavaVm.h"
#include "zUtil.h"
#include "system_setting_info.h"

bool isCharging(JNIEnv *env, jobject context) {
    if (context == nullptr) {
        LOGE("Failed to get context");
        return JNI_FALSE;
    }

    // 获取 Intent.ACTION_BATTERY_CHANGED
    jclass intentClass = env->FindClass("android/content/Intent");
    jfieldID actionBatteryChangedField = env->GetStaticFieldID(intentClass, "ACTION_BATTERY_CHANGED", "Ljava/lang/String;");
    jobject actionBatteryChanged = env->GetStaticObjectField(intentClass, actionBatteryChangedField);

    // 创建 IntentFilter 对象
    jclass intentFilterClass = env->FindClass("android/content/IntentFilter");
    jmethodID intentFilterConstructor = env->GetMethodID(intentFilterClass, "<init>", "(Ljava/lang/String;)V");
    jobject intentFilter = env->NewObject(intentFilterClass, intentFilterConstructor, actionBatteryChanged);

    // 调用 context.registerReceiver(null, intentFilter)
    jclass contextClass = env->GetObjectClass(context);
    jmethodID registerReceiverMethod = env->GetMethodID(contextClass, "registerReceiver",
                                                        "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;");
    jobject batteryStatusIntent = env->CallObjectMethod(context, registerReceiverMethod, nullptr, intentFilter);
    if (batteryStatusIntent == nullptr) {
        LOGE("Failed to register receiver for battery status");
        return JNI_FALSE;
    }

    // 获取 BatteryManager.EXTRA_STATUS 常量
    jclass batteryManagerClass = env->FindClass("android/os/BatteryManager");
    jfieldID extraStatusField = env->GetStaticFieldID(batteryManagerClass, "EXTRA_STATUS", "Ljava/lang/String;");
    jobject extraStatusStr = env->GetStaticObjectField(batteryManagerClass, extraStatusField);

    // 调用 batteryStatusIntent.getIntExtra
    jmethodID getIntExtraMethod = env->GetMethodID(intentClass, "getIntExtra", "(Ljava/lang/String;I)I");
    jint status = env->CallIntMethod(batteryStatusIntent, getIntExtraMethod, extraStatusStr, -1);

    // 获取 BATTERY_STATUS_CHARGING 和 BATTERY_STATUS_FULL 值
    jfieldID chargingField = env->GetStaticFieldID(batteryManagerClass, "BATTERY_STATUS_CHARGING", "I");
    jfieldID fullField = env->GetStaticFieldID(batteryManagerClass, "BATTERY_STATUS_FULL", "I");
    jint BATTERY_STATUS_CHARGING = env->GetStaticIntField(batteryManagerClass, chargingField);
    jint BATTERY_STATUS_FULL = env->GetStaticIntField(batteryManagerClass, fullField);

    // 判断状态
    if (status == BATTERY_STATUS_CHARGING || status == BATTERY_STATUS_FULL) {
        LOGI("Device is charging");
        return JNI_TRUE;
    } else {
        LOGI("Device is not charging");
        return JNI_FALSE;
    }
}

string getInstallerName(JNIEnv *env, jobject context) {
    // 获取 Context 类
    jclass contextClass = env->GetObjectClass(context);

    // 获取 getPackageManager 方法 ID
    jmethodID getPackageManager = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject packageManager = env->CallObjectMethod(context, getPackageManager);

    // 获取 getPackageName 方法 ID
    jmethodID getPackageName = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
    jstring packageName = (jstring)env->CallObjectMethod(context, getPackageName);

    // 获取 PackageManager 类
    jclass pmClass = env->GetObjectClass(packageManager);

    // 获取 getInstallerPackageName 方法 ID
    jmethodID getInstallerPackageName = env->GetMethodID(pmClass, "getInstallerPackageName", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring installer = (jstring)env->CallObjectMethod(packageManager, getInstallerPackageName, packageName);

    // 如果返回值为 null，说明是 ADB 安装
    if (installer == nullptr) {
        LOGD("installer=null");
        return "";
    } else {
        const char* installer_cstr = env->GetStringUTFChars(installer, 0);
        LOGI("installer=%s", installer_cstr);
        return string(installer_cstr);
    }
}

bool isMarketInstalled(JNIEnv *env, jobject context){
    string installer_name =  getInstallerName(env, context);
    if(installer_name.empty()) {
        return false;
    }

    vector<string> market_name_list = {
            "com.oppo.market",                      // OPPO
            "com.bbk.appstore",                     // VIVO
            "com.xiaomi.market",                    // XIAOMI
            "com.huawei.appmarket",                 // HUAWEI
            "com.hihonor.appmarket",                // HONOR
    };

    for(auto market_name : market_name_list) {
        if(installer_name==market_name) {
            return true;
        }
    }

    return false;
}


// 判断 SIM 是否存在
bool isSimExist(JNIEnv *env, jobject context) {
    jclass contextCls = env->GetObjectClass(context);
    jmethodID getSysService = env->GetMethodID(contextCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring telephonyStr = env->NewStringUTF("phone");
    jobject tm = env->CallObjectMethod(context, getSysService, telephonyStr);
    env->DeleteLocalRef(telephonyStr);

    if (tm == nullptr) return JNI_FALSE;

    jclass tmCls = env->GetObjectClass(tm);
    jmethodID getSimState = env->GetMethodID(tmCls, "getSimState", "()I");
    jint simState = env->CallIntMethod(tm, getSimState);
    const int SIM_STATE_ABSENT = 1;
    const int SIM_STATE_UNKNOWN = 0;
    return (simState != SIM_STATE_ABSENT && simState != SIM_STATE_UNKNOWN) ? JNI_TRUE : JNI_FALSE;
}

// 获取开发者选项是否开启
bool isDeveloperModeEnabled(JNIEnv *env, jobject context) {
    jclass settingsClass = env->FindClass("android/provider/Settings$Global");
    jmethodID getInt = env->GetStaticMethodID(settingsClass, "getInt",
                                              "(Landroid/content/ContentResolver;Ljava/lang/String;I)I");

    jclass contextCls = env->GetObjectClass(context);
    jmethodID getContentResolver = env->GetMethodID(contextCls, "getContentResolver", "()Landroid/content/ContentResolver;");
    jobject resolver = env->CallObjectMethod(context, getContentResolver);

    jstring key = env->NewStringUTF("development_settings_enabled");
    jint value = env->CallStaticIntMethod(settingsClass, getInt, resolver, key, 0);
    env->DeleteLocalRef(key);
    return value == 1 ? JNI_TRUE : JNI_FALSE;
}

// USB调试是否开启
bool isUsbDebugEnabled(JNIEnv *env, jobject context) {
    jclass settingsClass = env->FindClass("android/provider/Settings$Global");
    jmethodID getInt = env->GetStaticMethodID(settingsClass, "getInt",
                                              "(Landroid/content/ContentResolver;Ljava/lang/String;I)I");

    jclass contextCls = env->GetObjectClass(context);
    jmethodID getContentResolver = env->GetMethodID(contextCls, "getContentResolver", "()Landroid/content/ContentResolver;");
    jobject resolver = env->CallObjectMethod(context, getContentResolver);

    jstring key = env->NewStringUTF("adb_enabled");
    jint value = env->CallStaticIntMethod(settingsClass, getInt, resolver, key, 0);
    env->DeleteLocalRef(key);
    return value == 1 ? JNI_TRUE : JNI_FALSE;
}


// 系统是否配置了代理
bool isProxyEnabled(JNIEnv *env, jobject context) {
    jclass sysCls = env->FindClass("java/lang/System");
    jmethodID getProp = env->GetStaticMethodID(sysCls, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");

    jstring hostKey = env->NewStringUTF("http.proxyHost");
    jstring portKey = env->NewStringUTF("http.proxyPort");

    jstring host = (jstring) env->CallStaticObjectMethod(sysCls, getProp, hostKey);
    jstring port = (jstring) env->CallStaticObjectMethod(sysCls, getProp, portKey);

    env->DeleteLocalRef(hostKey);
    env->DeleteLocalRef(portKey);

    if (host == nullptr || port == nullptr) return JNI_FALSE;
    const char *hostChars = env->GetStringUTFChars(host, nullptr);
    const char *portChars = env->GetStringUTFChars(port, nullptr);
    bool result = (strlen(hostChars) > 0 && strlen(portChars) > 0);
    env->ReleaseStringUTFChars(host, hostChars);
    env->ReleaseStringUTFChars(port, portChars);
    return result ? JNI_TRUE : JNI_FALSE;
}

// 判断是否是 VPN
bool isVpnEnable(JNIEnv *env, jobject context) {
    jclass contextCls = env->GetObjectClass(context);
    jmethodID getSysService = env->GetMethodID(contextCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring connStr = env->NewStringUTF("connectivity");
    jobject connMgr = env->CallObjectMethod(context, getSysService, connStr);
    env->DeleteLocalRef(connStr);

    if (connMgr == nullptr) return JNI_FALSE;

    jclass connCls = env->GetObjectClass(connMgr);
    jmethodID getActiveNetwork = env->GetMethodID(connCls, "getActiveNetwork", "()Landroid/net/Network;");
    jobject network = env->CallObjectMethod(connMgr, getActiveNetwork);
    if (network == nullptr) return JNI_FALSE;

    jmethodID getCaps = env->GetMethodID(connCls, "getNetworkCapabilities", "(Landroid/net/Network;)Landroid/net/NetworkCapabilities;");
    jobject caps = env->CallObjectMethod(connMgr, getCaps, network);
    if (caps == nullptr) return JNI_FALSE;

    jclass capsCls = env->GetObjectClass(caps);
    jmethodID hasTransport = env->GetMethodID(capsCls, "hasTransport", "(I)Z");
    const int TRANSPORT_VPN = 4;
    jboolean result = env->CallBooleanMethod(caps, hasTransport, TRANSPORT_VPN);
    return result;
}

// 判断是否设置锁屏密码
bool isPasswordLocked(JNIEnv *env, jobject context) {
    jclass contextCls = env->GetObjectClass(context);
    jmethodID getSysService = env->GetMethodID(contextCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring keyguardStr = env->NewStringUTF("keyguard");
    jobject km = env->CallObjectMethod(context, getSysService, keyguardStr);
    env->DeleteLocalRef(keyguardStr);

    if (km == nullptr) return JNI_FALSE;

    jclass kmCls = env->GetObjectClass(km);
    jmethodID isSecure = env->GetMethodID(kmCls, "isKeyguardSecure", "()Z");
    jboolean result = env->CallBooleanMethod(km, isSecure);
    return result;
}

map<string, map<string, string>> get_system_setting_info(JNIEnv* env, jobject context) {
    map<string, map<string, string>> info;

    if (env == nullptr) {
        LOGE("Failed to get env");
        return info;
    }

    if (context == nullptr) {
        LOGE("Failed to get context");
        return info;
    }

    // 获取电池信息
    bool is_charging = isCharging(env, context);

    // 获取安装者包名
    bool is_market_installed = isMarketInstalled(env, context);

    // 获取 SIM 卡信息
    bool is_sim_exist = isSimExist(env, context);

    // 获取开发者模式信息
    bool is_developer_mode_enabled = isDeveloperModeEnabled(env, context);

    // 获取 USB 调试信息
    bool is_usb_debug_enabled = isUsbDebugEnabled(env, context);

    // 获取代理信息
    bool is_proxy_enabled = isProxyEnabled(env, context);

    // 获取锁屏密码信息
    bool is_password_locked = isPasswordLocked(env, context);

    // 获取 VPN 信息
    bool is_vpn_enable = isVpnEnable(env, context);


            LOGI("is_charging=%d", is_charging);
    if(is_charging){
        info["battery"]["risk"] = "warn";
        info["battery"]["explain"] = "phone is being charged";
    }

            LOGI("isMarketInstalled=%d", is_market_installed);
    if(!is_market_installed){
        info["installer"]["risk"] = "warn";
        info["installer"]["explain"] = "not install from official app market [" + getInstallerName(env, context) + "]";
    }

            LOGI("is_sim_exist=%d", is_sim_exist);
    if(!is_sim_exist){
        info["sim"]["risk"] = "error";
        info["sim"]["explain"] = "no sim card";
    }

            LOGI("is_developer_mode_enabled=%d", is_developer_mode_enabled);
    if(is_developer_mode_enabled){
        info["developer_mode"]["risk"] = "error";
        info["developer_mode"]["explain"] = "developer mode is enabled";
    }

            LOGI("is_usb_debug_enabled=%d", is_usb_debug_enabled);
    if(is_usb_debug_enabled){
        info["usb_debug"]["risk"] = "error";
        info["usb_debug"]["explain"] = "usb debugging is enabled";
    }

            LOGI("is_proxy_enabled=%d", is_proxy_enabled);
    if(is_proxy_enabled){
        info["proxy"]["risk"] = "error";
        info["proxy"]["explain"] = "proxy is enabled";
    }

            LOGI("is_password_locked=%d", is_password_locked);
    if(!is_password_locked){
        info["password"]["risk"] = "warn";
        info["password"]["explain"] = "lock screen password is not set";
    }

            LOGI("is_vpn_enable=%d", is_vpn_enable);
    if(is_vpn_enable){
        info["vpn"]["risk"] = "error";
        info["vpn"]["explain"] = "vpn is enable";
    }

    return info;
}

map<string, map<string, string>> get_system_setting_info() {
    return get_system_setting_info(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
}