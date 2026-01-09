//
// 合并后的头文件 - 包含所有类定义
//

#ifndef MY_TEE_zTeeCert_H
#define MY_TEE_zTeeCert_H

#include "zLog.h"
#include "zStd.h"
#include <fstream>
#include <memory>


// ========== 统一的 ASN.1 标签定义 ==========
#define ASN1_BOOLEAN 0x01
#define ASN1_INTEGER 0x02
#define ASN1_OCTET_STRING 0x04
#define ASN1_NULL 0x05
#define ASN1_OBJECT_IDENTIFIER 0x06
#define ASN1_ENUMERATED 0x0A
#define ASN1_SEQUENCE 0x30
#define ASN1_SET 0x31
#define ASN1_UTCTIME 0x17
#define ASN1_GENERALIZEDTIME 0x18
#define ASN1_PRINTABLE_STRING 0x13
#define ASN1_UTF8_STRING 0x0C
#define ASN1_IA5_STRING 0x16

// ========== Keymaster 标签号定义 ==========
#define KM_TAG_PURPOSE 1
#define KM_TAG_ALGORITHM 2
#define KM_TAG_KEY_SIZE 3
#define KM_TAG_DIGEST 5
#define KM_TAG_EC_CURVE 10
#define KM_TAG_NO_AUTH_REQUIRED 503
#define KM_TAG_ORIGIN 702
#define KM_TAG_CREATION_DATETIME 701
#define KM_TAG_ROOT_OF_TRUST 704
#define KM_TAG_OS_VERSION 705
#define KM_TAG_OS_PATCHLEVEL 706
#define KM_TAG_ATTESTATION_APPLICATION_ID 709
#define KM_TAG_VENDOR_PATCHLEVEL 718
#define KM_TAG_BOOT_PATCHLEVEL 719

// ========== Asn1.h ==========
class ASN1Element {
public:

    void* data_ptr = nullptr;

    uint8_t* tag_ptr = nullptr;
    uint8_t tag = 0;
    uint8_t tag_size = 0;
    uint32_t tag_number = 0;  // 扩展标签号（支持>=31）

    uint8_t* length_ptr = nullptr;
    uint32_t length = 0;  // 实际长度值
    uint8_t length_size = 0;  // 长度字段的字节数

    ASN1Element();
    ASN1Element(const void* data);

    // 获取标签类别 (0=Universal, 1=Application, 2=Context, 3=Private)
    uint8_t get_tag_class() const {
        return (tag >> 6) & 0x03;
    }

    // 判断是否为构造类型
    bool is_constructed() const {
        return (tag & 0x20) != 0;
    }

    // 获取标签号（包括扩展标签号）
    uint32_t get_tag_number() const {
        return tag_number;
    }

    // 获取实际内容长度
    uint32_t get_content_length() const {
        return length;
    }

    // 获取标签+长度字段的总偏移
    uint32_t get_next_offset() const {
        return tag_size + length_size;
    }

    // 获取标签+长度+内容的总偏移
    uint32_t get_next_body_offset() const {
        return get_next_offset() + length;
    }

    // 获取下一个标签的指针（跳过标签+长度）
    void* get_next_ptr() const {
        return (uint8_t*)data_ptr + get_next_offset();
    }

    // 获取下一个元素的指针（跳过标签+长度+内容）
    void* get_next_body_ptr() const {
        return (uint8_t*)data_ptr + get_next_body_offset();
    }

    // 获取内容开始指针
    void* get_content_ptr() const {
        return get_next_ptr();
    }

    // 解析INTEGER值
    uint64_t getIntegerValue() const {
        if (tag != 0x02 || length == 0) {  // ASN1_INTEGER = 0x02
            return 0;
        }
        uint64_t value = 0;
        uint8_t* bytes = (uint8_t*)get_content_ptr();
        for (uint32_t i = 0; i < length; i++) {
            value = (value << 8) | bytes[i];
        }
        return value;
    }

    // 解析ENUMERATED值
    uint8_t getEnumeratedValue() const {
        if (tag != 0x0A || length == 0) {  // ASN1_ENUMERATED = 0x0A
            return 0;
        }
        return *((uint8_t*)get_content_ptr());
    }
};

// ========== AttestationRecord.h ==========
// 安全级别枚举
enum class SecurityLevel {
    Software = 0,
    TEE = 1,
    StrongBox = 2,
    Unknown = 255
};

// Root of Trust结构
struct RootOfTrust {
    vector<uint8_t> verified_boot_key;
    bool device_locked = false;
    uint32_t verified_boot_state = 0;  // 0=Verified, 1=Self-Signed, 2=Unverified, 3=Failed
};

// Package Info结构
struct PackageInfo {
    string package_name;
    uint32_t version_code = 0;
};

// Attestation Application ID结构
struct AttestationApplicationId {
    vector<PackageInfo> package_infos;
    vector<vector<uint8_t>> signature_digests;  // SHA-256摘要列表
};

// 授权列表类（需要在 AttestationRecord 之前定义）
class AuthorizationList {
public:
    AuthorizationList() = default;
    
    // Getter方法
    const vector<string>& getPurposes() const { return purposes_; }
    uint32_t getAlgorithm() const { return algorithm_; }
    uint32_t getKeySize() const { return key_size_; }
    const vector<string>& getDigests() const { return digests_; }
    uint32_t getECCurve() const { return ec_curve_; }
    bool isNoAuthRequired() const { return no_auth_required_; }
    uint32_t getOrigin() const { return origin_; }
    uint64_t getCreationDateTime() const { return creation_datetime_; }
    const RootOfTrust& getRootOfTrust() const { return root_of_trust_; }
    uint32_t getOSVersion() const { return os_version_; }
    uint32_t getOSPatchLevel() const { return os_patchlevel_; }
    uint32_t getVendorPatchLevel() const { return vendor_patchlevel_; }
    uint32_t getBootPatchLevel() const { return boot_patchlevel_; }
    const AttestationApplicationId& getAttestationApplicationId() const { return attestation_application_id_; }
    
    // Setter方法（供解析器使用）
    void setPurposes(const vector<string>& purposes) { purposes_ = purposes; }
    void setAlgorithm(uint32_t alg) { algorithm_ = alg; }
    void setKeySize(uint32_t size) { key_size_ = size; }
    void setDigests(const vector<string>& digests) { digests_ = digests; }
    void setECCurve(uint32_t curve) { ec_curve_ = curve; }
    void setNoAuthRequired(bool value) { no_auth_required_ = value; }
    void setOrigin(uint32_t orig) { origin_ = orig; }
    void setCreationDateTime(uint64_t datetime) { creation_datetime_ = datetime; }
    void setRootOfTrust(const RootOfTrust& rot) { root_of_trust_ = rot; }
    void setOSVersion(uint32_t version) { os_version_ = version; }
    void setOSPatchLevel(uint32_t patchlevel) { os_patchlevel_ = patchlevel; }
    void setVendorPatchLevel(uint32_t patchlevel) { vendor_patchlevel_ = patchlevel; }
    void setBootPatchLevel(uint32_t patchlevel) { boot_patchlevel_ = patchlevel; }
    void setAttestationApplicationId(const AttestationApplicationId& app_id) { attestation_application_id_ = app_id; }

private:
    vector<string> purposes_;  // KM_TAG_PURPOSE
    uint32_t algorithm_ = 0;              // KM_TAG_ALGORITHM
    uint32_t key_size_ = 0;               // KM_TAG_KEY_SIZE
    vector<string> digests_;    // KM_TAG_DIGEST
    uint32_t ec_curve_ = 0;              // KM_TAG_EC_CURVE
    bool no_auth_required_ = false;       // KM_TAG_NO_AUTH_REQUIRED
    uint32_t origin_ = 0;                 // KM_TAG_ORIGIN
    uint64_t creation_datetime_ = 0;      // KM_TAG_CREATION_DATETIME (毫秒时间戳)
    
    RootOfTrust root_of_trust_;
    uint32_t os_version_ = 0;             // KM_TAG_OS_VERSION
    uint32_t os_patchlevel_ = 0;          // KM_TAG_OS_PATCHLEVEL (YYYYMM)
    uint32_t vendor_patchlevel_ = 0;      // KM_TAG_VENDOR_PATCHLEVEL (YYYYMMDD)
    uint32_t boot_patchlevel_ = 0;        // KM_TAG_BOOT_PATCHLEVEL (YYYYMMDD)
    AttestationApplicationId attestation_application_id_;
};

// 认证记录类
class AttestationRecord {
public:
    AttestationRecord() = default;
    
    // Getter方法
    uint32_t getAttestationVersion() const { return attestation_version_; }
    SecurityLevel getAttestationSecurityLevel() const { return attestation_security_level_; }
    uint32_t getKeymasterVersion() const { return keymaster_version_; }
    SecurityLevel getKeymasterSecurityLevel() const { return keymaster_security_level_; }
    const vector<uint8_t>& getAttestationChallenge() const { return attestation_challenge_; }
    const vector<uint8_t>& getUniqueId() const { return unique_id_; }
    
    // 获取Software Enforced授权列表
    const AuthorizationList* getSoftwareEnforced() const {
        return software_enforced_;
    }
    
    // 获取TEE Enforced授权列表
    const AuthorizationList* getTEEEnforced() const {
        return tee_enforced_;
    }
    
    // 析构函数：释放授权列表对象
    ~AttestationRecord() {
        delete software_enforced_;
        delete tee_enforced_;
    }
    
    // Setter方法（供解析器使用）
    void setAttestationVersion(uint32_t version) { attestation_version_ = version; }
    void setAttestationSecurityLevel(SecurityLevel level) { attestation_security_level_ = level; }
    void setKeymasterVersion(uint32_t version) { keymaster_version_ = version; }
    void setKeymasterSecurityLevel(SecurityLevel level) { keymaster_security_level_ = level; }
    void setAttestationChallenge(const vector<uint8_t>& challenge) { attestation_challenge_ = challenge; }
    void setUniqueId(const vector<uint8_t>& id) { unique_id_ = id; }
    void setSoftwareEnforced(AuthorizationList* list) { software_enforced_ = list; }
    void setTEEEnforced(AuthorizationList* list) { tee_enforced_ = list; }

private:
    uint32_t attestation_version_ = 0;
    SecurityLevel attestation_security_level_ = SecurityLevel::Unknown;
    uint32_t keymaster_version_ = 0;
    SecurityLevel keymaster_security_level_ = SecurityLevel::Unknown;
    vector<uint8_t> attestation_challenge_;
    vector<uint8_t> unique_id_;
    
    AuthorizationList* software_enforced_;  // Software Enforced授权列表
    AuthorizationList* tee_enforced_;        // TEE Enforced授权列表
};

// ========== AuthorizationListParser.h ==========
class AuthorizationListParser {
public:
    AuthorizationListParser(void* list_ptr, uint32_t list_length, const char* list_name);
    
    // 解析授权列表（在构造时自动调用）
    void parse();
    
    // 获取解析后的授权列表数据
    const AuthorizationList& getAuthorizationList() const { return auth_list_; }

private:
    void* list_ptr_;
    uint32_t list_length_;
    const char* list_name_;
    AuthorizationList auth_list_;  // 存储解析后的授权列表数据
    
    // 解析单个Keymaster标签
    void parseKeymasterTag(const ASN1Element& auth_tag, uint32_t tag_num);
    
    // 解析各种标签的具体实现
    void parsePurpose(const ASN1Element& auth_tag);
    void parseAlgorithm(const ASN1Element& auth_tag);
    void parseKeySize(const ASN1Element& auth_tag);
    void parseDigest(const ASN1Element& auth_tag);
    void parseECCurve(const ASN1Element& auth_tag);
    void parseNoAuthRequired(const ASN1Element& auth_tag);
    void parseOrigin(const ASN1Element& auth_tag);
    void parseCreationDateTime(const ASN1Element& auth_tag);
    void parseRootOfTrust(const ASN1Element& auth_tag);
    void parseOSVersion(const ASN1Element& auth_tag);
    void parseOSPatchLevel(const ASN1Element& auth_tag);
    void parseVendorPatchLevel(const ASN1Element& auth_tag);
    void parseBootPatchLevel(const ASN1Element& auth_tag);
    void parseAttestationApplicationId(const ASN1Element& auth_tag);
};

// ========== TEEAttestationExtension.h ==========
// TEE认证扩展的OID (对象标识符)
// 1.3.6.1.4.1.11129.2.1.17
extern const uint8_t TEE_ATTESTATION_OID[];
extern const size_t TEE_ATTESTATION_OID_LENGTH;

// TEE Attestation Extension类（需要在 CertificateExtensions 之前定义）
class TEEAttestationExtension {
public:
    // 构造函数：从扩展序列解析TEE认证扩展
    explicit TEEAttestationExtension(ASN1Element& extension_sequence);
    
    // 获取解析结果（0表示成功，非0表示失败）
    int getParseResult() const { return parse_result_; }
    
    // 获取OID
    const string& getOID() const { return oid_; }
    
    // 获取Critical标志
    bool isCritical() const { return critical_; }
    
    // 获取认证记录
    const AttestationRecord* getAttestationRecord() const {
        return attestation_record_;
    }
    
    // 析构函数：释放认证记录对象
    ~TEEAttestationExtension();

private:
    int parse_result_ = -1;  // 解析结果：0=成功，非0=失败
    string oid_;  // 扩展OID
    bool critical_ = false;  // Critical标志
    AttestationRecord* attestation_record_;  // 认证记录
    
    // 辅助函数
    string parseOID(const ASN1Element& oid_elem);
    int parseExtensionContent(ASN1Element& extension_value);
    int parseAttestationRecord(ASN1Element& attestation_sequence);
};

// ========== CertificateExtensions.h ==========
class CertificateExtensions {
public:
    // 构造函数：从扩展序列解析（自动列出所有扩展并解析TEE扩展）
    explicit CertificateExtensions(ASN1Element& extensions_sequence);
    
    // 获取解析结果（0表示成功，非0表示失败）
    int getParseResult() const { return parse_result_; }
    
    // 获取TEE认证扩展（如果解析成功）
    const TEEAttestationExtension* getTEEAttestationExtension() const {
        return tee_extension_;
    }
    
    // 析构函数：释放TEE扩展对象
    ~CertificateExtensions();
    
    // 获取TEE认证记录（如果解析成功）
    const AttestationRecord* getAttestationRecord() const {
        const TEEAttestationExtension* ext = getTEEAttestationExtension();
        return ext ? ext->getAttestationRecord() : nullptr;
    }
    
    // 获取TEE Enforced授权列表（通过AttestationRecord）
    const AuthorizationList* getTEEEnforced() const {
        const AttestationRecord* record = getAttestationRecord();
        return record ? record->getTEEEnforced() : nullptr;
    }
    
    // 获取Software Enforced授权列表（通过AttestationRecord）
    const AuthorizationList* getSoftwareEnforced() const {
        const AttestationRecord* record = getAttestationRecord();
        return record ? record->getSoftwareEnforced() : nullptr;
    }

private:
    ASN1Element& extensions_sequence_;
    int parse_result_ = -1;  // 解析结果：0=成功，非0=失败
    TEEAttestationExtension* tee_extension_;  // TEE认证扩展
    
    // 辅助函数：解析OID并返回可读字符串
    string parseOID(const ASN1Element& oid_elem);
    
    // 列出所有扩展的OID
    void listAllExtensions();
    
    // 查找并解析TEE认证扩展
    int parseTEEAttestationExtension();
};

// ========== TBSCertificate.h ==========
// TBSCertificate类 - 待签名证书部分
class TBSCertificate {
public:
    // 构造函数：从ASN1Element解析TBSCertificate
    explicit TBSCertificate(ASN1Element& tbs_element);
    
    // 获取解析结果（0表示成功，非0表示失败）
    int getParseResult() const { return parse_result_; }
    
    // Getter方法
    uint32_t getVersion() const { return version_; }
    const vector<uint8_t>& getSerialNumber() const { return serial_number_; }
    const string& getSignatureAlgorithm() const { return signature_algorithm_; }
    const string& getIssuer() const { return issuer_; }
    const string& getNotBefore() const { return not_before_; }
    const string& getNotAfter() const { return not_after_; }
    const string& getSubject() const { return subject_; }
    const string& getSubjectPublicKeyAlgorithm() const { return subject_public_key_algorithm_; }
    uint32_t getSubjectPublicKeyLength() const { return subject_public_key_length_; }
    
    // 获取扩展对象
    const CertificateExtensions* getExtensions() const {
        return extensions_;
    }
    
    // 析构函数：释放扩展对象
    ~TBSCertificate();

private:
    int parse_result_ = -1;  // 解析结果：0=成功，非0=失败
    
    // TBSCertificate字段
    uint32_t version_ = 2;  // 默认v3 (0=v1, 1=v2, 2=v3)
    vector<uint8_t> serial_number_;
    string signature_algorithm_;
    string issuer_;
    string not_before_;
    string not_after_;
    string subject_;
    string subject_public_key_algorithm_;
    uint32_t subject_public_key_length_ = 0;
    
    CertificateExtensions* extensions_;  // 扩展对象
    
    // 辅助函数
    string parseOID(const ASN1Element& oid_elem);
    string parseTime(const ASN1Element& time_elem);
    string parseDN(const ASN1Element& dn_elem);
    
    // 解析各个字段
    int parseVersion(ASN1Element& first_field);
    int parseSerialNumber(ASN1Element& serial_elem);
    int parseSignatureAlgorithm(ASN1Element& sig_alg_elem);
    int parseIssuer(ASN1Element& issuer_elem);
    int parseValidity(ASN1Element& validity_elem);
    int parseSubject(ASN1Element& subject_elem);
    int parseSubjectPublicKeyInfo(ASN1Element& pub_key_elem);
    int parseExtensions(ASN1Element& extensions_field);
};

// ========== X509Certificate.h ==========
// X.509 Certificate类 - 最外层证书类
class X509Certificate {
public:
    // 构造函数：从内存数据加载并解析证书
    explicit X509Certificate(vector<uint8_t> data);
    
    // 构造函数：从文件路径加载并解析证书
    explicit X509Certificate(const string& filepath);
    
    // 构造函数：从内存数据加载并解析证书（兼容旧接口）
    X509Certificate(const uint8_t* data, size_t length);
    
    // 检查证书是否有效
    bool isValid() const { return !cert_data_.empty() && parse_result_ == 0; }
    
    // 获取解析结果（0表示成功，非0表示失败）
    int getParseResult() const { return parse_result_; }
    
    // 获取证书长度
    size_t getCertLength() const { return cert_data_.size(); }
    
    // 获取TBSCertificate对象
    const TBSCertificate* getTBSCertificate() const {
        return tbs_certificate_;
    }
    
    // 析构函数：释放TBSCertificate对象
    ~X509Certificate();
    
    // 获取签名算法
    const string& getSignatureAlgorithm() const { return signature_algorithm_; }
    
    // 获取签名值（原始字节）
    const vector<uint8_t>& getSignatureValue() const { return signature_value_; }
    
    // 读取二进制证书文件并转换为 vector<uint8_t>
    static vector<uint8_t> readCertFile(const string& filename);

private:
    vector<uint8_t> cert_data_;
    int parse_result_ = -1;  // 解析结果：0=成功，非0=失败
    TBSCertificate* tbs_certificate_;  // TBSCertificate对象
    string signature_algorithm_;  // 签名算法
    vector<uint8_t> signature_value_;  // 签名值
    
    // 解析X.509证书
    int parseCertificate(const vector<uint8_t>& cert);
    
    // 辅助函数：解析OID并返回可读字符串
    string parseOID(const ASN1Element& oid_elem);
};

// ========== zTeeCert.h ==========
// zTeeCert类 - 封装X509Certificate，提供便捷的访问接口
class zTeeCert {
public:
    // 构造函数：从文件路径加载并解析证书
    explicit zTeeCert(const string& filepath);
    
    // 构造函数：从内存数据加载并解析证书
    zTeeCert(vector<uint8_t> data);
    
    // 检查证书是否有效
    bool isValid() const { return cert_ ? cert_->isValid() : false; }
    
    // 获取解析结果（0表示成功，非0表示失败）
    int getParseResult() const { return cert_ ? cert_->getParseResult() : -1; }
    
    // 获取证书长度
    size_t getCertLength() const { return cert_ ? cert_->getCertLength() : 0; }
    
    // 获取X509Certificate对象（必须通过此方法链式访问下层对象）
    const X509Certificate* getX509Certificate() const { return cert_; }
    
    // 析构函数：释放证书对象
    ~zTeeCert();

private:
    X509Certificate* cert_;  // X.509证书对象
};




#endif //MY_TEE_zTeeCert_H
