//
// Created by lxz on 2025/7/17.
//


#include <map>
#include <jni.h>

#include "zUtil.h"
#include "zJavaVm.h"
#include "tee_cert_parser.h"
#include "zLog.h"
#include "tee_info.h"



#define VERIFIED_BOOT_STATE_VERIFIED 0
#define VERIFIED_BOOT_STATE_SELF_SIGNED 1
#define VERIFIED_BOOT_STATE_UNVERIFIED 2
#define VERIFIED_BOOT_STATE_FAILED 3

// Convert byte array to hex string
string bytes_to_hex(const unsigned char* data, size_t len) {
    string result;
    char buf[3];
    for (size_t i = 0; i < len; ++i) {
        snprintf(buf, sizeof(buf), "%02x", data[i]);
        result += buf;
    }
    return result;
}

// Get attestation certificate from Android KeyStore using JNI
vector<uint8_t> get_attestation_cert_from_java(JNIEnv* env, jobject context) {
    LOGD("[tee_info] get_attestation_cert_from_java called");
    vector<uint8_t> result;
    LOGI("[JNI] Start get_attestation_cert_from_java");
    if (!env || !context) {
        LOGE("[JNI] env or context is null");
        return result;
    }

    // 1. KeyStore.getInstance("AndroidKeyStore")
    jclass clsKeyStore = env->FindClass("java/security/KeyStore");
    LOGD("[JNI] FindClass KeyStore: %p", clsKeyStore);
    jmethodID midGetInstance = env->GetStaticMethodID(clsKeyStore, "getInstance", "(Ljava/lang/String;)Ljava/security/KeyStore;");
    LOGD("[JNI] GetMethodID getInstance: %p", midGetInstance);
    jstring jAndroidKeyStore = env->NewStringUTF("AndroidKeyStore");
    jobject keyStore = env->CallStaticObjectMethod(clsKeyStore, midGetInstance, jAndroidKeyStore);
    LOGD("[JNI] keyStore: %p", keyStore);

    // 2. keyStore.load(null)
    jmethodID midLoad = env->GetMethodID(clsKeyStore, "load", "(Ljava/security/KeyStore$LoadStoreParameter;)V");
    LOGD("[JNI] GetMethodID load: %p", midLoad);
    env->CallVoidMethod(keyStore, midLoad, (jobject)NULL);
    LOGD("[JNI] keyStore.load(null) called");

    // 3. KeyGenParameterSpec.Builder(alias, purpose)
    jclass clsKeyGenBuilder = env->FindClass("android/security/keystore/KeyGenParameterSpec$Builder");
    LOGD("[JNI] FindClass KeyGenParameterSpec$Builder: %p", clsKeyGenBuilder);
    jstring jAlias = env->NewStringUTF("tee_check_key");
    jclass clsKeyProperties = env->FindClass("android/security/keystore/KeyProperties");
    LOGD("[JNI] FindClass KeyProperties: %p", clsKeyProperties);
    jfieldID fidPurposeSign = env->GetStaticFieldID(clsKeyProperties, "PURPOSE_SIGN", "I");
    jfieldID fidPurposeVerify = env->GetStaticFieldID(clsKeyProperties, "PURPOSE_VERIFY", "I");
    jint purpose = env->GetStaticIntField(clsKeyProperties, fidPurposeSign) | env->GetStaticIntField(clsKeyProperties, fidPurposeVerify);
    LOGD("[JNI] purpose: %d", purpose);
    jmethodID midBuilderCtor = env->GetMethodID(clsKeyGenBuilder, "<init>", "(Ljava/lang/String;I)V");
    jobject builder = env->NewObject(clsKeyGenBuilder, midBuilderCtor, jAlias, purpose);
    LOGD("[JNI] builder: %p", builder);

    // 4. setAlgorithmParameterSpec(new ECGenParameterSpec("secp256r1"))
    jclass clsECGenParamSpec = env->FindClass("java/security/spec/ECGenParameterSpec");
    LOGD("[JNI] FindClass ECGenParameterSpec: %p", clsECGenParamSpec);
    jmethodID midECGenCtor = env->GetMethodID(clsECGenParamSpec, "<init>", "(Ljava/lang/String;)V");
    jstring jCurve = env->NewStringUTF("secp256r1");
    jobject ecSpec = env->NewObject(clsECGenParamSpec, midECGenCtor, jCurve);
    LOGD("[JNI] ecSpec: %p", ecSpec);
    jmethodID midSetAlgParam = env->GetMethodID(clsKeyGenBuilder, "setAlgorithmParameterSpec", "(Ljava/security/spec/AlgorithmParameterSpec;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetAlgParam, ecSpec);
    LOGD("[JNI] builder after setAlgorithmParameterSpec: %p", builder);

    // 5. setDigests(KeyProperties.DIGEST_SHA256)
    jfieldID fidDigestSHA256 = env->GetStaticFieldID(clsKeyProperties, "DIGEST_SHA256", "Ljava/lang/String;");
    jstring jDigestSHA256 = (jstring)env->GetStaticObjectField(clsKeyProperties, fidDigestSHA256);
    jobjectArray digestArray = env->NewObjectArray(1, env->FindClass("java/lang/String"), nullptr);
    env->SetObjectArrayElement(digestArray, 0, jDigestSHA256);
    jmethodID midSetDigests = env->GetMethodID(clsKeyGenBuilder, "setDigests", "([Ljava/lang/String;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetDigests, digestArray);
    LOGD("[JNI] builder after setDigests: %p", builder);

    // 6. setAttestationChallenge
    const char* challengeStr = "tee_check";
    jbyteArray challenge = env->NewByteArray(strlen(challengeStr));
    env->SetByteArrayRegion(challenge, 0, strlen(challengeStr), (const jbyte*)challengeStr);
    jmethodID midSetChallenge = env->GetMethodID(clsKeyGenBuilder, "setAttestationChallenge", "([B)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetChallenge, challenge);
    LOGD("[JNI] builder after setAttestationChallenge: %p", builder);

    // 7. builder.build()
    jmethodID midBuild = env->GetMethodID(clsKeyGenBuilder, "build", "()Landroid/security/keystore/KeyGenParameterSpec;");
    jobject keyGenSpec = env->CallObjectMethod(builder, midBuild);
    LOGD("[JNI] keyGenSpec: %p", keyGenSpec);

    // 8. KeyPairGenerator.getInstance("EC", "AndroidKeyStore")
    jclass clsKeyPairGen = env->FindClass("java/security/KeyPairGenerator");
    LOGD("[JNI] FindClass KeyPairGenerator: %p", clsKeyPairGen);
    jmethodID midGetKPG = env->GetStaticMethodID(clsKeyPairGen, "getInstance", "(Ljava/lang/String;Ljava/lang/String;)Ljava/security/KeyPairGenerator;");
    jstring jAlg = env->NewStringUTF("EC");
    jobject kpg = env->CallStaticObjectMethod(clsKeyPairGen, midGetKPG, jAlg, jAndroidKeyStore);
    LOGD("[JNI] kpg: %p", kpg);

    // 9. kpg.initialize(keyGenSpec)
    jmethodID midInit = env->GetMethodID(clsKeyPairGen, "initialize", "(Ljava/security/spec/AlgorithmParameterSpec;)V");
    env->CallVoidMethod(kpg, midInit, keyGenSpec);
    LOGD("[JNI] kpg.initialize called");

    // 10. kpg.generateKeyPair()
    jmethodID midGenKeyPair = env->GetMethodID(clsKeyPairGen, "generateKeyPair", "()Ljava/security/KeyPair;");
    jobject keyPair = env->CallObjectMethod(kpg, midGenKeyPair);
    LOGD("[JNI] keyPair: %p", keyPair);

    if(!keyPair){
        LOGE("[JNI] generateKeyPair failed");
        env->ExceptionClear();
        return result;
    }

    // 11. keyStore.getCertificateChain("tee_check_key")
    jmethodID midGetCertChain = env->GetMethodID(clsKeyStore, "getCertificateChain", "(Ljava/lang/String;)[Ljava/security/cert/Certificate;");
    jobjectArray certChain = (jobjectArray)env->CallObjectMethod(keyStore, midGetCertChain, jAlias);
    LOGD("[JNI] certChain: %p", certChain);
    if (!certChain) return result;
    jobject cert = env->GetObjectArrayElement(certChain, 0);
    LOGD("[JNI] cert: %p", cert);

    // 12. cert.getEncoded()
    jclass clsX509 = env->FindClass("java/security/cert/X509Certificate");
    LOGD("[JNI] FindClass X509Certificate: %p", clsX509);
    jmethodID midGetEncoded = env->GetMethodID(clsX509, "getEncoded", "()[B");
    LOGD("[JNI] GetMethodID getEncoded: %p", midGetEncoded);
    jbyteArray certBytes = (jbyteArray)env->CallObjectMethod(cert, midGetEncoded);
    LOGD("[JNI] certBytes: %p", certBytes);

    if (certBytes && env->GetArrayLength(certBytes) > 0) {
        jsize len = env->GetArrayLength(certBytes);
        result.resize(len);
        env->GetByteArrayRegion(certBytes, 0, len, reinterpret_cast<jbyte*>(result.data()));
        LOGI("[JNI] Got DER cert, size: %d", (int)len);
        
        // Log the first few bytes to verify data integrity
        if (len > 0) {
            string hex_data = bytes_to_hex(result.data(), len);
            LOGD("[JNI] certBytes of cert[%d]: %s", len, hex_data.c_str());
        }
    } else {
        LOGE("[JNI] DER cert is empty");
    }
    LOGD("[JNI] End get_attestation_cert_from_java");
    return result;
}

// Main function to get TEE info using our C parser
map<string, map<string, string>> get_tee_info_openssl(JNIEnv* env, jobject context) {
    LOGD("[tee_info] get_tee_info_openssl called");
    map<string, map<string, string>> info;
    info["tee_statue"]["risk"] = "error";
    info["tee_statue"]["explain"] = "tee_statue is damage";

    if (!env) {
        LOGE("[tee_info] JNIEnv is null, 请确保JNIEnv可用");
        return info;
    }

    if (!context) {
        LOGE("[tee_info] context is null, 请确保Context可用");
        return info;
    }
    
    // Get attestation certificate from Java
    vector<uint8_t> cert_data = get_attestation_cert_from_java(env, context);
    if (cert_data.empty()) {
        LOGE("[tee_info] 获取证书失败");
        return info;
    }
    
    // Test our C parser with the certificate data
    LOGI("[tee_info] Testing certificate parsing with %zu bytes", cert_data.size());
    
    // Log first 64 bytes of certificate for debugging
    if (cert_data.size() > 0) {
        string hex_data = bytes_to_hex(cert_data.data(), cert_data.size());
        LOGD("[JNI] certBytes of cert[%d]: %s", cert_data.size(), hex_data.c_str());

        string hex_data_1 = bytes_to_hex(cert_data.data(), 300);
        string hex_data_2 = bytes_to_hex(cert_data.data()+300, 300);
        string hex_data_3 = bytes_to_hex(cert_data.data()+600, 53);

        LOGD("[JNI] certBytes of cert[300]: %s", hex_data_1.c_str());
        LOGD("[JNI] certBytes of cert[600]: %s", hex_data_2.c_str());
        LOGD("[JNI] certBytes of cert[653]: %s", hex_data_3.c_str());

    }
    
    // Parse certificate using our C parser
    tee_info_t tee_info;
    int result = parse_tee_certificate(cert_data.data(), cert_data.size(), &tee_info);
    
    if (result != 0) {
        LOGE("[tee_info] Failed to parse certificate, result: %d", result);
        return info;
    }
    
    LOGI("[tee_info] Successfully parsed certificate");
    LOGI("[tee_info] Security level: %d", tee_info.security_level);
    LOGI("[tee_info] Has attestation extension: %d", tee_info.has_attestation_extension);
    
    // Log detailed parsing results
    if (tee_info.has_attestation_extension) {
        LOGI("[tee_info] Found TEE attestation extension");
    } else {
        LOGI("[tee_info] No TEE attestation extension found");
    }
    
    if (tee_info.root_of_trust.valid) {
        LOGI("[tee_info] RootOfTrust: %s, security_level: %d, deviceLocked: %d, verifiedBootState: %d",
             tee_info.root_of_trust.verified_boot_key_hex,
             tee_info.security_level,
             tee_info.root_of_trust.device_locked, 
             tee_info.root_of_trust.verified_boot_state);
        // Check security conditions

        info.clear();

        if (!tee_info.root_of_trust.device_locked) {
            info["device_locked"]["risk"] = "error";
            info["device_locked"]["explain"] = "device_locked is unsafe";
        }
        if (tee_info.root_of_trust.verified_boot_state != VERIFIED_BOOT_STATE_VERIFIED) {
            info["verified_boot_state"]["risk"] = "error";
            info["verified_boot_state"]["explain"] = "verified_boot_state is unsafe";
        }
    } else {
        LOGE("[Native-TEE] RootOfTrust not valid");
    }
    
    return info;
}

// Legacy function for backward compatibility
map<string, map<string, string>> get_tee_info(JNIEnv* env, jobject context) {
    LOGD("[tee_info] get_tee_info called");
    return get_tee_info_openssl(env, context);
}

// Main entry point
map<string, map<string, string>> get_tee_info() {
    LOGD("[tee_info] get_tee_info called");
    return get_tee_info_openssl(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
}
