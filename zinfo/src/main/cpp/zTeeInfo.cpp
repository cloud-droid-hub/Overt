//
// Created by lxz on 2025/7/17.
//


#include <jni.h>
#include "zLog.h"
#include "zLibc.h"
#include "zJavaVm.h"
#include "zTee.h"
#include "zTeeInfo.h"
#include "zFile.h"
#include "zHttps.h"
#include "zBase64.h"
#include "zRandom.h"

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
 * 通过JNI从Android KeyStore获取认证证书链(完整链,含中间/根)
 * 使用Android KeyStore API生成密钥对并获取证书链
 * 查杀分离—采集端:返回完整链(不只叶子)与本次随机 challenge
 * @param env JNI环境指针
 * @param context Android上下文对象
 * @param out_challenge [out]本次生成的随机 attestation challenge(供上报校验一致性)
 * @return 证书链的DER编码字节数组,每项一张证书(顺序:叶子→根);空表示失败
 */
vector<vector<uint8_t>> get_attestation_cert_chain_from_java(JNIEnv* env, jobject context,
                                                             vector<uint8_t>* out_challenge) {
    LOGD("get_attestation_cert_chain_from_java called");
    vector<vector<uint8_t>> result;
    if (out_challenge) out_challenge->clear();
    LOGI("Start get_attestation_cert_chain_from_java");

    // 检查参数有效性
    if (!env || !context) {
        LOGE("env or context is null");
        return result;
    }

    struct LocalFrameGuard {
        JNIEnv* env;
        bool active;
        explicit LocalFrameGuard(JNIEnv* e, jint capacity) : env(e), active(false) {
            if (env != nullptr && env->PushLocalFrame(capacity) == 0) {
                active = true;
            }
        }
        ~LocalFrameGuard() {
            if (active && env != nullptr) {
                env->PopLocalFrame(nullptr);
            }
        }
    } local_frame(env, 256);

    if (!local_frame.active) {
        LOGE("PushLocalFrame failed in get_attestation_cert_from_java");
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

    // 步骤6: 设置认证挑战(本次随机生成 16 字节,用于防重放/一致性校验)
    vector<uint8_t> challenge_bytes(16);
    zRandom::getRandomBytes(challenge_bytes);   // zcore 通用随机工具(内核 getrandom + fallback)
    if (out_challenge) *out_challenge = challenge_bytes;
    jbyteArray challenge = env->NewByteArray((jsize)challenge_bytes.size());
    env->SetByteArrayRegion(challenge, 0, (jsize)challenge_bytes.size(),
                            (const jbyte*)challenge_bytes.data());
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

    // 步骤11: 获取完整证书链(叶子→根;verify 需要完整链,不只叶子)
    jmethodID midGetCertChain = env->GetMethodID(clsKeyStore, "getCertificateChain", "(Ljava/lang/String;)[Ljava/security/cert/Certificate;");
    jobjectArray certChain = (jobjectArray)env->CallObjectMethod(keyStore, midGetCertChain, jAlias);
    LOGD("certChain: %p", certChain);
    if (!certChain) return result;

    jsize chainLen = env->GetArrayLength(certChain);
    LOGI("certChain length: %d", (int)chainLen);

    // 步骤12: 遍历整条链,每张证书取 DER 编码(全部收进 result)
    jclass clsX509 = env->FindClass("java/security/cert/X509Certificate");
    LOGD("FindClass X509Certificate: %p", clsX509);
    jmethodID midGetEncoded = env->GetMethodID(clsX509, "getEncoded", "()[B");
    LOGD("GetMethodID getEncoded: %p", midGetEncoded);

    for (jsize i = 0; i < chainLen; i++) {
        jobject cert = env->GetObjectArrayElement(certChain, i);
        if (!cert) {
            LOGE("cert[%d] is null", (int)i);
            continue;
        }
        jbyteArray certBytes = (jbyteArray)env->CallObjectMethod(cert, midGetEncoded);
        if (certBytes && env->GetArrayLength(certBytes) > 0) {
            jsize len = env->GetArrayLength(certBytes);
            vector<uint8_t> der((size_t)len);
            env->GetByteArrayRegion(certBytes, 0, len, reinterpret_cast<jbyte*>(der.data()));
            result.push_back(std::move(der));
            LOGI("Got DER cert[%d], size: %d", (int)i, (int)len);
        } else {
            LOGE("cert[%d] DER is empty", (int)i);
        }
    }
    LOGD("End get_attestation_cert_chain_from_java");
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

    // 检查参数有效性
    if (!env) {
        LOGE("JNIEnv is null, 请确保JNIEnv可用");
        info["tee_state"]["value"] = "env_null";
        return info;
    }

    if (!context) {
        LOGE("context is null, 请确保Context可用");
        info["tee_state"]["value"] = "context_null";
        return info;
    }

    // 从Java层获取完整认证证书链(叶子→根;链供验签,叶子供解析TEE扩展)
    vector<uint8_t> tee_challenge;
    vector<vector<uint8_t>> cert_chain = get_attestation_cert_chain_from_java(env, context, &tee_challenge);
    if (cert_chain.empty()) {
        LOGE("获取证书失败");
        info["tee_state"]["value"] = "cert_empty";
        return info;
    }
    const vector<uint8_t>& cert_data = cert_chain[0]; // 叶子证书(含TEE扩展)

    // 查杀分离—采集端：把完整证书链(base64 编码)作为【原始数据】上报，
    // 验签/解析/判定完全由 zengine(Java)负责；此处不做任何验证。
    // 每条证书 DER → base64，多张用 ',' 拼接，键为 "tee_cert_chain"。
    {
        string chain_b64;
        for (size_t i = 0; i < cert_chain.size(); i++) {
            if (i > 0) chain_b64 += ",";
            chain_b64 += zBase64::encode(cert_chain[i]);   // zcore 通用 base64 工具
        }
        info["tee_cert_chain"]["value"] = chain_b64;
        LOGI("tee_cert_chain base64 len=%zu (certs=%zu)", chain_b64.size(), cert_chain.size());
    }

    // 上报本次随机 challenge(供 zengine 校验证书内 challenge == 本次值)
    info["tee_challenge"]["value"] = zBase64::encode(tee_challenge);
    LOGI("tee_challenge base64 len=%zu", tee_challenge.size());

//    zHttps https_client(5);
//
//    // 测试POST请求
//    LOGI("Testing POST request");
//    HttpsRequest postRequest("http://jiandanyun.myds.me:8086/api/oss/upload", "POST", 5);
//
//    postRequest.headers["Content-Type"] = "application/octet-stream";
//    postRequest.headers["filename"] = "cert_unsafe_1.bin";
//    postRequest.headers["path"] = "/data/cert";
//    postRequest.headers["preserveFilename"] = "true";

//    vector<uint8_t> body = vector<uint8_t>(cert_data.begin(), cert_data.end());

//    postRequest.setBody(body);
//    HttpsResponse postResponse = https_client.performRequest(postRequest);
//    if (!postResponse.error_message.empty()) {
//        LOGW("POST request failed: %s", postResponse.error_message.c_str());
//    } else {
//        LOGI("POST request successful, status: %d", postResponse.status_code);
//        LOGI("POST response body length: %zu", postResponse.body.length());
//        if (!postResponse.body.empty()) {
//            LOGI("POST response body (first 200 chars): %s", postResponse.body.substr(0, 20000).c_str());
//        }
//    }

    // 查杀分离—采集端：完整证书链已在上方作为 tee_cert_chain 上报；
    // 字段解析(device_locked/verified_boot_state/os_version 等)与验签全部在 zengine(Java)。
    // C++ 侧不再解析证书，此处直接结束。
    info["tee_state"]["value"] = "cert_collected";

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
