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

// JNI jstring -> nonstd::string(判空 + ExceptionClear,项目风格)
static string jstring_to_string(JNIEnv* env, jstring jstr) {
    if (env == nullptr || jstr == nullptr) return "";
    const char* c = env->GetStringUTFChars(jstr, nullptr);
    if (c == nullptr) {
        env->ExceptionClear();
        return "";   // 获取失败/异常 → 不 Release(规范禁止 Release null)
    }
    string s(c);
    env->ReleaseStringUTFChars(jstr, c);
    return s;
}

// ============================================================
// 数据源1: Settings 全量 dump (content://settings/global|secure|system 三表合并)
// 组内扁平键值对: {"global:adb_enabled": "1", "secure:development_settings_enabled": "1", ...}
// 只上报原始键值对,不做判定;query 被拒(SecurityException) → 清异常 + 整表不上报(纯 dump 不兜底)
// ============================================================
static map<string, string> get_settings_info(JNIEnv* env, jobject context) {
    map<string, string> settings;

    jclass ctxCls = env->GetObjectClass(context);
    if (ctxCls == nullptr) return settings;
    jmethodID getResolver = env->GetMethodID(ctxCls, "getContentResolver",
                                             "()Landroid/content/ContentResolver;");
    if (getResolver == nullptr) { env->ExceptionClear(); env->DeleteLocalRef(ctxCls); return settings; }
    jobject resolver = env->CallObjectMethod(context, getResolver);
    if (resolver == nullptr) { env->ExceptionClear(); env->DeleteLocalRef(ctxCls); return settings; }

    // Uri.parse("content://settings/xxx")
    jclass uriCls = env->FindClass("android/net/Uri");
    if (uriCls == nullptr) { env->ExceptionClear(); env->DeleteLocalRef(ctxCls); return settings; }
    jmethodID uriParse = env->GetStaticMethodID(uriCls, "parse",
                                                "(Ljava/lang/String;)Landroid/net/Uri;");
    if (uriParse == nullptr) { env->ExceptionClear(); env->DeleteLocalRef(uriCls); env->DeleteLocalRef(ctxCls); return settings; }

    // 三张表:global / secure / system(键加表前缀,如 "global:adb_enabled")
    const char* TABLES[][2] = {
        {"content://settings/global", "global"},
        {"content://settings/secure", "secure"},
        {"content://settings/system", "system"},
    };
    for (auto& t : TABLES) {
        jstring jUri = env->NewStringUTF(t[0]);
        if (jUri == nullptr) { env->ExceptionClear(); continue; }
        jobject uri = env->CallStaticObjectMethod(uriCls, uriParse, jUri);
        env->DeleteLocalRef(jUri);
        if (uri == nullptr) { env->ExceptionClear(); continue; }

        // ContentResolver.query(uri, null, null, null, null)
        jclass resolverCls = env->GetObjectClass(resolver);
        jmethodID queryMid = env->GetMethodID(resolverCls, "query",
            "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;");
        if (queryMid == nullptr) { env->ExceptionClear(); continue; }
        env->DeleteLocalRef(resolverCls);
        jobject cursor = env->CallObjectMethod(resolver, queryMid, uri, nullptr, nullptr, nullptr, nullptr);
        env->DeleteLocalRef(uri);
        if (cursor == nullptr) {
            env->ExceptionClear();   // SecurityException / provider 拒绝 → 整表不上报
            continue;
        }

        jclass cursorCls = env->GetObjectClass(cursor);
        jmethodID moveToFirst = env->GetMethodID(cursorCls, "moveToFirst", "()Z");
        jmethodID getColumnIndex = env->GetMethodID(cursorCls, "getColumnIndex",
                                                    "(Ljava/lang/String;)I");
        jmethodID getString = env->GetMethodID(cursorCls, "getString",
                                               "(I)Ljava/lang/String;");
        if (moveToFirst == nullptr || getColumnIndex == nullptr || getString == nullptr) {
            env->ExceptionClear();
            env->DeleteLocalRef(cursorCls);
            env->DeleteLocalRef(cursor);
            continue;
        }
        jstring jNameCol = env->NewStringUTF("name");
        jstring jValueCol = env->NewStringUTF("value");
        jint nameIdx  = env->CallIntMethod(cursor, getColumnIndex, jNameCol);
        jint valueIdx = env->CallIntMethod(cursor, getColumnIndex, jValueCol);
        env->DeleteLocalRef(jNameCol);
        env->DeleteLocalRef(jValueCol);
        if (env->ExceptionCheck()) {   // getColumnIndex 异常 → 清异常,防返回值未定义
            env->ExceptionClear();
            env->DeleteLocalRef(cursorCls);
            env->DeleteLocalRef(cursor);
            continue;
        }
        if (nameIdx < 0 || valueIdx < 0) {
            env->DeleteLocalRef(cursorCls);
            env->DeleteLocalRef(cursor);
            continue;
        }

        if (env->CallBooleanMethod(cursor, moveToFirst)) {
            jmethodID moveToNext = env->GetMethodID(cursorCls, "moveToNext", "()Z");
            do {
                jstring jKey = (jstring)env->CallObjectMethod(cursor, getString, nameIdx);
                jstring jVal = (jstring)env->CallObjectMethod(cursor, getString, valueIdx);
                if (env->ExceptionCheck()) {   // getString 抛异常 → 清异常,防悬挂引用进入下轮
                    env->ExceptionClear();
                    env->DeleteLocalRef(jKey);
                    env->DeleteLocalRef(jVal);
                    continue;
                }
                if (jKey == nullptr || jVal == nullptr) {   // NULL 列 → 跳过该行,不 Delete null
                    env->ExceptionClear();
                    env->DeleteLocalRef(jKey);
                    env->DeleteLocalRef(jVal);
                    continue;
                }
                string key = jstring_to_string(env, jKey);
                string val = jstring_to_string(env, jVal);
                env->DeleteLocalRef(jKey);
                env->DeleteLocalRef(jVal);
                if (!key.empty()) {
                    settings[string(t[1]) + ":" + key] = val;   // 扁平键值对
                }
            } while (env->CallBooleanMethod(cursor, moveToNext));
        }

        jmethodID closeMid = env->GetMethodID(cursorCls, "close", "()V");
        if (closeMid != nullptr) {
            env->CallVoidMethod(cursor, closeMid);
            env->ExceptionClear();
        }
        env->DeleteLocalRef(cursorCls);
        env->DeleteLocalRef(cursor);
    }

    env->DeleteLocalRef(uriCls);
    env->DeleteLocalRef(resolver);
    env->DeleteLocalRef(ctxCls);
    return settings;
}

// ============================================================
// 数据源2: Battery 全量 extras (registerReceiver(null, filter) 拿 sticky Intent)
// 组内扁平键值对: {"status": "2", "level": "50", ...} (Bundle.keySet + toString,禁止强转)
// ============================================================
static map<string, string> get_battery_info(JNIEnv* env, jobject context) {
    map<string, string> battery;

    jclass intentCls = nullptr, filterCls = nullptr, ctxCls = nullptr, bundleCls = nullptr, setCls = nullptr, objCls = nullptr;
    jobject filter = nullptr, intent = nullptr, bundle = nullptr, set = nullptr;
    jobjectArray keys = nullptr;
    // 统一清理:所有中途 return 都经过这里,避免局部引用泄漏
    auto cleanup = [&]() {
        env->DeleteLocalRef(keys);
        env->DeleteLocalRef(set);
        env->DeleteLocalRef(bundle);
        env->DeleteLocalRef(intent);
        env->DeleteLocalRef(filter);
        env->DeleteLocalRef(setCls);
        env->DeleteLocalRef(bundleCls);
        env->DeleteLocalRef(objCls);
        env->DeleteLocalRef(ctxCls);
        env->DeleteLocalRef(filterCls);
        env->DeleteLocalRef(intentCls);
    };

    intentCls = env->FindClass("android/content/Intent");
    if (intentCls == nullptr) { cleanup(); return battery; }
    jfieldID actionField = env->GetStaticFieldID(intentCls, "ACTION_BATTERY_CHANGED", "Ljava/lang/String;");
    if (actionField == nullptr) { env->ExceptionClear(); cleanup(); return battery; }
    jobject action = env->GetStaticObjectField(intentCls, actionField);
    if (action == nullptr) { env->ExceptionClear(); cleanup(); return battery; }

    filterCls = env->FindClass("android/content/IntentFilter");
    jmethodID filterCtor = env->GetMethodID(filterCls, "<init>", "(Ljava/lang/String;)V");
    filter = env->NewObject(filterCls, filterCtor, action);
    env->DeleteLocalRef(action);
    if (filter == nullptr) { env->ExceptionClear(); cleanup(); return battery; }

    ctxCls = env->GetObjectClass(context);
    jmethodID regReceiver = env->GetMethodID(ctxCls, "registerReceiver",
        "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;");
    if (regReceiver == nullptr) { env->ExceptionClear(); cleanup(); return battery; }
    intent = env->CallObjectMethod(context, regReceiver, nullptr, filter);
    env->DeleteLocalRef(filter);   // filter 用完即弃
    filter = nullptr;
    if (intent == nullptr) { env->ExceptionClear(); cleanup(); return battery; }

    jmethodID getExtras = env->GetMethodID(intentCls, "getExtras", "()Landroid/os/Bundle;");
    if (getExtras == nullptr) { env->ExceptionClear(); cleanup(); return battery; }
    bundle = env->CallObjectMethod(intent, getExtras);
    if (bundle == nullptr) { env->ExceptionClear(); cleanup(); return battery; }

    bundleCls = env->GetObjectClass(bundle);
    jmethodID keySet = env->GetMethodID(bundleCls, "keySet", "()Ljava/util/Set;");
    if (keySet == nullptr) { env->ExceptionClear(); cleanup(); return battery; }
    set = env->CallObjectMethod(bundle, keySet);
    if (set == nullptr) { env->ExceptionClear(); cleanup(); return battery; }
    setCls = env->GetObjectClass(set);
    jmethodID toArray = env->GetMethodID(setCls, "toArray", "()[Ljava/lang/Object;");
    if (toArray == nullptr) { env->ExceptionClear(); cleanup(); return battery; }
    keys = (jobjectArray)env->CallObjectMethod(set, toArray);
    if (keys == nullptr) { env->ExceptionClear(); cleanup(); return battery; }
    env->DeleteLocalRef(setCls);
    setCls = nullptr;

    jmethodID bundleGet = env->GetMethodID(bundleCls, "get", "(Ljava/lang/String;)Ljava/lang/Object;");
    objCls = env->FindClass("java/lang/Object");
    jmethodID objToString = env->GetMethodID(objCls, "toString", "()Ljava/lang/String;");
    if (bundleGet == nullptr || objToString == nullptr) { env->ExceptionClear(); cleanup(); return battery; }

    jsize n = env->GetArrayLength(keys);
    for (jsize i = 0; i < n; i++) {
        jstring jKey = (jstring)env->GetObjectArrayElement(keys, i);
        if (jKey == nullptr) continue;
        string key = jstring_to_string(env, jKey);
        jobject val = env->CallObjectMethod(bundle, bundleGet, jKey);
        if (val != nullptr) {
            jstring jValStr = (jstring)env->CallObjectMethod(val, objToString);
            string valStr = jstring_to_string(env, jValStr);
            env->DeleteLocalRef(jValStr);
            if (!key.empty()) {
                battery[key] = valStr;   // 扁平键值对
            }
            env->DeleteLocalRef(val);
        }
        env->DeleteLocalRef(jKey);
    }

    cleanup();
    return battery;
}

// ============================================================
// 数据源3: TelephonyManager 全量原始字段
// 组内扁平键值对: {"getSimState": "5", "getSimOperator": "46000", ...}
// 敏感字段(getSimSerialNumber/getVoiceMailNumber) 权限不足 → ExceptionClear + "denied"
// ============================================================
static map<string, string> get_telephony_info(JNIEnv* env, jobject context) {
    map<string, string> telephony;

    jclass ctxCls = nullptr, tmCls = nullptr;
    jobject tm = nullptr;
    auto cleanup = [&]() {
        env->DeleteLocalRef(tmCls);
        env->DeleteLocalRef(tm);
        env->DeleteLocalRef(ctxCls);
    };

    ctxCls = env->GetObjectClass(context);
    if (ctxCls == nullptr) return telephony;
    jmethodID getService = env->GetMethodID(ctxCls, "getSystemService",
                                            "(Ljava/lang/String;)Ljava/lang/Object;");
    if (getService == nullptr) { env->ExceptionClear(); cleanup(); return telephony; }
    jstring jPhone = env->NewStringUTF("phone");
    tm = env->CallObjectMethod(context, getService, jPhone);
    env->DeleteLocalRef(jPhone);
    if (tm == nullptr) { env->ExceptionClear(); cleanup(); return telephony; }
    tmCls = env->GetObjectClass(tm);

    struct TeleField { const char* method; const char* sig; };
    // int 字段:原始 int 直接上报
    static const TeleField INT_FIELDS[] = {
        {"getSimState", "()I"},
        {"getNetworkType", "()I"},
        {"getDataState", "()I"},
        {"getPhoneType", "()I"},
    };
    for (const auto& f : INT_FIELDS) {
        jmethodID m = env->GetMethodID(tmCls, f.method, f.sig);
        if (m == nullptr) { env->ExceptionClear(); continue; }   // 低版本无此方法 → 跳过
        jint v = env->CallIntMethod(tm, m);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            telephony[f.method] = "denied";
            continue;
        }
        telephony[f.method] = to_string((int)v);
    }
    // string 字段:原始字符串; 权限失败 → "denied"
    static const TeleField STR_FIELDS[] = {
        {"getSimOperator", "()Ljava/lang/String;"},
        {"getSimOperatorName", "()Ljava/lang/String;"},
        {"getSimSerialNumber", "()Ljava/lang/String;"},
        {"getNetworkOperator", "()Ljava/lang/String;"},
        {"getNetworkOperatorName", "()Ljava/lang/String;"},
        {"getVoiceMailNumber", "()Ljava/lang/String;"},
    };
    for (const auto& f : STR_FIELDS) {
        jmethodID m = env->GetMethodID(tmCls, f.method, f.sig);
        if (m == nullptr) { env->ExceptionClear(); continue; }
        jstring jVal = (jstring)env->CallObjectMethod(tm, m);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            telephony[f.method] = "denied";
            continue;
        }
        if (jVal == nullptr) {   // 返回 null → 权限失败/无卡,记 denied 与异常分支一致
            telephony[f.method] = "denied";
            continue;
        }
        telephony[f.method] = jstring_to_string(env, jVal);
        env->DeleteLocalRef(jVal);
    }
    cleanup();
    return telephony;
}

// ============================================================
// 数据源4: 逐网络 transport 明细 (getAllNetworks + getTransportTypes)
// 组内扁平键值对: {"0": "0,1,4", "1": "4", ...}(索引 → transport int 逗号串,不做 VPN 判定)
// ============================================================
static map<string, string> get_network_info(JNIEnv* env, jobject context) {
    map<string, string> network;

    jclass contextCls = nullptr, connCls = nullptr;
    jobject connMgr = nullptr;
    jobjectArray networks = nullptr;
    auto cleanup = [&]() {
        env->DeleteLocalRef(networks);
        env->DeleteLocalRef(connCls);
        env->DeleteLocalRef(connMgr);
        env->DeleteLocalRef(contextCls);
    };

    contextCls = env->GetObjectClass(context);
    if (contextCls == nullptr) return network;
    jmethodID getSysService = env->GetMethodID(contextCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    if (getSysService == nullptr) { env->ExceptionClear(); cleanup(); return network; }
    jstring connStr = env->NewStringUTF("connectivity");
    connMgr = env->CallObjectMethod(context, getSysService, connStr);
    env->DeleteLocalRef(connStr);
    if (connMgr == nullptr) { env->ExceptionClear(); cleanup(); return network; }
    connCls = env->GetObjectClass(connMgr);

    jmethodID getAllNetworks = env->GetMethodID(connCls, "getAllNetworks", "()[Landroid/net/Network;");
    if (getAllNetworks == nullptr) { env->ExceptionClear(); cleanup(); return network; }
    networks = (jobjectArray)env->CallObjectMethod(connMgr, getAllNetworks);
    if (env->ExceptionCheck() || networks == nullptr) { env->ExceptionClear(); cleanup(); return network; }

    jmethodID getCaps = env->GetMethodID(connCls, "getNetworkCapabilities",
                                         "(Landroid/net/Network;)Landroid/net/NetworkCapabilities;");
    if (getCaps == nullptr) { env->ExceptionClear(); cleanup(); return network; }

    jsize count = env->GetArrayLength(networks);
    for (jsize i = 0; i < count; i++) {
        jobject net = env->GetObjectArrayElement(networks, i);
        if (net == nullptr) continue;
        jobject caps = env->CallObjectMethod(connMgr, getCaps, net);
        env->DeleteLocalRef(net);
        if (env->ExceptionCheck()) {   // SecurityException → 清异常跳过,防悬垂引用
            env->ExceptionClear();
            env->DeleteLocalRef(caps);
            continue;
        }
        if (caps == nullptr) { env->ExceptionClear(); continue; }

        jclass capsCls = env->GetObjectClass(caps);
        jmethodID getTransportTypes = env->GetMethodID(capsCls, "getTransportTypes", "()[I");
        if (getTransportTypes == nullptr) {
            env->ExceptionClear();
            env->DeleteLocalRef(capsCls);
            env->DeleteLocalRef(caps);
            continue;
        }
        jintArray types = (jintArray)env->CallObjectMethod(caps, getTransportTypes);
        if (types == nullptr) { env->ExceptionClear(); env->DeleteLocalRef(capsCls); env->DeleteLocalRef(caps); continue; }

        jsize n = env->GetArrayLength(types);
        jint* arr = env->GetIntArrayElements(types, nullptr);
        if (arr == nullptr) {   // 获取失败 → 跳过该网络
            env->ExceptionClear();
            env->DeleteLocalRef(types);
            env->DeleteLocalRef(capsCls);
            env->DeleteLocalRef(caps);
            continue;
        }
        string joined;
        for (jsize k = 0; k < n; k++) {
            if (k) joined += ",";
            joined += to_string((int)arr[k]);   // TRANSPORT_VPN=4 等原始值,判定在 zengine
        }
        env->ReleaseIntArrayElements(types, arr, JNI_ABORT);
        network[to_string((int)i)] = joined;
        env->DeleteLocalRef(types);
        env->DeleteLocalRef(capsCls);
        env->DeleteLocalRef(caps);
    }
    cleanup();
    return network;
}

// ============================================================
// 数据源5: 生物识别 (hasSystemFeature(FINGERPRINT) + canAuthenticate 原始 int)
// 组内扁平键值对: {"fingerprint_feature": "1", "biometric_status": "0"}
// ============================================================
static map<string, string> get_biometric_info(JNIEnv* env, jobject context) {
    map<string, string> biometric;

    jclass ctxCls = env->GetObjectClass(context);
    if (ctxCls == nullptr) return biometric;
    // 5.1 fingerprint_feature: PackageManager.hasSystemFeature("android.hardware.fingerprint")
    jmethodID getPm = env->GetMethodID(ctxCls, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jstring jFeature = env->NewStringUTF("android.hardware.fingerprint");
    jobject pm = env->CallObjectMethod(context, getPm);
    if (env->ExceptionCheck()) {   // getPackageManager 异常 → 显式清,记为未知
        env->ExceptionClear();
        biometric["fingerprint_feature"] = "-1";
        env->DeleteLocalRef(jFeature);
        env->DeleteLocalRef(ctxCls);
        return biometric;
    }
    if (pm != nullptr) {
        jclass pmCls = env->GetObjectClass(pm);
        jmethodID hasFeature = env->GetMethodID(pmCls, "hasSystemFeature", "(Ljava/lang/String;)Z");
        jboolean has = env->CallBooleanMethod(pm, hasFeature, jFeature);
        if (env->ExceptionCheck()) {   // hasSystemFeature 抛异常 → 记为未知
            env->ExceptionClear();
            biometric["fingerprint_feature"] = "-1";
        } else {
            biometric["fingerprint_feature"] = has ? "1" : "0";
        }
        env->DeleteLocalRef(pmCls);
        env->DeleteLocalRef(pm);
    } else {
        env->ExceptionClear();
        biometric["fingerprint_feature"] = "-1";
    }
    env->DeleteLocalRef(jFeature);

    // 5.2 biometric_status: context.getSystemService(BiometricManager.class) + canAuthenticate(STRONG|WEAK)
    jmethodID getService = env->GetMethodID(ctxCls, "getSystemService",
                                            "(Ljava/lang/Class;)Ljava/lang/Object;");
    if (getService == nullptr) { env->ExceptionClear(); env->DeleteLocalRef(ctxCls); return biometric; }
    jclass bmCls = env->FindClass("android/hardware/biometrics/BiometricManager");
    if (bmCls == nullptr) { env->ExceptionClear(); env->DeleteLocalRef(ctxCls); return biometric; }
    jobject bm = env->CallObjectMethod(context, getService, bmCls);
    if (env->ExceptionCheck()) { env->ExceptionClear(); env->DeleteLocalRef(bmCls); env->DeleteLocalRef(ctxCls); return biometric; }
    int status = -1;   // 初始化,防异常路径未赋值
    if (bm == nullptr) {
        status = 11;   // NO_HARDWARE: 该 ROM 无 BiometricManager 服务
    } else {
        jmethodID canAuth = env->GetMethodID(bmCls, "canAuthenticate", "(I)I");
        if (canAuth != nullptr) {
            status = (int)env->CallIntMethod(bm, canAuth, 0x0F | 0xFF);   // STRONG|WEAK
        } else {
            env->ExceptionClear();
            jmethodID canAuthNoArg = env->GetMethodID(bmCls, "canAuthenticate", "()I");
            if (canAuthNoArg != nullptr) {
                status = (int)env->CallIntMethod(bm, canAuthNoArg);   // API 29 降级
            } else {
                env->ExceptionClear();
                status = -1;
            }
        }
        if (env->ExceptionCheck()) { env->ExceptionClear(); status = -1; }
        env->DeleteLocalRef(bm);
    }
    env->DeleteLocalRef(bmCls);
    env->DeleteLocalRef(ctxCls);
    biometric["biometric_status"] = to_string(status);
    return biometric;
}

// ============================================================
// 数据源6: installer(包名原始)
// 组内扁平键值对: {"installer": "com.xxx"} (ADB 安装 → 空,不写键)
// ============================================================
static map<string, string> get_installer_info(JNIEnv* env, jobject context) {
    map<string, string> installer_map;
    if (env == nullptr || context == nullptr) return installer_map;

    string result;
    jclass contextClass = nullptr;
    jobject packageManager = nullptr;
    jstring packageName = nullptr;
    jclass pmClass = nullptr;
    jstring installer = nullptr;

    do {
        contextClass = env->GetObjectClass(context);
        if (contextClass == nullptr) break;
        jmethodID getPackageManager = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        if (getPackageManager == nullptr) break;
        packageManager = env->CallObjectMethod(context, getPackageManager);
        if (packageManager == nullptr) break;
        jmethodID getPackageName = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
        if (getPackageName == nullptr) break;
        packageName = (jstring)env->CallObjectMethod(context, getPackageName);
        if (packageName == nullptr) break;
        pmClass = env->GetObjectClass(packageManager);
        if (pmClass == nullptr) break;
        jmethodID getInstallerPackageName = env->GetMethodID(pmClass, "getInstallerPackageName", "(Ljava/lang/String;)Ljava/lang/String;");
        if (getInstallerPackageName == nullptr) break;
        installer = (jstring)env->CallObjectMethod(packageManager, getInstallerPackageName, packageName);
        if (installer != nullptr) {
            result = jstring_to_string(env, installer);   // null 说明 ADB 安装 → 空串
        }
    } while (false);

    delete_local_ref(env, installer);
    delete_local_ref(env, pmClass);
    delete_local_ref(env, packageName);
    delete_local_ref(env, packageManager);
    delete_local_ref(env, contextClass);

    if (!result.empty()) {
        installer_map["installer"] = result;
    }
    return installer_map;
}

// ============================================================
// 数据源7: keyguard(锁屏密码原始 bool)
// 组内扁平键值对: {"password": "1"/"0" / "-1"(服务拿不到)}
// ============================================================
static map<string, string> get_keyguard_info(JNIEnv* env, jobject context) {
    map<string, string> keyguard;
    jclass contextCls = nullptr, kmCls = nullptr;
    jobject km = nullptr;
    auto cleanup = [&]() {
        env->DeleteLocalRef(km);
        env->DeleteLocalRef(kmCls);
        env->DeleteLocalRef(contextCls);
    };

    contextCls = env->GetObjectClass(context);
    if (contextCls == nullptr) return keyguard;
    jmethodID getSysService = env->GetMethodID(contextCls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    if (getSysService == nullptr) { env->ExceptionClear(); keyguard["password"] = "-1"; cleanup(); return keyguard; }
    jstring keyguardStr = env->NewStringUTF("keyguard");
    km = env->CallObjectMethod(context, getSysService, keyguardStr);
    env->DeleteLocalRef(keyguardStr);
    if (km == nullptr) { env->ExceptionClear(); keyguard["password"] = "-1"; cleanup(); return keyguard; }
    kmCls = env->GetObjectClass(km);
    jmethodID isSecure = env->GetMethodID(kmCls, "isKeyguardSecure", "()Z");
    if (isSecure == nullptr) { env->ExceptionClear(); keyguard["password"] = "-1"; cleanup(); return keyguard; }
    if (env->ExceptionCheck()) { env->ExceptionClear(); keyguard["password"] = "-1"; cleanup(); return keyguard; }
    jboolean result = env->CallBooleanMethod(km, isSecure);
    if (env->ExceptionCheck()) { env->ExceptionClear(); keyguard["password"] = "-1"; cleanup(); return keyguard; }
    keyguard["password"] = result ? "1" : "0";
    cleanup();
    return keyguard;
}

// ============================================================
// 汇聚: zManager 唯一调用点。保持单类别 system_setting_info 不变。
// 每个数据源一个局部 xxx_info 变量(内层 map<string,string>),整体塞进 info["xxx_info"]。
// info 仍是 map<string, map<string, string>>,键 = 数据源组名,值 = 组内扁平键值对。
// ============================================================
map<string, map<string, string>> get_system_setting_info(JNIEnv* env, jobject context) {
    map<string, map<string, string>> info;
    if (env == nullptr || context == nullptr) return info;

    if (env->PushLocalFrame(512) < 0) {
        LOGE("Failed to push local frame");
        return info;
    }

    // 依次调用各子函数,每数据源一组(组内扁平键值对)
    map<string, string> settings_info  = get_settings_info(env, context);   // global/secure/system 三表合并
    info["settings_info"] = settings_info;

    map<string, string> battery_info   = get_battery_info(env, context);
    info["battery_info"] = battery_info;

    map<string, string> telephony_info = get_telephony_info(env, context);
    info["telephony_info"] = telephony_info;

    map<string, string> network_info   = get_network_info(env, context);
    info["network_info"] = network_info;

    map<string, string> biometric_info = get_biometric_info(env, context);
    info["biometric_info"] = biometric_info;

    map<string, string> installer_info = get_installer_info(env, context);
    if (!installer_info.empty()) {
        info["installer_info"] = installer_info;
    }

    map<string, string> keyguard_info  = get_keyguard_info(env, context);
    info["keyguard_info"] = keyguard_info;

    LOGI("system_setting_info raw: settings=%zu battery=%zu telephony=%zu network=%zu biometric=%zu installer=%zu keyguard=%zu",
         settings_info.size(), battery_info.size(), telephony_info.size(),
         network_info.size(), biometric_info.size(), installer_info.size(), keyguard_info.size());

    env->PopLocalFrame(nullptr);
    return info;
}

map<string, map<string, string>> get_system_setting_info() {
    return get_system_setting_info(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
}