//
// Created by lxz on 2025/7/17.
//


#include <jni.h>
#include "zLog.h"
#include "zLibc.h"
#include "zJavaVm.h"
#include "zTee.h"
#include "zTeeInfo.h"

// 验证启动状态常量定义
#define VERIFIED_BOOT_STATE_VERIFIED 0      // 已验证状态
#define VERIFIED_BOOT_STATE_SELF_SIGNED 1   // 自签名状态
#define VERIFIED_BOOT_STATE_UNVERIFIED 2    // 未验证状态
#define VERIFIED_BOOT_STATE_FAILED 3        // 验证失败状态

/**
 * 将字节数组转换为十六进制字符串
 * @param data 字节数组指针
 * @param len 数组长度
 * @return 十六进制字符串
 */
string bytes_to_hex(const unsigned char* data, size_t len) {
    string result;
    char buf[3];
    for (size_t i = 0; i < len; ++i) {
        snprintf(buf, sizeof(buf), "%02x", data[i]);
        result += buf;
    }
    return result;
}

/**
 * 通过JNI从Android KeyStore获取认证证书
 * 使用Android KeyStore API生成密钥对并获取证书链
 * @param env JNI环境指针
 * @param context Android上下文对象
 * @return 证书的DER编码字节数组
 */
vector<uint8_t> get_attestation_cert_from_java(JNIEnv* env, jobject context) {
    LOGD("get_attestation_cert_from_java called");
    vector<uint8_t> result;
    LOGI("Start get_attestation_cert_from_java");
    
    // 检查参数有效性
    if (!env || !context) {
        LOGE("env or context is null");
        return result;
    }

    // 步骤1: 获取AndroidKeyStore实例
    jclass clsKeyStore = env->FindClass("java/security/KeyStore");
    LOGD("FindClass KeyStore: %p", clsKeyStore);
    jmethodID midGetInstance = env->GetStaticMethodID(clsKeyStore, "getInstance", "(Ljava/lang/String;)Ljava/security/KeyStore;");
    LOGD("GetMethodID getInstance: %p", midGetInstance);
    jstring jAndroidKeyStore = env->NewStringUTF("AndroidKeyStore");
    jobject keyStore = env->CallStaticObjectMethod(clsKeyStore, midGetInstance, jAndroidKeyStore);
    LOGD("keyStore: %p", keyStore);

    // 步骤2: 加载KeyStore
    jmethodID midLoad = env->GetMethodID(clsKeyStore, "load", "(Ljava/security/KeyStore$LoadStoreParameter;)V");
    LOGD("GetMethodID load: %p", midLoad);
    env->CallVoidMethod(keyStore, midLoad, (jobject)NULL);
    LOGD("keyStore.load(null) called");

    // 步骤3: 创建密钥生成参数构建器
    jclass clsKeyGenBuilder = env->FindClass("android/security/keystore/KeyGenParameterSpec$Builder");
    LOGD("FindClass KeyGenParameterSpec$Builder: %p", clsKeyGenBuilder);
    jstring jAlias = env->NewStringUTF("tee_check_key");
    jclass clsKeyProperties = env->FindClass("android/security/keystore/KeyProperties");
    LOGD("FindClass KeyProperties: %p", clsKeyProperties);
    
    // 获取密钥用途（签名和验证）
    jfieldID fidPurposeSign = env->GetStaticFieldID(clsKeyProperties, "PURPOSE_SIGN", "I");
    jfieldID fidPurposeVerify = env->GetStaticFieldID(clsKeyProperties, "PURPOSE_VERIFY", "I");
    jint purpose = env->GetStaticIntField(clsKeyProperties, fidPurposeSign) | env->GetStaticIntField(clsKeyProperties, fidPurposeVerify);
    LOGD("purpose: %d", purpose);
    
    // 创建构建器实例
    jmethodID midBuilderCtor = env->GetMethodID(clsKeyGenBuilder, "<init>", "(Ljava/lang/String;I)V");
    jobject builder = env->NewObject(clsKeyGenBuilder, midBuilderCtor, jAlias, purpose);
    LOGD("builder: %p", builder);

    // 步骤4: 设置椭圆曲线参数（secp256r1）
    jclass clsECGenParamSpec = env->FindClass("java/security/spec/ECGenParameterSpec");
    LOGD("FindClass ECGenParameterSpec: %p", clsECGenParamSpec);
    jmethodID midECGenCtor = env->GetMethodID(clsECGenParamSpec, "<init>", "(Ljava/lang/String;)V");
    jstring jCurve = env->NewStringUTF("secp256r1");
    jobject ecSpec = env->NewObject(clsECGenParamSpec, midECGenCtor, jCurve);
    LOGD("ecSpec: %p", ecSpec);
    jmethodID midSetAlgParam = env->GetMethodID(clsKeyGenBuilder, "setAlgorithmParameterSpec", "(Ljava/security/spec/AlgorithmParameterSpec;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetAlgParam, ecSpec);
    LOGD("builder after setAlgorithmParameterSpec: %p", builder);

    // 步骤5: 设置摘要算法（SHA256）
    jfieldID fidDigestSHA256 = env->GetStaticFieldID(clsKeyProperties, "DIGEST_SHA256", "Ljava/lang/String;");
    jstring jDigestSHA256 = (jstring)env->GetStaticObjectField(clsKeyProperties, fidDigestSHA256);
    jobjectArray digestArray = env->NewObjectArray(1, env->FindClass("java/lang/String"), nullptr);
    env->SetObjectArrayElement(digestArray, 0, jDigestSHA256);
    jmethodID midSetDigests = env->GetMethodID(clsKeyGenBuilder, "setDigests", "([Ljava/lang/String;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetDigests, digestArray);
    LOGD("builder after setDigests: %p", builder);

    // 步骤6: 设置认证挑战
    const char* challengeStr = "tee_check";
    jbyteArray challenge = env->NewByteArray(strlen(challengeStr));
    env->SetByteArrayRegion(challenge, 0, strlen(challengeStr), (const jbyte*)challengeStr);
    jmethodID midSetChallenge = env->GetMethodID(clsKeyGenBuilder, "setAttestationChallenge", "([B)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetChallenge, challenge);
    LOGD("builder after setAttestationChallenge: %p", builder);

    // 步骤7: 构建密钥生成参数规范
    jmethodID midBuild = env->GetMethodID(clsKeyGenBuilder, "build", "()Landroid/security/keystore/KeyGenParameterSpec;");
    jobject keyGenSpec = env->CallObjectMethod(builder, midBuild);
    LOGD("keyGenSpec: %p", keyGenSpec);

    // 步骤8: 获取椭圆曲线密钥对生成器
    jclass clsKeyPairGen = env->FindClass("java/security/KeyPairGenerator");
    LOGD("FindClass KeyPairGenerator: %p", clsKeyPairGen);
    jmethodID midGetKPG = env->GetStaticMethodID(clsKeyPairGen, "getInstance", "(Ljava/lang/String;Ljava/lang/String;)Ljava/security/KeyPairGenerator;");
    jstring jAlg = env->NewStringUTF("EC");
    jobject kpg = env->CallStaticObjectMethod(clsKeyPairGen, midGetKPG, jAlg, jAndroidKeyStore);
    LOGD("kpg: %p", kpg);

    // 步骤9: 初始化密钥对生成器
    jmethodID midInit = env->GetMethodID(clsKeyPairGen, "initialize", "(Ljava/security/spec/AlgorithmParameterSpec;)V");
    env->CallVoidMethod(kpg, midInit, keyGenSpec);
    LOGD("kpg.initialize called");

    // 步骤10: 生成密钥对
    jmethodID midGenKeyPair = env->GetMethodID(clsKeyPairGen, "generateKeyPair", "()Ljava/security/KeyPair;");
    jobject keyPair = env->CallObjectMethod(kpg, midGenKeyPair);
    LOGD("keyPair: %p", keyPair);

    // 检查密钥对生成是否成功
    if(!keyPair){
        LOGE("generateKeyPair failed");
        env->ExceptionClear();
        return result;
    }

    // 步骤11: 获取证书链
    jmethodID midGetCertChain = env->GetMethodID(clsKeyStore, "getCertificateChain", "(Ljava/lang/String;)[Ljava/security/cert/Certificate;");
    jobjectArray certChain = (jobjectArray)env->CallObjectMethod(keyStore, midGetCertChain, jAlias);
    LOGD("certChain: %p", certChain);
    if (!certChain) return result;
    jobject cert = env->GetObjectArrayElement(certChain, 0);
    LOGD("cert: %p", cert);

    // 步骤12: 获取证书的DER编码
    jclass clsX509 = env->FindClass("java/security/cert/X509Certificate");
    LOGD("FindClass X509Certificate: %p", clsX509);
    jmethodID midGetEncoded = env->GetMethodID(clsX509, "getEncoded", "()[B");
    LOGD("GetMethodID getEncoded: %p", midGetEncoded);
    jbyteArray certBytes = (jbyteArray)env->CallObjectMethod(cert, midGetEncoded);
    LOGD("certBytes: %p", certBytes);

    // 提取证书数据
    if (certBytes && env->GetArrayLength(certBytes) > 0) {
        jsize len = env->GetArrayLength(certBytes);
        result.resize(len);
        env->GetByteArrayRegion(certBytes, 0, len, reinterpret_cast<jbyte*>(result.data()));
        LOGI("Got DER cert, size: %d", (int)len);
        
        // 记录前几个字节用于调试
        if (len > 0) {
            string hex_data = bytes_to_hex(result.data(), len);
            LOGD("certBytes of cert[%d]: %s", len, hex_data.c_str());
        }
    } else {
        LOGE("DER cert is empty");
    }
    LOGD("End get_attestation_cert_from_java");
    return result;
}

/**
 * 使用OpenSSL解析器获取TEE信息的主函数
 * 通过Android KeyStore获取认证证书，然后使用C解析器分析证书内容
 * @param env JNI环境指针
 * @param context Android上下文对象
 * @return 包含TEE检测结果的Map
 */
map<string, map<string, string>> get_tee_info_openssl(JNIEnv* env, jobject context) {
    LOGD("get_tee_info_openssl called");
    map<string, map<string, string>> info;
    
    // 默认设置为错误状态
    info["tee_statue"]["risk"] = "error";
    info["tee_statue"]["explain"] = "tee_statue is damage";

    // 检查参数有效性
    if (!env) {
        LOGE("JNIEnv is null, 请确保JNIEnv可用");
        return info;
    }

    if (!context) {
        LOGE("context is null, 请确保Context可用");
        return info;
    }
    
    // 从Java层获取认证证书
    vector<uint8_t> cert_data = get_attestation_cert_from_java(env, context);
    if (cert_data.empty()) {
        LOGE("获取证书失败");
        return info;
    }
    
    // 测试C解析器解析证书数据
    LOGI("Testing certificate parsing with %zu bytes", cert_data.size());
    
    // 记录证书的前64字节用于调试
    if (cert_data.size() > 0) {
        string hex_data = bytes_to_hex(cert_data.data(), cert_data.size());
        LOGD("certBytes of cert[%d]: %s", cert_data.size(), hex_data.c_str());

        // 分段记录证书数据用于详细调试
        string hex_data_1 = bytes_to_hex(cert_data.data(), 300);
        string hex_data_2 = bytes_to_hex(cert_data.data()+300, 300);
        string hex_data_3 = bytes_to_hex(cert_data.data()+600, 53);

        LOGD("certBytes of cert[300]: %s", hex_data_1.c_str());
        LOGD("certBytes of cert[600]: %s", hex_data_2.c_str());
        LOGD("certBytes of cert[653]: %s", hex_data_3.c_str());
    }
    
    // 使用C解析器解析证书
    tee_info_t tee_info;
    int result = parse_tee_certificate(cert_data.data(), cert_data.size(), &tee_info);
    
    if (result != 0) {
        LOGE("Failed to parse certificate, result: %d", result);
        return info;
    }
    
    LOGI("Successfully parsed certificate");
    LOGI("Security level: %d", tee_info.security_level);
    LOGI("Has attestation extension: %d", tee_info.has_attestation_extension);
    
    // 记录详细的解析结果
    if (tee_info.has_attestation_extension) {
        LOGI("Found TEE attestation extension");
    } else {
        LOGI("No TEE attestation extension found");
    }
    
    // 检查RootOfTrust信息是否有效
    if (tee_info.root_of_trust.valid) {
        LOGI("RootOfTrust: %s, security_level: %d, deviceLocked: %d, verifiedBootState: %d",
             tee_info.root_of_trust.verified_boot_key_hex,
             tee_info.security_level,
             tee_info.root_of_trust.device_locked, 
             tee_info.root_of_trust.verified_boot_state);
        
        // 清空默认错误信息，开始安全检查
        info.clear();

        // 检查设备锁定状态
        if (!tee_info.root_of_trust.device_locked) {
            info["device_locked"]["risk"] = "error";
            info["device_locked"]["explain"] = "device_locked is unsafe";
        }
        
        // 检查验证启动状态
        if (tee_info.root_of_trust.verified_boot_state != VERIFIED_BOOT_STATE_VERIFIED) {
            info["verified_boot_state"]["risk"] = "error";
            info["verified_boot_state"]["explain"] = "verified_boot_state is unsafe";
        }
    } else {
        LOGW("[Native-TEE] RootOfTrust not valid");
    }
    
    return info;
}

/**
 * 向后兼容的TEE信息获取函数
 * @param env JNI环境指针
 * @param context Android上下文对象
 * @return 包含TEE检测结果的Map
 */
map<string, map<string, string>> get_tee_info(JNIEnv* env, jobject context) {
    LOGD("get_tee_info called");
    return get_tee_info_openssl(env, context);
}

/**
 * TEE信息获取的主入口函数
 * 自动获取JNI环境和上下文对象
 * @return 包含TEE检测结果的Map
 */
map<string, map<string, string>> get_tee_info() {
    LOGD("get_tee_info called");
    return get_tee_info_openssl(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
}
