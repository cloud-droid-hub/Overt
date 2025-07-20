//
// Created by lxz on 2025/7/17.
//

#include "tee_info.h"

#include <jni.h>
#include "util.h"
#include <vector>
#include <string.h>
#include "zJavaVm.h"
#include <map>
#include <string>
#include <android/log.h>

// Include our C++-based certificate parser
#include "tee_cert_parser.h"

// Define LOGT macro if not already defined
#include "zLog.h"

// Verified boot states
#define VERIFIED_BOOT_STATE_VERIFIED 0
#define VERIFIED_BOOT_STATE_SELF_SIGNED 1
#define VERIFIED_BOOT_STATE_UNVERIFIED 2
#define VERIFIED_BOOT_STATE_FAILED 3

// Convert byte array to hex string
std::string bytes_to_hex(const unsigned char* data, size_t len) {
    std::string result;
    char buf[3];
    for (size_t i = 0; i < len; ++i) {
        snprintf(buf, sizeof(buf), "%02x", data[i]);
        result += buf;
    }
    return result;
}

// Get attestation certificate from Android KeyStore using JNI
std::vector<uint8_t> get_attestation_cert_from_java(JNIEnv* env, jobject context) {
    std::vector<uint8_t> result;
    LOGE("[JNI] Start get_attestation_cert_from_java");

    // 1. KeyStore.getInstance("AndroidKeyStore")
    jclass clsKeyStore = env->FindClass("java/security/KeyStore");
    LOGE("[JNI] FindClass KeyStore: %p", clsKeyStore);
    jmethodID midGetInstance = env->GetStaticMethodID(clsKeyStore, "getInstance", "(Ljava/lang/String;)Ljava/security/KeyStore;");
    LOGE("[JNI] GetStaticMethodID getInstance: %p", midGetInstance);
    jstring jAndroidKeyStore = env->NewStringUTF("AndroidKeyStore");
    jobject keyStore = env->CallStaticObjectMethod(clsKeyStore, midGetInstance, jAndroidKeyStore);
    LOGE("[JNI] keyStore: %p", keyStore);

    // 2. keyStore.load(null)
    jmethodID midLoad = env->GetMethodID(clsKeyStore, "load", "(Ljava/security/KeyStore$LoadStoreParameter;)V");
    LOGE("[JNI] GetMethodID load: %p", midLoad);
    env->CallVoidMethod(keyStore, midLoad, (jobject)NULL);
    LOGE("[JNI] keyStore.load(null) called");

    // 3. KeyGenParameterSpec.Builder(alias, purpose)
    jclass clsKeyGenBuilder = env->FindClass("android/security/keystore/KeyGenParameterSpec$Builder");
    LOGE("[JNI] FindClass KeyGenParameterSpec$Builder: %p", clsKeyGenBuilder);
    jstring jAlias = env->NewStringUTF("tee_check_key");
    jclass clsKeyProperties = env->FindClass("android/security/keystore/KeyProperties");
    LOGE("[JNI] FindClass KeyProperties: %p", clsKeyProperties);
    jfieldID fidPurposeSign = env->GetStaticFieldID(clsKeyProperties, "PURPOSE_SIGN", "I");
    jfieldID fidPurposeVerify = env->GetStaticFieldID(clsKeyProperties, "PURPOSE_VERIFY", "I");
    jint purpose = env->GetStaticIntField(clsKeyProperties, fidPurposeSign) | env->GetStaticIntField(clsKeyProperties, fidPurposeVerify);
    LOGE("[JNI] purpose: %d", purpose);
    jmethodID midBuilderCtor = env->GetMethodID(clsKeyGenBuilder, "<init>", "(Ljava/lang/String;I)V");
    jobject builder = env->NewObject(clsKeyGenBuilder, midBuilderCtor, jAlias, purpose);
    LOGE("[JNI] builder: %p", builder);

    // 4. setAlgorithmParameterSpec(new ECGenParameterSpec("secp256r1"))
    jclass clsECGenParamSpec = env->FindClass("java/security/spec/ECGenParameterSpec");
    LOGE("[JNI] FindClass ECGenParameterSpec: %p", clsECGenParamSpec);
    jmethodID midECGenCtor = env->GetMethodID(clsECGenParamSpec, "<init>", "(Ljava/lang/String;)V");
    jstring jCurve = env->NewStringUTF("secp256r1");
    jobject ecSpec = env->NewObject(clsECGenParamSpec, midECGenCtor, jCurve);
    LOGE("[JNI] ecSpec: %p", ecSpec);
    jmethodID midSetAlgParam = env->GetMethodID(clsKeyGenBuilder, "setAlgorithmParameterSpec", "(Ljava/security/spec/AlgorithmParameterSpec;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetAlgParam, ecSpec);
    LOGE("[JNI] builder after setAlgorithmParameterSpec: %p", builder);

    // 5. setDigests(KeyProperties.DIGEST_SHA256)
    jfieldID fidDigestSHA256 = env->GetStaticFieldID(clsKeyProperties, "DIGEST_SHA256", "Ljava/lang/String;");
    jstring jDigestSHA256 = (jstring)env->GetStaticObjectField(clsKeyProperties, fidDigestSHA256);
    jobjectArray digestArray = env->NewObjectArray(1, env->FindClass("java/lang/String"), nullptr);
    env->SetObjectArrayElement(digestArray, 0, jDigestSHA256);
    jmethodID midSetDigests = env->GetMethodID(clsKeyGenBuilder, "setDigests", "([Ljava/lang/String;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetDigests, digestArray);
    LOGE("[JNI] builder after setDigests: %p", builder);

    // 6. setAttestationChallenge
    const char* challengeStr = "tee_check";
    jbyteArray challenge = env->NewByteArray(strlen(challengeStr));
    env->SetByteArrayRegion(challenge, 0, strlen(challengeStr), (const jbyte*)challengeStr);
    jmethodID midSetChallenge = env->GetMethodID(clsKeyGenBuilder, "setAttestationChallenge", "([B)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    builder = env->CallObjectMethod(builder, midSetChallenge, challenge);
    LOGE("[JNI] builder after setAttestationChallenge: %p", builder);

    // 7. builder.build()
    jmethodID midBuild = env->GetMethodID(clsKeyGenBuilder, "build", "()Landroid/security/keystore/KeyGenParameterSpec;");
    jobject keyGenSpec = env->CallObjectMethod(builder, midBuild);
    LOGE("[JNI] keyGenSpec: %p", keyGenSpec);

    // 8. KeyPairGenerator.getInstance("EC", "AndroidKeyStore")
    jclass clsKeyPairGen = env->FindClass("java/security/KeyPairGenerator");
    LOGE("[JNI] FindClass KeyPairGenerator: %p", clsKeyPairGen);
    jmethodID midGetKPG = env->GetStaticMethodID(clsKeyPairGen, "getInstance", "(Ljava/lang/String;Ljava/lang/String;)Ljava/security/KeyPairGenerator;");
    jstring jAlg = env->NewStringUTF("EC");
    jobject kpg = env->CallStaticObjectMethod(clsKeyPairGen, midGetKPG, jAlg, jAndroidKeyStore);
    LOGE("[JNI] kpg: %p", kpg);

    // 9. kpg.initialize(keyGenSpec)
    jmethodID midInit = env->GetMethodID(clsKeyPairGen, "initialize", "(Ljava/security/spec/AlgorithmParameterSpec;)V");
    env->CallVoidMethod(kpg, midInit, keyGenSpec);
    LOGE("[JNI] kpg.initialize called");

    // 10. kpg.generateKeyPair()
    jmethodID midGenKeyPair = env->GetMethodID(clsKeyPairGen, "generateKeyPair", "()Ljava/security/KeyPair;");
    jobject keyPair = env->CallObjectMethod(kpg, midGenKeyPair);
    LOGE("[JNI] keyPair: %p", keyPair);

    if(!keyPair){
        LOGE("[JNI] generateKeyPair failed");
        env->ExceptionClear();
        return result;
    }

    // 11. keyStore.getCertificateChain("tee_check_key")
    jmethodID midGetCertChain = env->GetMethodID(clsKeyStore, "getCertificateChain", "(Ljava/lang/String;)[Ljava/security/cert/Certificate;");
    jobjectArray certChain = (jobjectArray)env->CallObjectMethod(keyStore, midGetCertChain, jAlias);
    LOGE("[JNI] certChain: %p", certChain);
    if (!certChain) return result;
    jobject cert = env->GetObjectArrayElement(certChain, 0);
    LOGE("[JNI] cert: %p", cert);

    // 12. cert.getEncoded()
    jclass clsX509 = env->FindClass("java/security/cert/X509Certificate");
    LOGE("[JNI] FindClass X509Certificate: %p", clsX509);
    jmethodID midGetEncoded = env->GetMethodID(clsX509, "getEncoded", "()[B");
    LOGE("[JNI] GetMethodID getEncoded: %p", midGetEncoded);
    jbyteArray certBytes = (jbyteArray)env->CallObjectMethod(cert, midGetEncoded);
    LOGE("[JNI] certBytes: %p", certBytes);

    if (certBytes && env->GetArrayLength(certBytes) > 0) {
        jsize len = env->GetArrayLength(certBytes);
        result.resize(len);
        env->GetByteArrayRegion(certBytes, 0, len, reinterpret_cast<jbyte*>(result.data()));
        LOGE("[JNI] Got DER cert, size: %d", (int)len);
        
        // Log the first few bytes to verify data integrity
        if (len > 0) {
            std::string hex_data = bytes_to_hex(result.data(), std::min((size_t)len, (size_t)32));
            LOGE("[JNI] First 32 bytes of cert: %s", hex_data.c_str());
        }
    } else {
        LOGE("[JNI] DER cert is empty");
    }
    LOGE("[JNI] End get_attestation_cert_from_java");
    return result;
}

// Main function to get TEE info using our C parser
std::map<std::string, std::map<std::string, std::string>> get_tee_info_openssl(JNIEnv* env, jobject context) {
    std::map<std::string, std::map<std::string, std::string>> info;
    
    if (!env) {
        LOGE("JNIEnv is null, 请确保JNIEnv可用");
        return info;
    }

    if (!context) {
        LOGE("context is null, 请确保Context可用");
        return info;
    }
    
    // Get attestation certificate from Java
    std::vector<uint8_t> cert_data = get_attestation_cert_from_java(env, context);
    if (cert_data.empty()) {
        LOGE("获取证书失败");
        info["tee_statue"]["risk"] = "error";
        info["tee_statue"]["explain"] = "tee_statue is damage";
        return info;
    }
    
    // Test our C parser with the certificate data
    LOGE("[Native-TEE] Testing certificate parsing with %zu bytes", cert_data.size());
    
    // Log first 64 bytes of certificate for debugging
    if (cert_data.size() > 0) {
        std::string hex_data = bytes_to_hex(cert_data.data(), std::min(cert_data.size(), (size_t)64));
        LOGE("[Native-TEE] Certificate data (first 64 bytes): %s", hex_data.c_str());
    }
    
    // Parse certificate using our C parser
    tee_info_t tee_info;
    int result = parse_tee_certificate(cert_data.data(), cert_data.size(), &tee_info);
    
    if (result != 0) {
        LOGE("[Native-TEE] Failed to parse certificate, result: %d", result);
        return info;
    }
    
    LOGE("[Native-TEE] Successfully parsed certificate");
    LOGE("[Native-TEE] Security level: %d", tee_info.security_level);
    LOGE("[Native-TEE] Has attestation extension: %d", tee_info.has_attestation_extension);
    
    // Log detailed parsing results
    if (tee_info.has_attestation_extension) {
        LOGE("[Native-TEE] Found TEE attestation extension");
    } else {
        LOGE("[Native-TEE] No TEE attestation extension found");
    }
    
    if (tee_info.root_of_trust.valid) {
        LOGE("[Native-TEE] RootOfTrust: %s, security_level: %d, deviceLocked: %d, verifiedBootState: %d",
             tee_info.root_of_trust.verified_boot_key_hex,
             tee_info.security_level,
             tee_info.root_of_trust.device_locked, 
             tee_info.root_of_trust.verified_boot_state);
        // Check security conditions
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
std::map<std::string, std::map<std::string, std::string>> get_tee_info(JNIEnv* env, jobject context) {
    return get_tee_info_openssl(env, context);
}

// Main entry point
std::map<std::string, std::map<std::string, std::string>> get_tee_info() {
    return get_tee_info_openssl(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
}
