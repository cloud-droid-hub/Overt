//
// Created by lxz on 2025/7/10.
//


#include <jni.h>
#include "system_setting_info.h"
#include "zLog.h"
#include "zJavaVm.h"


bool nativeIsCharging(JNIEnv *env, jobject context) {
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
        LOGE("Device is charging");
        return JNI_TRUE;
    } else {
        LOGE("Device is not charging");
        return JNI_FALSE;
    }
}

jobject constructContext(JNIEnv* env, const char* newAppClassName) {
    jclass clsActivityThread = env->FindClass("android/app/ActivityThread");
    jmethodID m_currentAT = env->GetStaticMethodID(clsActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject at = env->CallStaticObjectMethod(clsActivityThread, m_currentAT);

    jfieldID fid_mBoundApp = env->GetFieldID(clsActivityThread, "mBoundApplication", "Landroid/app/ActivityThread$AppBindData;");
    jobject mBoundApp = env->GetObjectField(at, fid_mBoundApp);

    jclass clsAppBindData = env->FindClass("android/app/ActivityThread$AppBindData");
    jfieldID fid_info = env->GetFieldID(clsAppBindData, "info", "Landroid/app/LoadedApk;");
    jobject loadedApk = env->GetObjectField(mBoundApp, fid_info);

    jclass clsLoadedApk = env->FindClass("android/app/LoadedApk");
    jmethodID m_makeApp = env->GetMethodID(clsLoadedApk, "makeApplication", "(ZLandroid/app/Instrumentation;)Landroid/app/Application;");
    jobject app = env->CallObjectMethod(loadedApk, m_makeApp, JNI_FALSE, nullptr);

    // app.getBaseContext()
    jclass clsApp = env->GetObjectClass(app);
    jmethodID m_getBaseContext = env->GetMethodID(clsApp, "getBaseContext", "()Landroid/content/Context;");
    jobject context = env->CallObjectMethod(app, m_getBaseContext);

    // 转为全局引用后返回
    return env->NewGlobalRef(context);
}

std::map<std::string, std::map<std::string, std::string>> get_system_setting_info(JNIEnv* env) {
    std::map<std::string, std::map<std::string, std::string>> info;

    if (env == nullptr) {
        LOGE("Failed to get JNIEnv");
        return info;
    }

    jobject context =  constructContext(env, "com.example.overt.MainApplication");

    bool isCharging = nativeIsCharging(env, context);

//    bool isCharging = true;

    LOGE("isCharging=%d", isCharging);

    if(isCharging){
        info["battery"]["risk"] = "warn";
        info["battery"]["explain"] = "phone is being charged";
    }

    return info;
}

std::map<std::string, std::map<std::string, std::string>> get_system_setting_info() {
    return get_system_setting_info(zJavaVm::getInstance()->getEnv());
}