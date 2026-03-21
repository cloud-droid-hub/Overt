//
// Created by lxz on 2025/7/10.
//


#include <jni.h>


#include "zLog.h"
#include "zLibc.h"
#include "zJavaVm.h"

#include "zSystemSettingInfo.h"

static inline void delete_local_ref(JNIEnv* env, jobject ref) {
    if (env != nullptr && ref != nullptr) {
        env->DeleteLocalRef(ref);
    }
}

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
    if (env == nullptr || context == nullptr) {
        return "";
    }

    string result;
    jclass contextClass = nullptr;
    jobject packageManager = nullptr;
    jstring packageName = nullptr;
    jclass pmClass = nullptr;
    jstring installer = nullptr;

    do {
        // 获取 Context 类
        contextClass = env->GetObjectClass(context);
        if (contextClass == nullptr) {
            break;
        }

        // 获取 getPackageManager 方法 ID
        jmethodID getPackageManager = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        if (getPackageManager == nullptr) {
            break;
        }
        packageManager = env->CallObjectMethod(context, getPackageManager);
        if (packageManager == nullptr) {
            break;
        }

        // 获取 getPackageName 方法 ID
        jmethodID getPackageName = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
        if (getPackageName == nullptr) {
            break;
        }
        packageName = (jstring)env->CallObjectMethod(context, getPackageName);
        if (packageName == nullptr) {
            break;
        }

        // 获取 PackageManager 类
        pmClass = env->GetObjectClass(packageManager);
        if (pmClass == nullptr) {
            break;
        }

        // 获取 getInstallerPackageName 方法 ID
        jmethodID getInstallerPackageName = env->GetMethodID(pmClass, "getInstallerPackageName", "(Ljava/lang/String;)Ljava/lang/String;");
        if (getInstallerPackageName == nullptr) {
            break;
        }
        installer = (jstring)env->CallObjectMethod(packageManager, getInstallerPackageName, packageName);

        // 如果返回值为 null，说明是 ADB 安装
        if (installer == nullptr) {
            LOGD("installer=null");
        } else {
            const char* installer_cstr = env->GetStringUTFChars(installer, nullptr);
            if (installer_cstr != nullptr) {
                LOGI("installer=%s", installer_cstr);
                result = string(installer_cstr);
                env->ReleaseStringUTFChars(installer, installer_cstr);
            }
        }
    } while (false);

    delete_local_ref(env, installer);
    delete_local_ref(env, pmClass);
    delete_local_ref(env, packageName);
    delete_local_ref(env, packageManager);
    delete_local_ref(env, contextClass);

    return result;
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
    if (sysCls == nullptr) return JNI_FALSE;
    jmethodID getProp = env->GetStaticMethodID(sysCls, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
    if (getProp == nullptr) {
        delete_local_ref(env, sysCls);
        return JNI_FALSE;
    }

    jstring hostKey = env->NewStringUTF("http.proxyHost");
    jstring portKey = env->NewStringUTF("http.proxyPort");
    if (hostKey == nullptr || portKey == nullptr) {
        delete_local_ref(env, hostKey);
        delete_local_ref(env, portKey);
        delete_local_ref(env, sysCls);
        return JNI_FALSE;
    }

    jstring host = (jstring) env->CallStaticObjectMethod(sysCls, getProp, hostKey);
    jstring port = (jstring) env->CallStaticObjectMethod(sysCls, getProp, portKey);

    delete_local_ref(env, hostKey);
    delete_local_ref(env, portKey);

    if (host == nullptr || port == nullptr) {
        delete_local_ref(env, host);
        delete_local_ref(env, port);
        delete_local_ref(env, sysCls);
        return JNI_FALSE;
    }

    const char *hostChars = env->GetStringUTFChars(host, nullptr);
    const char *portChars = env->GetStringUTFChars(port, nullptr);
    if (hostChars == nullptr || portChars == nullptr) {
        if (hostChars != nullptr) {
            env->ReleaseStringUTFChars(host, hostChars);
        }
        if (portChars != nullptr) {
            env->ReleaseStringUTFChars(port, portChars);
        }
        delete_local_ref(env, host);
        delete_local_ref(env, port);
        delete_local_ref(env, sysCls);
        return JNI_FALSE;
    }

    bool result = (strlen(hostChars) > 0 && strlen(portChars) > 0);
    env->ReleaseStringUTFChars(host, hostChars);
    env->ReleaseStringUTFChars(port, portChars);
    delete_local_ref(env, host);
    delete_local_ref(env, port);
    delete_local_ref(env, sysCls);
    return result ? JNI_TRUE : JNI_FALSE;
}

// 判断是否是 VPN
bool isVpnEnable(JNIEnv *env, jobject context) {
    if (env == nullptr || context == nullptr) return JNI_FALSE;

    jclass contextCls = env->GetObjectClass(context);
    if (contextCls == nullptr) return JNI_FALSE;
    jmethodID getSysService = env->GetMethodID(contextCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    if (getSysService == nullptr) {
        delete_local_ref(env, contextCls);
        return JNI_FALSE;
    }
    jstring connStr = env->NewStringUTF("connectivity");
    if (connStr == nullptr) {
        delete_local_ref(env, contextCls);
        return JNI_FALSE;
    }
    jobject connMgr = env->CallObjectMethod(context, getSysService, connStr);
    delete_local_ref(env, connStr);

    if (connMgr == nullptr) {
        delete_local_ref(env, contextCls);
        return JNI_FALSE;
    }

    jclass connCls = env->GetObjectClass(connMgr);
    if (connCls == nullptr) {
        delete_local_ref(env, connMgr);
        delete_local_ref(env, contextCls);
        return JNI_FALSE;
    }

    // 获取所有网络（包含 active network）
    jmethodID getAllNetworks = env->GetMethodID(connCls, "getAllNetworks", "()[Landroid/net/Network;");
    if (getAllNetworks == nullptr) {
        delete_local_ref(env, connCls);
        delete_local_ref(env, connMgr);
        delete_local_ref(env, contextCls);
        return JNI_FALSE;
    }
    jobjectArray networks = static_cast<jobjectArray>(env->CallObjectMethod(connMgr, getAllNetworks));
    if (networks == nullptr) {
        delete_local_ref(env, connCls);
        delete_local_ref(env, connMgr);
        delete_local_ref(env, contextCls);
        return JNI_FALSE;
    }

    jmethodID getCaps = env->GetMethodID(connCls, "getNetworkCapabilities", "(Landroid/net/Network;)Landroid/net/NetworkCapabilities;");
    if (getCaps == nullptr) {
        delete_local_ref(env, networks);
        delete_local_ref(env, connCls);
        delete_local_ref(env, connMgr);
        delete_local_ref(env, contextCls);
        return JNI_FALSE;
    }

    const int TRANSPORT_VPN = 4;
    bool has_vpn = false;

    jsize count = env->GetArrayLength(networks);
    for (jsize i = 0; i < count; i++) {
        jobject network = env->GetObjectArrayElement(networks, i);
        if (network == nullptr) continue;

        jobject caps = env->CallObjectMethod(connMgr, getCaps, network);
        if (caps == nullptr) {
            delete_local_ref(env, network);
            continue;
        }

        jclass capsCls = env->GetObjectClass(caps);
        if (capsCls == nullptr) {
            delete_local_ref(env, caps);
            delete_local_ref(env, network);
            continue;
        }
        jmethodID hasTransport = env->GetMethodID(capsCls, "hasTransport", "(I)Z");
        if (hasTransport == nullptr) {
            delete_local_ref(env, capsCls);
            delete_local_ref(env, caps);
            delete_local_ref(env, network);
            continue;
        }

        if (env->CallBooleanMethod(caps, hasTransport, TRANSPORT_VPN)) {
            has_vpn = true;  // ✓ 检测到 VPN
            delete_local_ref(env, capsCls);
            delete_local_ref(env, caps);
            delete_local_ref(env, network);
            break;
        }
        delete_local_ref(env, capsCls);
        delete_local_ref(env, caps);
        delete_local_ref(env, network);
    }

    delete_local_ref(env, networks);
    delete_local_ref(env, connCls);
    delete_local_ref(env, connMgr);
    delete_local_ref(env, contextCls);
    return has_vpn ? JNI_TRUE : JNI_FALSE;
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

    if (env->PushLocalFrame(512) < 0) {
        LOGE("PushLocalFrame failed");
        return info;
    }

    // 获取电池信息
    bool is_charging = isCharging(env, context);

    // 获取安装者包名
    string installer_name = getInstallerName(env, context);
    bool is_market_installed = !installer_name.empty();
    if (is_market_installed) {
        vector<string> market_name_list = {
                "com.oppo.market",                      // OPPO
                "com.bbk.appstore",                     // VIVO
                "com.xiaomi.market",                    // XIAOMI
                "com.huawei.appmarket",                 // HUAWEI
                "com.hihonor.appmarket",                // HONOR
        };
        is_market_installed = false;
        for (const auto& market_name : market_name_list) {
            if (installer_name == market_name) {
                is_market_installed = true;
                break;
            }
        }
    }

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
        info["installer"]["explain"] = "not install from official app market [" + installer_name + "]";
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

    env->PopLocalFrame(nullptr);
    return info;
}

map<string, map<string, string>> get_system_setting_info() {
    return get_system_setting_info(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
}
