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



jbyteArray Asn1Utils_getByteArrayFromAsn1(JNIEnv *env, jobject asn1Encodable) {
    jclass cls_DEROctetString = env->FindClass("org/bouncycastle/asn1/DEROctetString");

    if (!env->IsInstanceOf(asn1Encodable, cls_DEROctetString)) {
        jclass exception = env->FindClass("java/security/cert/CertificateParsingException");
        env->ThrowNew(exception, "Expected DEROctetString");
        return nullptr;
    }

    jmethodID mid_getOctets = env->GetMethodID(cls_DEROctetString, "getOctets", "()[B");
    return (jbyteArray) env->CallObjectMethod(asn1Encodable, mid_getOctets);
}

jint Asn1Utils_getIntegerFromAsn1(JNIEnv *env, jobject asn1Value) {
    jclass cls_ASN1Integer = env->FindClass("org/bouncycastle/asn1/ASN1Integer");
    jclass cls_ASN1Enumerated = env->FindClass("org/bouncycastle/asn1/ASN1Enumerated");

    if (env->IsInstanceOf(asn1Value, cls_ASN1Integer) || env->IsInstanceOf(asn1Value, cls_ASN1Enumerated)) {
        jmethodID mid_getValue = env->GetMethodID(env->GetObjectClass(asn1Value), "getValue", "()Ljava/math/BigInteger;");
        jobject bigInt = env->CallObjectMethod(asn1Value, mid_getValue);
        jclass cls_BigInteger = env->FindClass("java/math/BigInteger");
        jmethodID intValue = env->GetMethodID(cls_BigInteger, "intValue", "()I");
        return env->CallIntMethod(bigInt, intValue);
    }

    jclass exception = env->FindClass("java/security/cert/CertificateParsingException");
    env->ThrowNew(exception, "Integer value expected");
    return -1;
}

jboolean Asn1Utils_getBooleanFromAsn1(JNIEnv *env, jobject value) {
    jclass cls_ASN1Boolean = env->FindClass("org/bouncycastle/asn1/ASN1Boolean");
    if (!env->IsInstanceOf(value, cls_ASN1Boolean)) {
        jclass exception = env->FindClass("java/security/cert/CertificateParsingException");
        env->ThrowNew(exception, "Expected ASN1Boolean");
        return JNI_FALSE;
    }

    jfieldID fid_TRUE = env->GetStaticFieldID(cls_ASN1Boolean, "TRUE", "Lorg/bouncycastle/asn1/ASN1Boolean;");
    jfieldID fid_FALSE = env->GetStaticFieldID(cls_ASN1Boolean, "FALSE", "Lorg/bouncycastle/asn1/ASN1Boolean;");

    jobject trueObj = env->GetStaticObjectField(cls_ASN1Boolean, fid_TRUE);
    jobject falseObj = env->GetStaticObjectField(cls_ASN1Boolean, fid_FALSE);

    if (env->IsSameObject(value, trueObj)) {
        return JNI_TRUE;
    } else if (env->IsSameObject(value, falseObj)) {
        return JNI_FALSE;
    } else {
        jclass exception = env->FindClass("java/security/cert/CertificateParsingException");
        env->ThrowNew(exception, "Invalid ASN1Boolean value");
        return JNI_FALSE;
    }
}

// 修改：增加 context 参数
static std::vector<uint8_t> get_attestation_cert_from_java(JNIEnv* env, jobject context) {
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
    } else {
        LOGE("[JNI] DER cert is empty");
    }
    LOGE("[JNI] End get_attestation_cert_from_java");
    return result;
}

// 获取 attestationRecord ASN1Sequence
jobject get_attestation_record_asn1_sequence(JNIEnv* env, const std::vector<uint8_t>& cert) {
    // 1. 用 CertificateFactory 解析 DER 证书为 X509Certificate
    jclass clsCertFactory = env->FindClass("java/security/cert/CertificateFactory");
    jmethodID midGetInstance = env->GetStaticMethodID(clsCertFactory, "getInstance", "(Ljava/lang/String;)Ljava/security/cert/CertificateFactory;");
    jstring jX509 = env->NewStringUTF("X.509");
    jobject certFactory = env->CallStaticObjectMethod(clsCertFactory, midGetInstance, jX509);
    jclass clsByteArrayIS = env->FindClass("java/io/ByteArrayInputStream");
    jmethodID midBAISCtor = env->GetMethodID(clsByteArrayIS, "<init>", "([B)V");
    jbyteArray jCertBytes = env->NewByteArray(cert.size());
    env->SetByteArrayRegion(jCertBytes, 0, cert.size(), (const jbyte*)cert.data());
    jobject inputStream = env->NewObject(clsByteArrayIS, midBAISCtor, jCertBytes);
    jmethodID midGenCert = env->GetMethodID(clsCertFactory, "generateCertificate", "(Ljava/io/InputStream;)Ljava/security/cert/Certificate;");
    jobject x509Cert = env->CallObjectMethod(certFactory, midGenCert, inputStream);
    jclass clsX509 = env->FindClass("java/security/cert/X509Certificate");
    if (!env->IsInstanceOf(x509Cert, clsX509)) {
        LOGE("[JNI] x509Cert is not instance of X509Certificate!");
        return nullptr;
    }
    // 2. getExtensionValue("1.3.6.1.4.1.11129.2.1.17")
    jmethodID midGetExt = env->GetMethodID(clsX509, "getExtensionValue", "(Ljava/lang/String;)[B");
    jstring jOid = env->NewStringUTF("1.3.6.1.4.1.11129.2.1.17");
    jbyteArray extBytes = (jbyteArray)env->CallObjectMethod(x509Cert, midGetExt, jOid);
    if (!extBytes || env->GetArrayLength(extBytes) == 0) {
        LOGE("[JNI] 未找到attestation extension");
        return nullptr;
    }
    // 3. ASN1InputStream asn1InputStream = new ASN1InputStream(new ByteArrayInputStream(extBytes));
    jclass clsASN1InputStream = env->FindClass("org/bouncycastle/asn1/ASN1InputStream");
    jobject extInputStream = env->NewObject(clsByteArrayIS, midBAISCtor, extBytes);
    jmethodID midASN1ISCtor = env->GetMethodID(clsASN1InputStream, "<init>", "(Ljava/io/InputStream;)V");
    jobject asn1InputStream = env->NewObject(clsASN1InputStream, midASN1ISCtor, extInputStream);
    jmethodID midReadObject = env->GetMethodID(clsASN1InputStream, "readObject", "()Lorg/bouncycastle/asn1/ASN1Primitive;");
    jobject asn1Obj = env->CallObjectMethod(asn1InputStream, midReadObject); // ASN1OctetString

    jbyteArray octets = Asn1Utils_getByteArrayFromAsn1(env, asn1Obj);
    if (!octets || env->GetArrayLength(octets) == 0) {
        LOGE("[JNI] octets is empty!");
        return nullptr;
    }
    // 4. ASN1InputStream asn1InputStream2 = new ASN1InputStream(new ByteArrayInputStream(octets));
    jobject octetInputStream = env->NewObject(clsByteArrayIS, midBAISCtor, octets);
    jobject asn1InputStream2 = env->NewObject(clsASN1InputStream, midASN1ISCtor, octetInputStream);
    jobject asn1SeqObj = env->CallObjectMethod(asn1InputStream2, midReadObject); // ASN1Sequence
    jclass clsASN1Sequence = env->FindClass("org/bouncycastle/asn1/ASN1Sequence");
    if (!env->IsInstanceOf(asn1SeqObj, clsASN1Sequence)) {
        LOGE("[JNI] asn1SeqObj is not ASN1Sequence!");
        return nullptr;
    }
    return asn1SeqObj;
}

// 获取 asn1Sequence.getObjectAt(id)
jobject get_asn1_sequence_object_at(JNIEnv* env, jobject asn1Sequence, int id) {
    if (!asn1Sequence) {
        LOGE("[JNI] get_asn1_sequence_object_at: asn1Sequence is null!");
        return nullptr;
    }
    jclass clsASN1Sequence = env->FindClass("org/bouncycastle/asn1/ASN1Sequence");
    jmethodID midGetObjectAt = env->GetMethodID(clsASN1Sequence, "getObjectAt", "(I)Lorg/bouncycastle/asn1/ASN1Encodable;");
    jobject obj = env->CallObjectMethod(asn1Sequence, midGetObjectAt, id);
    LOGE("[JNI] get_asn1_sequence_object_at: id=%d, obj=%p", id, obj);
    return obj;
}

// 获取安全级别（securityLevel）
int get_security_level_from_asn1_sequence(JNIEnv* env, jobject attestation_1) {
    if (!attestation_1) {
        LOGE("[JNI] get_security_level_from_asn1_sequence: input is null!");
        return -1;
    }
    jclass clsASN1Integer = env->FindClass("org/bouncycastle/asn1/ASN1Integer");
    jclass clsASN1Enumerated = env->FindClass("org/bouncycastle/asn1/ASN1Enumerated");
    if (env->IsInstanceOf(attestation_1, clsASN1Integer) || env->IsInstanceOf(attestation_1, clsASN1Enumerated)) {
        int securityLevel = Asn1Utils_getIntegerFromAsn1(env, attestation_1);
        LOGE("[JNI] get_security_level_from_asn1_sequence: %d", securityLevel);
        return securityLevel;
    } else {
        LOGE("[JNI] get_security_level_from_asn1_sequence: not ASN1Integer or ASN1Enumerated!");
        return -1;
    }
}

// 从 AuthorizationList (ASN1Sequence) 解析 RootOfTrust 的 ASN1Primitive value
jobject get_root_of_trust_primitive_from_authorization_list(JNIEnv* env, jobject attestation_n) {
    if (!attestation_n) {
        LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: input is null!");
        return nullptr;
    }
    LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: start, attestation_6=%p", attestation_n);
    jclass clsASN1Sequence = env->FindClass("org/bouncycastle/asn1/ASN1Sequence");
    jmethodID midSize = env->GetMethodID(clsASN1Sequence, "size", "()I");
    jmethodID midGetObjectAt = env->GetMethodID(clsASN1Sequence, "getObjectAt", "(I)Lorg/bouncycastle/asn1/ASN1Encodable;");
    jint size = env->CallIntMethod(attestation_n, midSize);
    LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: sequence size=%d", size);
    jclass clsASN1TaggedObject = env->FindClass("org/bouncycastle/asn1/ASN1TaggedObject");
    jmethodID midGetTagNo = env->GetMethodID(clsASN1TaggedObject, "getTagNo", "()I");
    const int KEYMASTER_TAG_TYPE_MASK = 0x0FFFFFFF;
    const int KM_BYTES = 9 << 28;
    const int KM_TAG_ROOT_OF_TRUST = KM_BYTES | 704;
    for (int i = 0; i < size; ++i) {
        jobject element = env->CallObjectMethod(attestation_n, midGetObjectAt, i);
        LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: i=%d, element=%p", i, element);
        if (!env->IsInstanceOf(element, clsASN1TaggedObject)) {
            LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: element %d is not ASN1TaggedObject!", i);
            return nullptr;
        }
        jint tag = env->CallIntMethod(element, midGetTagNo);
        LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: i=%d, tag=%d", i, tag);
        if ((tag & KEYMASTER_TAG_TYPE_MASK) == (KM_TAG_ROOT_OF_TRUST & KEYMASTER_TAG_TYPE_MASK)) {
            jclass elementClass = env->GetObjectClass(element);
            jmethodID midGetBaseObject = env->GetMethodID(elementClass, "getBaseObject", "()Lorg/bouncycastle/asn1/ASN1Object;");
            if (!midGetBaseObject) {
                LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: neither getBaseObject nor getObject found!");
                return nullptr;
            }
            jobject baseObj = env->CallObjectMethod(element, midGetBaseObject);
            LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: i=%d, baseObj=%p", i, baseObj);
            jclass baseObjClass = env->GetObjectClass(baseObj);
            jmethodID midToASN1Primitive = env->GetMethodID(baseObjClass, "toASN1Primitive", "()Lorg/bouncycastle/asn1/ASN1Primitive;");
            jobject value = env->CallObjectMethod(baseObj, midToASN1Primitive);
            LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: found RootOfTrust ASN1Primitive at %d, value=%p", i, value);
            return value;
        }
    }
    LOGE("[JNI] get_root_of_trust_primitive_from_authorization_list: not found!");
    return nullptr;
}

struct RootOfTrustFields {
    std::string verifiedBootKeyHex;
    bool deviceLocked;
    int verifiedBootState;
    bool valid;
};

RootOfTrustFields parse_root_of_trust_from_asn1_sequence(JNIEnv* env, jobject rootOfTrustSeq) {
    RootOfTrustFields result;
    result.valid = false;
    if (!rootOfTrustSeq) {
        LOGE("[JNI] parse_root_of_trust_from_asn1_sequence: input is null!");
        return result;
    }
    jclass clsASN1Sequence = env->FindClass("org/bouncycastle/asn1/ASN1Sequence");
    jmethodID midGetObjectAt = env->GetMethodID(clsASN1Sequence, "getObjectAt", "(I)Lorg/bouncycastle/asn1/ASN1Encodable;");

    // 1. verifiedBootKey
    jobject verifiedBootKeyObj = env->CallObjectMethod(rootOfTrustSeq, midGetObjectAt, 0);
    jbyteArray vbkArr = Asn1Utils_getByteArrayFromAsn1(env, verifiedBootKeyObj);
    std::vector<uint8_t> vbk;
    if (vbkArr && env->GetArrayLength(vbkArr) > 0) {
        jsize len = env->GetArrayLength(vbkArr);
        vbk.resize(len);
        env->GetByteArrayRegion(vbkArr, 0, len, reinterpret_cast<jbyte*>(vbk.data()));
    }
    char buf[3];
    for (size_t i = 0; i < vbk.size(); ++i) {
        snprintf(buf, sizeof(buf), "%02x", vbk[i]);
        result.verifiedBootKeyHex += buf;
    }

    // 2. deviceLocked
    jobject deviceLockedObj = env->CallObjectMethod(rootOfTrustSeq, midGetObjectAt, 1);
    result.deviceLocked = Asn1Utils_getBooleanFromAsn1(env, deviceLockedObj);

    // 3. verifiedBootState
    jobject verifiedBootStateObj = env->CallObjectMethod(rootOfTrustSeq, midGetObjectAt, 2);
    result.verifiedBootState = Asn1Utils_getIntegerFromAsn1(env, verifiedBootStateObj);

    result.valid = true;
    LOGE("[JNI] parse_root_of_trust_from_asn1_sequence: verifiedBootKey=%s, deviceLocked=%d, verifiedBootState=%d",
         result.verifiedBootKeyHex.c_str(), result.deviceLocked, result.verifiedBootState);
    return result;
}

// 通过 JNI 步步反射解析 attestation extension，提取 RootOfTrust 字段
std::map<std::string, std::map<std::string, std::string>> get_tee_info(JNIEnv* env, jobject context) {
    std::map<std::string, std::map<std::string, std::string>> info;
    if (!env) {
        LOGE("JNIEnv is null, 请确保JNIEnv可用");
        return info;
    }

    if (!context) {
        LOGE("context is null, 请确保Context可用");
        return info;
    }

    std::vector<uint8_t> cert = get_attestation_cert_from_java(env, context);
    if (cert.empty()) {
        LOGE("获取证书失败");
        return info;
    }

    std::vector<uint8_t> certs = get_attestation_cert_from_java(env, context);

    jobject attestation = get_attestation_record_asn1_sequence(env, certs);

    jobject attestation_1 = get_asn1_sequence_object_at(env, attestation, 1);
    jobject attestation_6 = get_asn1_sequence_object_at(env, attestation, 6);
    jobject attestation_7 = get_asn1_sequence_object_at(env, attestation, 7);

    // 解析 attestation_1: attestation_1.getObjectAt(1) -> securityLevel
    int securityLevel = get_security_level_from_asn1_sequence(env, attestation_1);
    LOGE("securityLevel: %d", securityLevel);

    jobject attestation_6_value = get_root_of_trust_primitive_from_authorization_list(env, attestation_6);

    jobject attestation_7_value = get_root_of_trust_primitive_from_authorization_list(env, attestation_7);

    jobject sequence = attestation_6_value ? attestation_6_value : attestation_7_value;

    RootOfTrustFields rootOfTrust = parse_root_of_trust_from_asn1_sequence(env, sequence);

    LOGE("RootOfTrust6: %s, deviceLocked: %d, verifiedBootState: %d", rootOfTrust.verifiedBootKeyHex.c_str(), rootOfTrust.deviceLocked, rootOfTrust.verifiedBootState);

    if (!rootOfTrust.deviceLocked && rootOfTrust.verifiedBootState != 0){
        info["deviceLocked"]["risk"] = "error";
        info["deviceLocked"]["explain"] = "设备已解锁 启动验证未通过";
    }else if (!rootOfTrust.deviceLocked){
        info["deviceLocked"]["risk"] = "error";
        info["deviceLocked"]["explain"] = "设备已解锁";
    }else if (rootOfTrust.verifiedBootState != 0){
        info["deviceLocked"]["risk"] = "error";
        info["deviceLocked"]["explain"] = "启动验证未通过";
    }

    return info;
}

// 主入口，增加 context 参数
std::map<std::string, std::map<std::string, std::string>> get_tee_info() {
    return get_tee_info(zJavaVm::getInstance()->getEnv(), zJavaVm::getInstance()->getContext());
}
