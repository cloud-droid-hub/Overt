//
// 合并后的实现文件 - 包含所有类实现
//

#include <cstring>
#include "zTeeCert.h"
#include "zLog.h"
#include "zStd.h"


// ========== TEE认证扩展的OID定义 ==========
// 1.3.6.1.4.1.11129.2.1.17
const uint8_t TEE_ATTESTATION_OID[] = {0x2B, 0x06, 0x01, 0x04, 0x01, 0xD6, 0x79, 0x02, 0x01, 0x11};
const size_t TEE_ATTESTATION_OID_LENGTH = sizeof(TEE_ATTESTATION_OID);

// ========== Asn1.cpp ==========
ASN1Element::ASN1Element(const void* data){
    data_ptr = const_cast<void*>(data);

    // 解析标签
    tag_ptr = (uint8_t*)data_ptr;
    tag = *tag_ptr;
    tag_size = 1;
    
    // 检查是否为扩展标签号（标签号 >= 31）
    bool is_extended = ((tag & 0x1F) == 0x1F);
    if (is_extended) {
        // 扩展标签号：多字节BER编码
        tag_number = 0;
        uint8_t* current = tag_ptr + 1;
        uint8_t byte;
        do {
            byte = *current;
            tag_number = (tag_number << 7) | (byte & 0x7F);
            tag_size++;
            current++;
        } while ((byte & 0x80) != 0);
    } else {
        // 单字节标签号
        tag_number = tag & 0x1F;
    }

    // 解析长度
    length_ptr = tag_ptr + tag_size;
    uint8_t first_length_byte = *length_ptr;
    length_size = 1;

    if ((first_length_byte & 0x80) == 0) {
        // 短格式：长度直接编码在第一个字节中
        length = first_length_byte;
    } else {
        // 长格式：第一个字节的低7位表示后续长度字节数
        uint8_t num_octets = first_length_byte & 0x7F;
        if (num_octets == 0) {
            // 不定长格式（0x80），DER编码不支持，但需要处理
            length = 0;
            length_size = 1;
        } else if (num_octets > 0 && num_octets <= 4) {
            length = 0;
            for (uint8_t i = 0; i < num_octets; i++) {
                length = (length << 8) | *(length_ptr + 1 + i);
            }
            length_size += num_octets;
        } else {
            // 无效的长度编码（num_octets > 4）
            length = 0;
            length_size = 1;
        }
    }
}

ASN1Element::ASN1Element() {
    data_ptr = nullptr;
    tag_ptr = nullptr;
    tag = 0;
    tag_size = 0;
    tag_number = 0;
    length_ptr = nullptr;
    length = 0;
    length_size = 0;
}


// ========== AuthorizationListParser.cpp ==========
AuthorizationListParser::AuthorizationListParser(void* list_ptr, uint32_t list_length, const char* list_name)
    : list_ptr_(list_ptr), list_length_(list_length), list_name_(list_name) {
}

void AuthorizationListParser::parse() {
    LOGE("[Native-TEE] ========== 开始解析%s授权列表 ==========", list_name_);
    LOGE("[Native-TEE] 授权列表起始位置: %p, 长度: %u bytes", list_ptr_, list_length_);
    
    uint32_t offset = 0;
    while (offset < list_length_) {
        ASN1Element auth_tag((uint8_t*)list_ptr_ + offset);
        
        if (auth_tag.get_tag_class() == 2) {  // Context类别
            uint32_t tag_num = auth_tag.get_tag_number();
            parseKeymasterTag(auth_tag, tag_num);
        } else {
            LOGE("[Native-TEE] 非Context标签: class=%u, tag=0x%02X", 
                 auth_tag.get_tag_class(), auth_tag.tag);
        }
        
        offset += auth_tag.get_next_body_offset();
        if (auth_tag.get_next_body_offset() == 0) {
            LOGE("[Native-TEE] 警告: 标签偏移为0，停止解析");
            break;
        }
    }
    
    LOGE("[Native-TEE] ========== %s授权列表解析完成 ==========", list_name_);
}

void AuthorizationListParser::parseKeymasterTag(const ASN1Element& auth_tag, uint32_t tag_num) {
    switch (tag_num) {
        case KM_TAG_PURPOSE:
            parsePurpose(auth_tag);
            break;
        case KM_TAG_ALGORITHM:
            parseAlgorithm(auth_tag);
            break;
        case KM_TAG_KEY_SIZE:
            parseKeySize(auth_tag);
            break;
        case KM_TAG_DIGEST:
            parseDigest(auth_tag);
            break;
        case KM_TAG_EC_CURVE:
            parseECCurve(auth_tag);
            break;
        case KM_TAG_NO_AUTH_REQUIRED:
            parseNoAuthRequired(auth_tag);
            break;
        case KM_TAG_ORIGIN:
            parseOrigin(auth_tag);
            break;
        case KM_TAG_CREATION_DATETIME:
            parseCreationDateTime(auth_tag);
            break;
        case KM_TAG_ROOT_OF_TRUST:
            parseRootOfTrust(auth_tag);
            break;
        case KM_TAG_OS_VERSION:
            parseOSVersion(auth_tag);
            break;
        case KM_TAG_OS_PATCHLEVEL:
            parseOSPatchLevel(auth_tag);
            break;
        case KM_TAG_VENDOR_PATCHLEVEL:
            parseVendorPatchLevel(auth_tag);
            break;
        case KM_TAG_BOOT_PATCHLEVEL:
            parseBootPatchLevel(auth_tag);
            break;
        case KM_TAG_ATTESTATION_APPLICATION_ID:
            parseAttestationApplicationId(auth_tag);
            break;
        default:
            LOGE("[Native-TEE] 未知标签: Context[%u]", tag_num);
            break;
    }
}

void AuthorizationListParser::parsePurpose(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析密钥用途 (Context[1])");
    vector<string> purposes;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element purpose_set((uint8_t*)auth_tag.get_content_ptr());
        if (purpose_set.tag == ASN1_SET) {
            void* set_ptr = purpose_set.get_content_ptr();
            uint32_t set_remaining = purpose_set.get_content_length();
            uint32_t set_offset = 0;
            while (set_offset < set_remaining) {
                ASN1Element purpose_elem((uint8_t*)set_ptr + set_offset);
                uint8_t purpose_val = 0;
                
                // PURPOSE可以是ENUMERATED或INTEGER（某些实现使用INTEGER编码枚举值）
                if (purpose_elem.tag == ASN1_ENUMERATED) {
                    purpose_val = purpose_elem.getEnumeratedValue();
                } else if (purpose_elem.tag == ASN1_INTEGER && purpose_elem.get_content_length() > 0) {
                    // 某些实现使用INTEGER编码枚举值（1字节）
                    purpose_val = (uint8_t)purpose_elem.getIntegerValue();
                } else {
                    set_offset += purpose_elem.get_next_body_offset();
                    if (purpose_elem.get_next_body_offset() == 0) break;
                    continue;
                }
                
                const char* purpose_names[] = {"ENCRYPT", "DECRYPT", "SIGN", "VERIFY", "DERIVE_KEY", "WRAP_KEY"};
                if (purpose_val < 6) {
                    purposes.push_back(purpose_names[purpose_val]);
                }
                
                set_offset += purpose_elem.get_next_body_offset();
                if (purpose_elem.get_next_body_offset() == 0) break;
            }
            if (purposes.empty()) {
                LOGE("[Native-TEE] 密钥用途: 未知");
            } else {
                string purposes_str;
                for (size_t i = 0; i < purposes.size(); i++) {
                    if (i > 0) purposes_str += ", ";
                    purposes_str += purposes[i];
                }
                LOGE("[Native-TEE] 密钥用途: %s", purposes_str.c_str());
            }
        }
    }
    auth_list_.setPurposes(purposes);
}

void AuthorizationListParser::parseAlgorithm(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析算法 (Context[2])");
    uint32_t algorithm = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element alg_elem((uint8_t*)auth_tag.get_content_ptr());
        uint32_t alg_val = 0;
        
        // ALGORITHM可以是ENUMERATED或INTEGER（某些实现使用INTEGER编码枚举值）
        if (alg_elem.tag == ASN1_ENUMERATED) {
            alg_val = alg_elem.getEnumeratedValue();
        } else if (alg_elem.tag == ASN1_INTEGER && alg_elem.get_content_length() > 0) {
            alg_val = (uint32_t)alg_elem.getIntegerValue();
        } else {
            LOGE("[Native-TEE] 警告: ALGORITHM元素格式错误，tag=0x%02X", alg_elem.tag);
            return;
        }
        
        algorithm = alg_val;
        
        // Keymaster算法枚举值定义（根据Keymaster HAL）
        // 注意：不同版本的Keymaster可能使用不同的值
        const char* alg_name = "Unknown";
        switch (alg_val) {
            case 1: alg_name = "RSA"; break;
            case 3: alg_name = "EC"; break;      // 椭圆曲线
            case 32: alg_name = "AES"; break;
            case 33: alg_name = "3DES"; break;
            case 128: alg_name = "HMAC"; break;
            case 5: alg_name = "GCM"; break;
            case 6: alg_name = "CHACHA20"; break;
            // 兼容旧版本定义（0-based索引）
            case 0: alg_name = "RSA"; break;
            case 2: alg_name = "AES"; break;
            case 4: alg_name = "HMAC"; break;
            default: {
                // 尝试作为0-based索引
                const char* alg_names_old[] = {"RSA", "EC", "AES", "3DES", "HMAC", "GCM", "CHACHA20"};
                if (alg_val < 7) {
                    alg_name = alg_names_old[alg_val];
                }
                break;
            }
        }
        LOGE("[Native-TEE] 算法: %u (%s)", alg_val, alg_name);
    }
    auth_list_.setAlgorithm(algorithm);
}

void AuthorizationListParser::parseKeySize(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析密钥大小 (Context[3])");
    uint32_t key_size = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element key_size_int((uint8_t*)auth_tag.get_content_ptr());
        if (key_size_int.tag == ASN1_INTEGER) {
            key_size = (uint32_t)key_size_int.getIntegerValue();
            LOGE("[Native-TEE] 密钥大小: %u bits", key_size);
        }
    }
    auth_list_.setKeySize(key_size);
}

void AuthorizationListParser::parseDigest(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析摘要算法 (Context[5])");
    vector<string> digests;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element digest_set((uint8_t*)auth_tag.get_content_ptr());
        if (digest_set.tag == ASN1_SET) {
            void* set_ptr = digest_set.get_content_ptr();
            uint32_t set_remaining = digest_set.get_content_length();
            uint32_t set_offset = 0;
            while (set_offset < set_remaining) {
                ASN1Element digest_elem((uint8_t*)set_ptr + set_offset);
                uint8_t digest_val = 0;
                
                // DIGEST可以是ENUMERATED或INTEGER（某些实现使用INTEGER编码枚举值）
                if (digest_elem.tag == ASN1_ENUMERATED) {
                    digest_val = digest_elem.getEnumeratedValue();
                } else if (digest_elem.tag == ASN1_INTEGER && digest_elem.get_content_length() > 0) {
                    digest_val = (uint8_t)digest_elem.getIntegerValue();
                } else {
                    set_offset += digest_elem.get_next_body_offset();
                    if (digest_elem.get_next_body_offset() == 0) break;
                    continue;
                }
                
                const char* digest_names[] = {"MD5", "SHA1", "SHA_2_224", "SHA_2_256", "SHA_2_384", "SHA_2_512"};
                if (digest_val < 6) {
                    digests.push_back(digest_names[digest_val]);
                }
                
                set_offset += digest_elem.get_next_body_offset();
                if (digest_elem.get_next_body_offset() == 0) break;
            }
            if (digests.empty()) {
                LOGE("[Native-TEE] 摘要算法: 未知");
            } else {
                string digests_str;
                for (size_t i = 0; i < digests.size(); i++) {
                    if (i > 0) digests_str += ", ";
                    digests_str += digests[i];
                }
                LOGE("[Native-TEE] 摘要算法: %s", digests_str.c_str());
            }
        }
    }
    auth_list_.setDigests(digests);
}

void AuthorizationListParser::parseECCurve(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析椭圆曲线 (Context[10])");
    uint32_t ec_curve = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element curve_elem((uint8_t*)auth_tag.get_content_ptr());
        uint8_t curve_val = 0;
        
        // EC_CURVE可以是ENUMERATED或INTEGER（某些实现使用INTEGER编码枚举值）
        if (curve_elem.tag == ASN1_ENUMERATED) {
            curve_val = curve_elem.getEnumeratedValue();
        } else if (curve_elem.tag == ASN1_INTEGER && curve_elem.get_content_length() > 0) {
            curve_val = (uint8_t)curve_elem.getIntegerValue();
        } else {
            LOGE("[Native-TEE] 警告: EC_CURVE元素格式错误，tag=0x%02X", curve_elem.tag);
            auth_list_.setECCurve(0);
            return;
        }
        
        ec_curve = curve_val;
        const char* curve_names[] = {"P_224", "P_256", "P_384", "P_521"};
        LOGE("[Native-TEE] 椭圆曲线: %d (%s)", curve_val, 
             (curve_val < 4) ? curve_names[curve_val] : "Unknown");
    }
    auth_list_.setECCurve(ec_curve);
}

void AuthorizationListParser::parseNoAuthRequired(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析无需认证标志 (Context[503])");
    bool no_auth_required = false;
    if (auth_tag.is_constructed()) {
        // NO_AUTH_REQUIRED可以是NULL（0x05，表示TRUE）或BOOLEAN
        if (auth_tag.get_content_length() == 0) {
            // 空内容，表示TRUE
            no_auth_required = true;
            LOGE("[Native-TEE] 无需认证: 是 (NULL)");
        } else {
            ASN1Element no_auth_elem((uint8_t*)auth_tag.get_content_ptr());
            if (no_auth_elem.tag == 0x05) {  // ASN1_NULL
                no_auth_required = true;
                LOGE("[Native-TEE] 无需认证: 是 (NULL)");
            } else if (no_auth_elem.tag == ASN1_BOOLEAN) {
                uint8_t val = *((uint8_t*)no_auth_elem.get_content_ptr());
                no_auth_required = (val == 0xFF);
                LOGE("[Native-TEE] 无需认证: %s", (val == 0xFF) ? "是" : "否");
            } else {
                no_auth_required = true;
                LOGE("[Native-TEE] 无需认证: 是 (存在标签，tag=0x%02X)", no_auth_elem.tag);
            }
        }
    }
    auth_list_.setNoAuthRequired(no_auth_required);
}

void AuthorizationListParser::parseOrigin(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析密钥来源 (Context[702])");
    uint32_t origin = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element origin_elem((uint8_t*)auth_tag.get_content_ptr());
        uint8_t origin_val = 0;
        
        // ORIGIN可以是ENUMERATED或INTEGER（某些实现使用INTEGER编码枚举值）
        if (origin_elem.tag == ASN1_ENUMERATED) {
            origin_val = origin_elem.getEnumeratedValue();
        } else if (origin_elem.tag == ASN1_INTEGER && origin_elem.get_content_length() > 0) {
            origin_val = (uint8_t)origin_elem.getIntegerValue();
        } else {
            LOGE("[Native-TEE] 警告: ORIGIN元素格式错误，tag=0x%02X", origin_elem.tag);
            auth_list_.setOrigin(0);
            return;
        }
        
        origin = origin_val;
        const char* origin_names[] = {"GENERATED", "DERIVED", "IMPORTED", "UNKNOWN", "SECURELY_IMPORTED"};
        LOGE("[Native-TEE] 密钥来源: %d (%s)", origin_val, 
             (origin_val < 5) ? origin_names[origin_val] : "Unknown");
    }
    auth_list_.setOrigin(origin);
}

void AuthorizationListParser::parseCreationDateTime(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析创建时间 (Context[701])");
    uint64_t creation_datetime = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element creation_int((uint8_t*)auth_tag.get_content_ptr());
        if (creation_int.tag == ASN1_INTEGER && creation_int.get_content_length() > 0) {
            creation_datetime = creation_int.getIntegerValue();
            uint64_t timestamp_ms = creation_datetime;
            uint64_t timestamp_s = timestamp_ms / 1000;
            uint64_t ms_remainder = timestamp_ms % 1000;
            time_t time_val = (time_t)timestamp_s;
            struct tm* timeinfo = gmtime(&time_val);
            if (timeinfo) {
                LOGE("[Native-TEE] 创建时间: %04d-%02d-%02d %02d:%02d:%02d.%03llu UTC (时间戳: %llu ms)", 
                     timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                     timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                     (unsigned long long)ms_remainder, (unsigned long long)timestamp_ms);
            } else {
                LOGE("[Native-TEE] 创建时间戳: %llu ms (自1970-01-01 00:00:00 UTC)", 
                     (unsigned long long)timestamp_ms);
            }
        }
    }
    auth_list_.setCreationDateTime(creation_datetime);
}

void AuthorizationListParser::parseRootOfTrust(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析信任根 (Context[704])");
    RootOfTrust root_of_trust;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element rot_sequence((uint8_t*)auth_tag.get_content_ptr());
        if (rot_sequence.tag == ASN1_SEQUENCE) {
            ASN1Element vbk((uint8_t*)rot_sequence.get_content_ptr());
            if (vbk.tag == ASN1_OCTET_STRING) {
                uint32_t vbk_len = vbk.get_content_length();
                LOGE("[Native-TEE] Verified Boot Key: %u bytes", vbk_len);
                if (vbk_len > 0 && vbk_len <= 64) {
                    uint8_t* vbk_data = (uint8_t*)vbk.get_content_ptr();
                    root_of_trust.verified_boot_key.assign(vbk_data, vbk_data + vbk_len);
                    string vbk_hex;
                    for (uint32_t i = 0; i < vbk_len && i < 16; i++) {
                        char hex[4];
                        snprintf(hex, sizeof(hex), "%02X", vbk_data[i]);
                        vbk_hex += hex;
                        if (i < vbk_len - 1 && i < 15) vbk_hex += " ";
                    }
                    if (vbk_len > 16) vbk_hex += "...";
                    LOGE("[Native-TEE] VBK (前16字节): %s", vbk_hex.c_str());
                }
                
                ASN1Element device_locked((uint8_t*)vbk.get_next_body_ptr());
                if (device_locked.tag == ASN1_BOOLEAN) {
                    uint8_t locked = *((uint8_t*)device_locked.get_content_ptr());
                    root_of_trust.device_locked = (locked == 0xFF);
                    LOGE("[Native-TEE] Device Locked: %s", (locked == 0xFF) ? "是" : "否");
                    
                    ASN1Element boot_state((uint8_t*)device_locked.get_next_body_ptr());
                    if (boot_state.tag == ASN1_ENUMERATED) {
                        root_of_trust.verified_boot_state = boot_state.getEnumeratedValue();
                        uint8_t state = root_of_trust.verified_boot_state;
                        const char* state_str[] = {"Verified", "Self-Signed", "Unverified", "Failed"};
                        LOGE("[Native-TEE] Verified Boot State: %d (%s)", 
                             state, (state < 4) ? state_str[state] : "Unknown");
                    }
                }
            }
        }
    }
    auth_list_.setRootOfTrust(root_of_trust);
}

void AuthorizationListParser::parseOSVersion(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析OS版本 (Context[705])");
    uint32_t os_version = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element os_version_int((uint8_t*)auth_tag.get_content_ptr());
        if (os_version_int.tag == ASN1_INTEGER) {
            os_version = (uint32_t)os_version_int.getIntegerValue();
            uint32_t version_value = os_version;
            int major = version_value / 10000;
            int minor = (version_value % 10000) / 100;
            int patch = version_value % 100;
            LOGE("[Native-TEE] OS Version: %u -> Android %d.%d.%d", 
                 version_value, major, minor, patch);
        }
    }
    auth_list_.setOSVersion(os_version);
}

void AuthorizationListParser::parseOSPatchLevel(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析OS补丁级别 (Context[706])");
    uint32_t os_patchlevel = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element patch_int((uint8_t*)auth_tag.get_content_ptr());
        if (patch_int.tag == ASN1_INTEGER) {
            os_patchlevel = (uint32_t)patch_int.getIntegerValue();
            uint32_t patch_value = os_patchlevel;
            int year = patch_value / 100;
            int month = patch_value % 100;
            LOGE("[Native-TEE] OS Patch Level: %u -> %04d-%02d", 
                 patch_value, year, month);
        }
    }
    auth_list_.setOSPatchLevel(os_patchlevel);
}

void AuthorizationListParser::parseVendorPatchLevel(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析厂商补丁级别 (Context[718])");
    uint32_t vendor_patchlevel = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element vendor_patch_int((uint8_t*)auth_tag.get_content_ptr());
        if (vendor_patch_int.tag == ASN1_INTEGER) {
            vendor_patchlevel = (uint32_t)vendor_patch_int.getIntegerValue();
            uint32_t vendor_patch = vendor_patchlevel;
            int year = vendor_patch / 10000;
            int month = (vendor_patch / 100) % 100;
            int day = vendor_patch % 100;
            LOGE("[Native-TEE] Vendor Patch Level: %u -> %04d-%02d-%02d", 
                 vendor_patch, year, month, day);
        }
    }
    auth_list_.setVendorPatchLevel(vendor_patchlevel);
}

void AuthorizationListParser::parseBootPatchLevel(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析启动补丁级别 (Context[719])");
    uint32_t boot_patchlevel = 0;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element boot_patch_int((uint8_t*)auth_tag.get_content_ptr());
        if (boot_patch_int.tag == ASN1_INTEGER) {
            boot_patchlevel = (uint32_t)boot_patch_int.getIntegerValue();
            uint32_t boot_patch = boot_patchlevel;
            int year = boot_patch / 10000;
            int month = (boot_patch / 100) % 100;
            int day = boot_patch % 100;
            LOGE("[Native-TEE] Boot Patch Level: %u -> %04d-%02d-%02d", 
                 boot_patch, year, month, day);
        }
    }
    auth_list_.setBootPatchLevel(boot_patchlevel);
}

void AuthorizationListParser::parseAttestationApplicationId(const ASN1Element& auth_tag) {
    LOGE("[Native-TEE] >>> 解析认证应用ID (Context[709])");
    AttestationApplicationId app_id;
    if (auth_tag.is_constructed() && auth_tag.get_content_length() > 0) {
        ASN1Element inner_octet((uint8_t*)auth_tag.get_content_ptr());
        if (inner_octet.tag == ASN1_OCTET_STRING) {
            LOGE("[Native-TEE] 认证应用ID被OCTET STRING包装，长度: %u", 
                 inner_octet.get_content_length());
            
            ASN1Element app_id_seq((uint8_t*)inner_octet.get_content_ptr());
            if (app_id_seq.tag == ASN1_SEQUENCE) {
                void* seq_ptr = app_id_seq.get_content_ptr();
                uint32_t seq_remaining = app_id_seq.get_content_length();
                uint32_t seq_offset = 0;
                
                while (seq_offset < seq_remaining) {
                    ASN1Element set_elem((uint8_t*)seq_ptr + seq_offset);
                    if (set_elem.tag == ASN1_SET) {
                        void* set_ptr = set_elem.get_content_ptr();
                        uint32_t set_remaining = set_elem.get_content_length();
                        uint32_t set_offset = 0;
                        
                        if (set_offset < set_remaining) {
                            ASN1Element first_elem((uint8_t*)set_ptr + set_offset);
                            if (first_elem.tag == ASN1_SEQUENCE) {
                                LOGE("[Native-TEE] 找到Package Infos SET");
                                while (set_offset < set_remaining) {
                                    ASN1Element pkg_info((uint8_t*)set_ptr + set_offset);
                                    if (pkg_info.tag == ASN1_SEQUENCE) {
                                        ASN1Element pkg_name((uint8_t*)pkg_info.get_content_ptr());
                                        if (pkg_name.tag == ASN1_OCTET_STRING) {
                                            uint32_t name_len = pkg_name.get_content_length();
                                            if (name_len > 0 && name_len < 200) {
                                                string package_name((char*)pkg_name.get_content_ptr(), name_len);
                                                ASN1Element pkg_version((uint8_t*)pkg_name.get_next_body_ptr());
                                                if (pkg_version.tag == ASN1_INTEGER) {
                                                    uint32_t version = (uint32_t)pkg_version.getIntegerValue();
                                                    PackageInfo pkg;
                                                    pkg.package_name = package_name;
                                                    pkg.version_code = version;
                                                    app_id.package_infos.push_back(pkg);
                                                    LOGE("[Native-TEE] 包信息: %s (versionCode: %u)", 
                                                         package_name.c_str(), version);
                                                }
                                            }
                                        }
                                    }
                                    set_offset += pkg_info.get_next_body_offset();
                                }
                            } else if (first_elem.tag == ASN1_OCTET_STRING) {
                                LOGE("[Native-TEE] 找到Signature Digests SET");
                                int sig_count = 0;
                                while (set_offset < set_remaining && sig_count < 10) {
                                    ASN1Element sig_digest((uint8_t*)set_ptr + set_offset);
                                    if (sig_digest.tag == ASN1_OCTET_STRING) {
                                        uint32_t sig_len = sig_digest.get_content_length();
                                        if (sig_len > 0 && sig_len <= 64) {
                                            uint8_t* sig_data = (uint8_t*)sig_digest.get_content_ptr();
                                            vector<uint8_t> digest(sig_data, sig_data + sig_len);
                                            app_id.signature_digests.push_back(digest);
                                            string sig_hex;
                                            for (uint32_t i = 0; i < sig_len && i < 8; i++) {
                                                char hex[4];
                                                snprintf(hex, sizeof(hex), "%02X", sig_data[i]);
                                                sig_hex += hex;
                                            }
                                            if (sig_len > 8) sig_hex += "...";
                                            LOGE("[Native-TEE] 签名摘要[%d]: %u bytes (SHA-256: %s...)", 
                                                 sig_count, sig_len, sig_hex.c_str());
                                            sig_count++;
                                        }
                                    }
                                    set_offset += sig_digest.get_next_body_offset();
                                }
                            }
                        }
                    }
                    seq_offset += set_elem.get_next_body_offset();
                }
            }
        }
    }
    auth_list_.setAttestationApplicationId(app_id);
}



// ========== CertificateExtensions.cpp ==========
CertificateExtensions::CertificateExtensions(ASN1Element& extensions_sequence)
    : extensions_sequence_(extensions_sequence) {
    // 列出所有扩展
    listAllExtensions();
    
    // 解析TEE认证扩展
    parse_result_ = parseTEEAttestationExtension();
}

string CertificateExtensions::parseOID(const ASN1Element& oid_elem) {
    if (oid_elem.tag != ASN1_OBJECT_IDENTIFIER) {
        return "Not an OID";
    }
    
    uint8_t* oid_data = (uint8_t*)oid_elem.get_content_ptr();
    uint32_t oid_len = oid_elem.get_content_length();
    
    if (oid_len == 0) return "Empty OID";
    
    // 解析OID的第一个字节（前两个数字）
    uint32_t first = oid_data[0] / 40;
    uint32_t second = oid_data[0] % 40;
    
    string oid_str = to_string(first) + "." + to_string(second);
    
    // 解析后续字节
    uint32_t value = 0;
    for (uint32_t i = 1; i < oid_len; i++) {
        value = (value << 7) | (oid_data[i] & 0x7F);
        if ((oid_data[i] & 0x80) == 0) {
            oid_str += "." + to_string(value);
            value = 0;
        }
    }
    
    // 常见OID映射
    if (oid_str == "1.2.840.113549.1.1.11") return "sha256WithRSAEncryption (1.2.840.113549.1.1.11)";
    if (oid_str == "1.2.840.113549.1.1.5") return "sha1WithRSAEncryption (1.2.840.113549.1.1.5)";
    if (oid_str == "1.2.840.10045.4.3.2") return "ecdsa-with-SHA256 (1.2.840.10045.4.3.2)";
    if (oid_str == "1.2.840.10045.4.3.3") return "ecdsa-with-SHA384 (1.2.840.10045.4.3.3)";
    if (oid_str == "1.2.840.10045.4.3.4") return "ecdsa-with-SHA512 (1.2.840.10045.4.3.4)";
    if (oid_str == "1.2.840.10045.2.1") return "ecPublicKey (1.2.840.10045.2.1)";
    if (oid_str == "1.2.840.113549.1.1.1") return "rsaEncryption (1.2.840.113549.1.1.1)";
    if (oid_str == "1.3.6.1.4.1.11129.2.1.17") return "Android TEE Key Attestation (1.3.6.1.4.1.11129.2.1.17)";
    
    return oid_str;
}

void CertificateExtensions::listAllExtensions() {
    void* all_extensions_ptr = extensions_sequence_.get_content_ptr();
    uint32_t all_extensions_remaining = extensions_sequence_.get_content_length();
    int ext_count = 0;
    
    while (all_extensions_remaining > 0) {
        ASN1Element ext_seq((uint8_t*)all_extensions_ptr);
        if (ext_seq.tag != ASN1_SEQUENCE) break;
        
        ASN1Element ext_oid((uint8_t*)ext_seq.get_content_ptr());
        if (ext_oid.tag == ASN1_OBJECT_IDENTIFIER) {
            string ext_oid_str = parseOID(ext_oid);
            ext_count++;
            LOGE("[Native-TEE]   扩展[%d]: %s", ext_count, ext_oid_str.c_str());
        }
        
        all_extensions_ptr = ext_seq.get_next_body_ptr();
        uint32_t consumed = ext_seq.get_next_body_offset();
        if (consumed == 0 || consumed > all_extensions_remaining) break;
        all_extensions_remaining -= consumed;
    }
    LOGE("[Native-TEE] 共找到 %d 个扩展", ext_count);
}

int CertificateExtensions::parseTEEAttestationExtension() {
    void* extension_data_ptr = extensions_sequence_.get_content_ptr();
    uint32_t extensions_remaining = extensions_sequence_.get_content_length();
    const uint8_t* target_oid = TEE_ATTESTATION_OID;
    size_t target_oid_length = TEE_ATTESTATION_OID_LENGTH;
    
    while (extensions_remaining > 0) {
        ASN1Element extension_sequence((uint8_t*)extension_data_ptr);
        if (extension_sequence.tag != ASN1_SEQUENCE) {
            LOGE("[Native-TEE] 错误: 扩展序列格式错误");
            break;
        }
        
        ASN1Element extension_id((uint8_t*)extension_sequence.get_content_ptr());
        
        if (extension_id.tag != ASN1_OBJECT_IDENTIFIER) {
            extension_data_ptr = extension_sequence.get_next_body_ptr();
            extensions_remaining -= extension_sequence.get_next_body_offset();
            continue;
        }
        
        uint8_t* oid_data = (uint8_t*)extension_id.get_content_ptr();
        uint32_t oid_len = extension_id.get_content_length();
        
        if (oid_len == target_oid_length && memcmp(oid_data, target_oid, target_oid_length) == 0) {
            LOGE("[Native-TEE] ✓ 找到TEE认证扩展！");
            // 使用 TEEAttestationExtension 类解析
            tee_extension_ = new TEEAttestationExtension(extension_sequence);
            return tee_extension_->getParseResult();
        }
        
        extension_data_ptr = extension_sequence.get_next_body_ptr();
        extensions_remaining -= extension_sequence.get_next_body_offset();
    }
    
    LOGE("[Native-TEE] 未找到TEE认证扩展");
    return -1;
}

CertificateExtensions::~CertificateExtensions() {
    delete tee_extension_;
}



// ========== TEEAttestationExtension.cpp ==========
TEEAttestationExtension::TEEAttestationExtension(ASN1Element& extension_sequence)
    : critical_(false) {
    // 解析扩展序列
    // 扩展序列结构: SEQUENCE { OID, BOOLEAN (可选), OCTET STRING }
    
    void* ext_ptr = extension_sequence.get_content_ptr();
    uint32_t ext_remaining = extension_sequence.get_content_length();
    
    // 1. 解析OID
    ASN1Element ext_oid((uint8_t*)ext_ptr);
    if (ext_oid.tag != ASN1_OBJECT_IDENTIFIER) {
        LOGE("[Native-TEE] 错误: TEE扩展OID格式错误");
        return;
    }
    oid_ = parseOID(ext_oid);
    
    // 验证OID是否为TEE认证扩展
    const uint8_t* target_oid = TEE_ATTESTATION_OID;
    size_t target_oid_length = TEE_ATTESTATION_OID_LENGTH;
    uint8_t* oid_data = (uint8_t*)ext_oid.get_content_ptr();
    uint32_t oid_len = ext_oid.get_content_length();
    
    if (oid_len != target_oid_length || memcmp(oid_data, target_oid, target_oid_length) != 0) {
        LOGE("[Native-TEE] 错误: 不是TEE认证扩展OID");
        return;
    }
    
    ext_ptr = ext_oid.get_next_body_ptr();
    ext_remaining -= ext_oid.get_next_body_offset();
    
    // 2. 解析Critical标志（可选，默认为FALSE）
    if (ext_remaining > 0) {
        ASN1Element critical_elem((uint8_t*)ext_ptr);
        if (critical_elem.tag == ASN1_BOOLEAN) {
            uint8_t val = *((uint8_t*)critical_elem.get_content_ptr());
            critical_ = (val == 0xFF);
            ext_ptr = critical_elem.get_next_body_ptr();
            ext_remaining -= critical_elem.get_next_body_offset();
        }
    }
    
    // 3. 解析扩展值（OCTET STRING）
    if (ext_remaining > 0) {
        ASN1Element extension_value((uint8_t*)ext_ptr);
        if (extension_value.tag == ASN1_OCTET_STRING) {
            parse_result_ = parseExtensionContent(extension_value);
        } else {
            LOGE("[Native-TEE] 错误: TEE扩展值格式错误，tag=0x%02X", extension_value.tag);
            parse_result_ = -1;
        }
    } else {
        LOGE("[Native-TEE] 错误: 未找到TEE扩展值");
        parse_result_ = -1;
    }
}

string TEEAttestationExtension::parseOID(const ASN1Element& oid_elem) {
    if (oid_elem.tag != ASN1_OBJECT_IDENTIFIER) {
        return "Not an OID";
    }
    
    uint8_t* oid_data = (uint8_t*)oid_elem.get_content_ptr();
    uint32_t oid_len = oid_elem.get_content_length();
    
    if (oid_len == 0) return "Empty OID";
    
    // 解析OID的第一个字节（前两个数字）
    uint32_t first = oid_data[0] / 40;
    uint32_t second = oid_data[0] % 40;
    
    string oid_str = to_string(first) + "." + to_string(second);
    
    // 解析后续字节
    uint32_t value = 0;
    for (uint32_t i = 1; i < oid_len; i++) {
        value = (value << 7) | (oid_data[i] & 0x7F);
        if ((oid_data[i] & 0x80) == 0) {
            oid_str += "." + to_string(value);
            value = 0;
        }
    }
    
    // 常见OID映射
    if (oid_str == "1.3.6.1.4.1.11129.2.1.17") return "Android TEE Key Attestation (1.3.6.1.4.1.11129.2.1.17)";
    
    return oid_str;
}

int TEEAttestationExtension::parseExtensionContent(ASN1Element& extension_value) {
    LOGE("[Native-TEE] 扩展值 (OCTET STRING): length=%u", extension_value.get_content_length());
    
    // 扩展值是一个OCTET STRING，其内容是认证记录的DER编码
    // 直接解析认证记录序列
    ASN1Element attestation_sequence((uint8_t*)extension_value.get_content_ptr());
    
    if (attestation_sequence.tag != ASN1_SEQUENCE) {
        LOGE("[Native-TEE] 错误: 认证记录不是SEQUENCE格式，tag=0x%02X", attestation_sequence.tag);
        return -1;
    }
    
    return parseAttestationRecord(attestation_sequence);
}

int TEEAttestationExtension::parseAttestationRecord(ASN1Element& attestation_sequence) {
    LOGE("[Native-TEE] ========== 开始解析认证记录字段 ==========");
    
    // 创建认证记录对象
    attestation_record_ = new AttestationRecord();
    
    // 使用指针跟踪当前位置，支持灵活的解析
    void* current_ptr = attestation_sequence.get_content_ptr();
    uint32_t remaining = attestation_sequence.get_content_length();
    
    // 1. Attestation Version (INTEGER)
    ASN1Element attestation_version((uint8_t*)current_ptr);
    uint32_t attestation_version_value = 0;
    if (attestation_version.tag == ASN1_INTEGER) {
        attestation_version_value = (uint32_t)attestation_version.getIntegerValue();
        attestation_record_->setAttestationVersion(attestation_version_value);
        const char* version_names[] = {"v1 (100)", "v2 (200)", "v3 (300)", "v4 (400)"};
        int version_idx = (attestation_version_value / 100) - 1;
        LOGE("[Native-TEE] Attestation Version: %u %s", attestation_version_value, 
             (version_idx >= 0 && version_idx < 4) ? version_names[version_idx] : "");
        current_ptr = attestation_version.get_next_body_ptr();
        remaining -= attestation_version.get_next_body_offset();
    } else {
        LOGE("[Native-TEE] Attestation Version: tag=0x%02X, length=%u (格式错误，应为INTEGER)", 
         attestation_version.tag, attestation_version.get_content_length());
        // 即使格式错误，也尝试继续解析
        current_ptr = attestation_version.get_next_body_ptr();
        remaining -= attestation_version.get_next_body_offset();
    }
    
    // 2. Attestation Security Level (ENUMERATED)
    // 根据 JDK 的处理方式：如果是构造类型，尝试从内部提取
    ASN1Element attestation_security_level((uint8_t*)current_ptr);
    uint8_t security_level_value = 255; // Unknown
    
    // 如果是构造类型，尝试从内部提取（类似 JDK 的处理方式）
    if (attestation_security_level.is_constructed()) {
        ASN1Element inner_level((uint8_t*)attestation_security_level.get_content_ptr());
        if (inner_level.tag == ASN1_ENUMERATED) {
            security_level_value = inner_level.getEnumeratedValue();
        } else if (inner_level.tag == ASN1_INTEGER) {
            security_level_value = (uint8_t)inner_level.getIntegerValue();
        }
    } else if (attestation_security_level.tag == ASN1_ENUMERATED) {
        security_level_value = attestation_security_level.getEnumeratedValue();
    } else if (attestation_security_level.tag == ASN1_INTEGER) {
        // 某些实现可能使用 INTEGER 而不是 ENUMERATED
        security_level_value = (uint8_t)attestation_security_level.getIntegerValue();
    }
    
    if (security_level_value < 3) {
        SecurityLevel sec_level = static_cast<SecurityLevel>(security_level_value);
        attestation_record_->setAttestationSecurityLevel(sec_level);
        const char* level_names[] = {"Software", "TEE", "StrongBox"};
        LOGE("[Native-TEE] Attestation Security Level: %d (%s)", security_level_value, 
             level_names[security_level_value]);
    } else {
        LOGE("[Native-TEE] Attestation Security Level: tag=0x%02X, length=%u (无法解析)", 
         attestation_security_level.tag, attestation_security_level.get_content_length());
    }
    current_ptr = attestation_security_level.get_next_body_ptr();
    remaining -= attestation_security_level.get_next_body_offset();
    
    // 3. Keymaster Version (INTEGER)
    // 根据 JDK 的处理方式：如果是构造类型或 OCTET STRING，尝试从内部提取
    ASN1Element keymaster_version((uint8_t*)current_ptr);
    uint32_t km_version_value = 0;
    bool km_version_parsed = false;
    
    if (keymaster_version.tag == ASN1_INTEGER) {
        km_version_value = (uint32_t)keymaster_version.getIntegerValue();
        km_version_parsed = true;
    } else if (keymaster_version.tag == ASN1_OCTET_STRING) {
        // 某些实现可能使用 OCTET STRING 包装 INTEGER（类似 JDK 的 getOctetString 处理构造类型）
        uint32_t content_len = keymaster_version.get_content_length();
        if (content_len > 0 && content_len <= 4) {
            uint8_t* data = (uint8_t*)keymaster_version.get_content_ptr();
            for (uint32_t i = 0; i < content_len; i++) {
                km_version_value = (km_version_value << 8) | data[i];
            }
            km_version_parsed = true;
        }
    } else if (keymaster_version.is_constructed()) {
        // 如果是构造类型，尝试从内部提取
        ASN1Element inner_version((uint8_t*)keymaster_version.get_content_ptr());
        if (inner_version.tag == ASN1_INTEGER) {
            km_version_value = (uint32_t)inner_version.getIntegerValue();
            km_version_parsed = true;
        }
    }
    
    if (km_version_parsed) {
        attestation_record_->setKeymasterVersion(km_version_value);
        LOGE("[Native-TEE] Keymaster Version: %u (Keymaster %u.0)", km_version_value, km_version_value);
    } else {
        LOGE("[Native-TEE] Keymaster Version: tag=0x%02X, length=%u (无法解析)", 
         keymaster_version.tag, keymaster_version.get_content_length());
    }
    current_ptr = keymaster_version.get_next_body_ptr();
    remaining -= keymaster_version.get_next_body_offset();
    
    // 4. Keymaster Security Level (ENUMERATED)
    // 根据 JDK 的处理方式：支持多种格式
    ASN1Element keymaster_security_level((uint8_t*)current_ptr);
    uint8_t km_level_value = 255; // Unknown
    
    if (keymaster_security_level.is_constructed()) {
        ASN1Element inner_level((uint8_t*)keymaster_security_level.get_content_ptr());
        if (inner_level.tag == ASN1_ENUMERATED) {
            km_level_value = inner_level.getEnumeratedValue();
        } else if (inner_level.tag == ASN1_INTEGER) {
            km_level_value = (uint8_t)inner_level.getIntegerValue();
        }
    } else if (keymaster_security_level.tag == ASN1_ENUMERATED) {
        km_level_value = keymaster_security_level.getEnumeratedValue();
    } else if (keymaster_security_level.tag == ASN1_INTEGER) {
        km_level_value = (uint8_t)keymaster_security_level.getIntegerValue();
    } else if (keymaster_security_level.tag == ASN1_OCTET_STRING) {
        // 某些实现可能使用 OCTET STRING 包装 ENUMERATED
        uint32_t content_len = keymaster_security_level.get_content_length();
        if (content_len > 0) {
            uint8_t* data = (uint8_t*)keymaster_security_level.get_content_ptr();
            km_level_value = data[0];
        }
    }
    
    if (km_level_value < 3) {
        SecurityLevel km_sec_level = static_cast<SecurityLevel>(km_level_value);
        attestation_record_->setKeymasterSecurityLevel(km_sec_level);
        const char* level_names[] = {"Software", "TEE", "StrongBox"};
        LOGE("[Native-TEE] Keymaster Security Level: %d (%s)", km_level_value, 
             level_names[km_level_value]);
    } else {
        LOGE("[Native-TEE] Keymaster Security Level: tag=0x%02X, length=%u (无法解析)", 
         keymaster_security_level.tag, keymaster_security_level.get_content_length());
    }
    current_ptr = keymaster_security_level.get_next_body_ptr();
    remaining -= keymaster_security_level.get_next_body_offset();
    
    // 5. Attestation Challenge (OCTET STRING)
    // 根据 JDK 的 getOctetString 处理方式：支持构造类型和直接 OCTET STRING
    ASN1Element attestation_challenge((uint8_t*)current_ptr);
    bool challenge_parsed = false;
    
    if (attestation_challenge.tag == ASN1_OCTET_STRING) {
        uint32_t challenge_len = attestation_challenge.get_content_length();
        if (challenge_len > 0) {
            uint8_t* challenge_data = (uint8_t*)attestation_challenge.get_content_ptr();
            vector<uint8_t> challenge(challenge_data, challenge_data + challenge_len);
            attestation_record_->setAttestationChallenge(challenge);
            LOGE("[Native-TEE] Attestation Challenge: length=%u bytes", challenge_len);
            if (challenge_len <= 32) {
                string hex_str;
                for (uint32_t i = 0; i < challenge_len && i < 16; i++) {
                    char hex[4];
                    snprintf(hex, sizeof(hex), "%02X ", challenge_data[i]);
                    hex_str += hex;
                }
                LOGE("[Native-TEE] Challenge数据: %s", hex_str.c_str());
            }
            challenge_parsed = true;
        }
    } else if (attestation_challenge.is_constructed() || attestation_challenge.tag == ASN1_SEQUENCE) {
        // 如果是构造类型或 SEQUENCE，尝试从内部提取（类似 JDK 的 getOctetString）
        ASN1Element inner_challenge((uint8_t*)attestation_challenge.get_content_ptr());
        if (inner_challenge.tag == ASN1_OCTET_STRING) {
            uint32_t challenge_len = inner_challenge.get_content_length();
            if (challenge_len > 0) {
                uint8_t* challenge_data = (uint8_t*)inner_challenge.get_content_ptr();
                vector<uint8_t> challenge(challenge_data, challenge_data + challenge_len);
                attestation_record_->setAttestationChallenge(challenge);
                LOGE("[Native-TEE] Attestation Challenge: length=%u bytes [使用构造类型包装]", challenge_len);
                if (challenge_len <= 32) {
                    string hex_str;
                    for (uint32_t i = 0; i < challenge_len && i < 16; i++) {
                        char hex[4];
                        snprintf(hex, sizeof(hex), "%02X ", challenge_data[i]);
                        hex_str += hex;
                    }
                    LOGE("[Native-TEE] Challenge数据: %s", hex_str.c_str());
                }
                challenge_parsed = true;
            }
        }
    }
    
    if (!challenge_parsed) {
        LOGE("[Native-TEE] Attestation Challenge: tag=0x%02X, length=%u (无法解析)", 
         attestation_challenge.tag, attestation_challenge.get_content_length());
    }
    current_ptr = attestation_challenge.get_next_body_ptr();
    remaining -= attestation_challenge.get_next_body_offset();
    
    // 6. Unique ID (OCTET STRING, 可选)
    void* next_ptr = current_ptr;
    ASN1Element unique_id((uint8_t*)next_ptr);
    if (unique_id.tag == ASN1_OCTET_STRING) {
        uint32_t unique_id_len = unique_id.get_content_length();
        if (unique_id_len > 0) {
            uint8_t* unique_id_data = (uint8_t*)unique_id.get_content_ptr();
            vector<uint8_t> uid(unique_id_data, unique_id_data + unique_id_len);
            attestation_record_->setUniqueId(uid);
        }
        LOGE("[Native-TEE] Unique ID: tag=0x%02X, length=%u", 
             unique_id.tag, unique_id_len);
        next_ptr = unique_id.get_next_body_ptr();
    } else {
        LOGE("[Native-TEE] Unique ID: 未找到（可选字段）");
    }
    
    // 7. Software Enforced (SEQUENCE - Authorization List)
    ASN1Element software_enforced((uint8_t*)next_ptr);
    if (software_enforced.tag == ASN1_SEQUENCE) {
        AuthorizationList* software_list = new AuthorizationList();
        AuthorizationListParser software_parser(software_enforced.get_content_ptr(), 
                                              software_enforced.get_content_length(), 
                                              "Software Enforced");
        software_parser.parse();
        *software_list = software_parser.getAuthorizationList();
        attestation_record_->setSoftwareEnforced(software_list);
    } else {
        LOGE("[Native-TEE] Software Enforced: tag=0x%02X, length=%u (格式错误)", 
             software_enforced.tag, software_enforced.get_content_length());
    }
    
    // 8. TEE Enforced (SEQUENCE - Authorization List)
    ASN1Element tee_enforced((uint8_t*)software_enforced.get_next_body_ptr());
    if (tee_enforced.tag == ASN1_SEQUENCE) {
        AuthorizationList* tee_list = new AuthorizationList();
        AuthorizationListParser tee_parser(tee_enforced.get_content_ptr(), 
                                         tee_enforced.get_content_length(), 
                                         "TEE Enforced");
        tee_parser.parse();
        *tee_list = tee_parser.getAuthorizationList();
        attestation_record_->setTEEEnforced(tee_list);
    } else {
        LOGE("[Native-TEE] TEE Enforced: tag=0x%02X, length=%u (格式错误)", 
             tee_enforced.tag, tee_enforced.get_content_length());
    }
    
    return 0;
}

TEEAttestationExtension::~TEEAttestationExtension() {
    delete attestation_record_;
}



// ========== TBSCertificate.cpp ==========
TBSCertificate::TBSCertificate(ASN1Element& tbs_element) {
    if (tbs_element.tag != ASN1_SEQUENCE) {
        LOGE("[Native-TEE] 错误: TBSCertificate不是SEQUENCE格式，tag=0x%02X", tbs_element.tag);
        parse_result_ = -1;
        return;
    }
    
    LOGE("[Native-TEE] TBSCertificate序列: tag=0x%02X, tag_number=%u, length=%u, tag_size=%u, length_size=%u", 
         tbs_element.tag, tbs_element.get_tag_number(), 
         tbs_element.get_content_length(), tbs_element.tag_size, tbs_element.length_size);
    
    void* tbs_current_ptr = tbs_element.get_content_ptr();
    
    // 检查是否有版本字段（Context[0]）
    ASN1Element first_field((uint8_t*)tbs_current_ptr);
    if (first_field.get_tag_class() == 2 && first_field.get_tag_number() == 0) {
        LOGE("[Native-TEE] 找到版本字段 (Context[0])");
        parseVersion(first_field);
        tbs_current_ptr = first_field.get_next_body_ptr();
    }
    
    // 解析序列号
    ASN1Element serial_number((uint8_t*)tbs_current_ptr);
    if (parseSerialNumber(serial_number) != 0) {
        parse_result_ = -1;
        return;
    }
    tbs_current_ptr = serial_number.get_next_body_ptr();
    
    // 解析签名算法
    ASN1Element signature_algorithm((uint8_t*)tbs_current_ptr);
    if (parseSignatureAlgorithm(signature_algorithm) != 0) {
        parse_result_ = -1;
        return;
    }

    LOGE("1111111111111");


    tbs_current_ptr = signature_algorithm.get_next_body_ptr();
    LOGE("222222222222222");

    // 解析颁发者
    ASN1Element issuer((uint8_t*)tbs_current_ptr);
    if (parseIssuer(issuer) != 0) {
        parse_result_ = -1;
        return;
    }

    LOGE("3333333333333");

    tbs_current_ptr = issuer.get_next_body_ptr();
    
    // 解析有效期
    ASN1Element validity((uint8_t*)tbs_current_ptr);
    if (parseValidity(validity) != 0) {
        parse_result_ = -1;
        return;
    }
    tbs_current_ptr = validity.get_next_body_ptr();
    
    // 解析主题
    ASN1Element subject((uint8_t*)tbs_current_ptr);
    if (parseSubject(subject) != 0) {
        parse_result_ = -1;
        return;
    }
    tbs_current_ptr = subject.get_next_body_ptr();
    
    // 解析主题公钥信息
    ASN1Element subject_public_key_info((uint8_t*)tbs_current_ptr);
    if (parseSubjectPublicKeyInfo(subject_public_key_info) != 0) {
        parse_result_ = -1;
        return;
    }
    tbs_current_ptr = subject_public_key_info.get_next_body_ptr();
    
    // 解析扩展字段
    ASN1Element extensions_field((uint8_t*)tbs_current_ptr);
    if (extensions_field.get_tag_class() != 2 || extensions_field.get_tag_number() != 3) {
        LOGE("[Native-TEE] 错误: 未找到扩展字段 (Context[3])");
        parse_result_ = -1;
        return;
    }
    LOGE("[Native-TEE] 扩展字段 (Context[3]): tag=0x%02X, length=%u", 
         extensions_field.tag, extensions_field.get_content_length());
    
    if (parseExtensions(extensions_field) != 0) {
        parse_result_ = -1;
        return;
    }
    
    parse_result_ = 0;
}

int TBSCertificate::parseVersion(ASN1Element& version_field) {
    ASN1Element version_int((uint8_t*)version_field.get_content_ptr());
    if (version_int.tag == ASN1_INTEGER) {
        version_ = (uint32_t)version_int.getIntegerValue();
        LOGE("[Native-TEE] 版本: %u (v%u)", version_, version_ + 1);
    }
    return 0;
}

int TBSCertificate::parseSerialNumber(ASN1Element& serial_elem) {
    LOGE("[Native-TEE] 序列号: tag=0x%02X, tag_number=%u, length=%u, tag_size=%u, length_size=%u", 
         serial_elem.tag, serial_elem.get_tag_number(), serial_elem.get_content_length(),
         serial_elem.tag_size, serial_elem.length_size);
    
    if (serial_elem.tag == ASN1_INTEGER) {
        uint8_t* serial_data = (uint8_t*)serial_elem.get_content_ptr();
        uint32_t serial_len = serial_elem.get_content_length();
        serial_number_.assign(serial_data, serial_data + serial_len);
    } else {
        LOGE("[Native-TEE] 警告: 序列号不是INTEGER格式，tag=0x%02X", serial_elem.tag);
    }
    return 0;
}

int TBSCertificate::parseSignatureAlgorithm(ASN1Element& sig_alg_elem) {
    if (sig_alg_elem.tag == ASN1_SEQUENCE) {
        ASN1Element sig_alg_oid((uint8_t*)sig_alg_elem.get_content_ptr());
        if (sig_alg_oid.tag == ASN1_OBJECT_IDENTIFIER) {
            signature_algorithm_ = parseOID(sig_alg_oid);
            LOGE("[Native-TEE] 签名算法: %s", signature_algorithm_.c_str());
        } else {
            LOGE("[Native-TEE] 签名算法: tag=0x%02X, length=%u (OID格式错误)", 
                 sig_alg_elem.tag, sig_alg_elem.get_content_length());
        }
    } else {
        LOGE("[Native-TEE] 签名算法: tag=0x%02X, length=%u (格式错误)", 
             sig_alg_elem.tag, sig_alg_elem.get_content_length());
    }
    return 0;
}

int TBSCertificate::parseIssuer(ASN1Element& issuer_elem) {
    if (issuer_elem.tag == ASN1_SEQUENCE) {
        issuer_ = parseDN(issuer_elem);
        LOGE("[Native-TEE] 颁发者: %s", issuer_.c_str());
    } else {
        LOGE("[Native-TEE] 颁发者: tag=0x%02X, length=%u (格式错误)", 
             issuer_elem.tag, issuer_elem.get_content_length());
    }
    return 0;
}

int TBSCertificate::parseValidity(ASN1Element& validity_elem) {
    if (validity_elem.tag == ASN1_SEQUENCE) {
        ASN1Element not_before((uint8_t*)validity_elem.get_content_ptr());
        not_before_ = parseTime(not_before);
        
        ASN1Element not_after((uint8_t*)not_before.get_next_body_ptr());
        not_after_ = parseTime(not_after);
        
        LOGE("[Native-TEE] 有效期: Not Before: %s, Not After: %s", 
             not_before_.c_str(), not_after_.c_str());
    } else {
        LOGE("[Native-TEE] 有效期: tag=0x%02X, length=%u (格式错误)", 
             validity_elem.tag, validity_elem.get_content_length());
    }
    return 0;
}

int TBSCertificate::parseSubject(ASN1Element& subject_elem) {
    if (subject_elem.tag == ASN1_SEQUENCE) {
        subject_ = parseDN(subject_elem);
        LOGE("[Native-TEE] 主题: %s", subject_.c_str());
    } else {
        LOGE("[Native-TEE] 主题: tag=0x%02X, length=%u (格式错误)", 
             subject_elem.tag, subject_elem.get_content_length());
    }
    return 0;
}

int TBSCertificate::parseSubjectPublicKeyInfo(ASN1Element& pub_key_elem) {
    if (pub_key_elem.tag == ASN1_SEQUENCE) {
        ASN1Element pub_key_alg((uint8_t*)pub_key_elem.get_content_ptr());
        if (pub_key_alg.tag == ASN1_SEQUENCE) {
            ASN1Element pub_key_alg_oid((uint8_t*)pub_key_alg.get_content_ptr());
            if (pub_key_alg_oid.tag == ASN1_OBJECT_IDENTIFIER) {
                subject_public_key_algorithm_ = parseOID(pub_key_alg_oid);
                ASN1Element pub_key_bit_string((uint8_t*)pub_key_alg.get_next_body_ptr());
                if (pub_key_bit_string.tag == 0x03) { // BIT STRING
                    uint32_t key_len = pub_key_bit_string.get_content_length();
                    subject_public_key_length_ = (key_len > 0 ? (key_len - 1) * 8 : 0);
                    LOGE("[Native-TEE] 主题公钥信息: 算法=%s, 公钥长度=%u bits (BIT STRING长度=%u字节)", 
                         subject_public_key_algorithm_.c_str(), subject_public_key_length_, key_len);
                } else {
                    LOGE("[Native-TEE] 主题公钥信息: 算法=%s, 公钥格式错误", subject_public_key_algorithm_.c_str());
                }
            } else {
                LOGE("[Native-TEE] 主题公钥信息: 算法OID格式错误");
            }
        } else {
            LOGE("[Native-TEE] 主题公钥信息: tag=0x%02X, length=%u (格式错误)", 
                 pub_key_elem.tag, pub_key_elem.get_content_length());
        }
    } else {
        LOGE("[Native-TEE] 主题公钥信息: tag=0x%02X, length=%u (格式错误)", 
             pub_key_elem.tag, pub_key_elem.get_content_length());
    }
    return 0;
}

int TBSCertificate::parseExtensions(ASN1Element& extensions_field) {
    // 解析扩展序列 (包含多个扩展对象)
    ASN1Element extensions_sequence((uint8_t*)extensions_field.get_content_ptr());
    if (extensions_sequence.tag != ASN1_SEQUENCE) {
        LOGE("[Native-TEE] 错误: 扩展序列格式错误");
        return -1;
    }
    LOGE("[Native-TEE] 扩展序列: tag=0x%02X, length=%u", 
         extensions_sequence.tag, extensions_sequence.get_content_length());
    
    // 使用 CertificateExtensions 类处理扩展（包括TEE扩展）
    // 构造函数会自动列出所有扩展并解析TEE扩展
    extensions_ = new CertificateExtensions(extensions_sequence);
    
    return extensions_->getParseResult();
}

TBSCertificate::~TBSCertificate() {
    delete extensions_;
}

string TBSCertificate::parseOID(const ASN1Element& oid_elem) {
    if (oid_elem.tag != ASN1_OBJECT_IDENTIFIER) {
        return "Not an OID";
    }
    
    uint8_t* oid_data = (uint8_t*)oid_elem.get_content_ptr();
    uint32_t oid_len = oid_elem.get_content_length();
    
    if (oid_len == 0) return "Empty OID";
    
    // 解析OID的第一个字节（前两个数字）
    uint32_t first = oid_data[0] / 40;
    uint32_t second = oid_data[0] % 40;
    
    string oid_str = to_string(first) + "." + to_string(second);
    
    // 解析后续字节
    uint32_t value = 0;
    for (uint32_t i = 1; i < oid_len; i++) {
        value = (value << 7) | (oid_data[i] & 0x7F);
        if ((oid_data[i] & 0x80) == 0) {
            oid_str += "." + to_string(value);
            value = 0;
        }
    }
    
    // 常见OID映射
    if (oid_str == "1.2.840.113549.1.1.11") return "sha256WithRSAEncryption (1.2.840.113549.1.1.11)";
    if (oid_str == "1.2.840.113549.1.1.5") return "sha1WithRSAEncryption (1.2.840.113549.1.1.5)";
    if (oid_str == "1.2.840.10045.4.3.2") return "ecdsa-with-SHA256 (1.2.840.10045.4.3.2)";
    if (oid_str == "1.2.840.10045.4.3.3") return "ecdsa-with-SHA384 (1.2.840.10045.4.3.3)";
    if (oid_str == "1.2.840.10045.4.3.4") return "ecdsa-with-SHA512 (1.2.840.10045.4.3.4)";
    if (oid_str == "1.2.840.10045.2.1") return "ecPublicKey (1.2.840.10045.2.1)";
    if (oid_str == "1.2.840.113549.1.1.1") return "rsaEncryption (1.2.840.113549.1.1.1)";
    if (oid_str == "1.3.6.1.4.1.11129.2.1.17") return "Android TEE Key Attestation (1.3.6.1.4.1.11129.2.1.17)";
    
    return oid_str;
}

string TBSCertificate::parseTime(const ASN1Element& time_elem) {
    uint8_t* time_data = (uint8_t*)time_elem.get_content_ptr();
    uint32_t time_len = time_elem.get_content_length();
    
    if (time_elem.tag == ASN1_UTCTIME) {
        // UTCTime格式: YYMMDDHHMMSSZ (13字节) 或 YYMMDDHHMMSS+/-HHMM (17字节)
        if (time_len >= 13) {
            char year_str[3] = {(char)time_data[0], (char)time_data[1], '\0'};
            int year = atoi(year_str);
            year = (year < 50) ? 2000 + year : 1900 + year;
            
            char month_str[3] = {(char)time_data[2], (char)time_data[3], '\0'};
            char day_str[3] = {(char)time_data[4], (char)time_data[5], '\0'};
            char hour_str[3] = {(char)time_data[6], (char)time_data[7], '\0'};
            char min_str[3] = {(char)time_data[8], (char)time_data[9], '\0'};
            char sec_str[3] = {(char)time_data[10], (char)time_data[11], '\0'};
            
            char time_str[64];
            snprintf(time_str, sizeof(time_str), "%04d-%s-%s %s:%s:%s UTC", 
                     year, month_str, day_str, hour_str, min_str, sec_str);
            return string(time_str);
        }
    } else if (time_elem.tag == ASN1_GENERALIZEDTIME) {
        // GeneralizedTime格式: YYYYMMDDHHMMSSZ (15字节) 或 YYYYMMDDHHMMSS+/-HHMM (19字节)
        if (time_len >= 15) {
            char year_str[5] = {(char)time_data[0], (char)time_data[1], (char)time_data[2], (char)time_data[3], '\0'};
            char month_str[3] = {(char)time_data[4], (char)time_data[5], '\0'};
            char day_str[3] = {(char)time_data[6], (char)time_data[7], '\0'};
            char hour_str[3] = {(char)time_data[8], (char)time_data[9], '\0'};
            char min_str[3] = {(char)time_data[10], (char)time_data[11], '\0'};
            char sec_str[3] = {(char)time_data[12], (char)time_data[13], '\0'};
            
            char time_str[64];
            snprintf(time_str, sizeof(time_str), "%s-%s-%s %s:%s:%s UTC", 
                     year_str, month_str, day_str, hour_str, min_str, sec_str);
            return string(time_str);
        }
    }
    
    return "Invalid Time";
}

string TBSCertificate::parseDN(const ASN1Element& dn_elem) {
    if (dn_elem.tag != ASN1_SEQUENCE) {
        return "Invalid DN";
    }
    
    string result;
    void* dn_ptr = dn_elem.get_content_ptr();
    uint32_t dn_remaining = dn_elem.get_content_length();
    uint32_t dn_offset = 0;
    vector<string> dn_parts;
    
    while (dn_offset < dn_remaining) {
        ASN1Element rdn_set((uint8_t*)dn_ptr + dn_offset);
        if (rdn_set.tag != ASN1_SET) break;
        
        void* set_ptr = rdn_set.get_content_ptr();
        uint32_t set_remaining = rdn_set.get_content_length();
        uint32_t set_offset = 0;
        
        while (set_offset < set_remaining) {
            ASN1Element rdn_seq((uint8_t*)set_ptr + set_offset);
            if (rdn_seq.tag != ASN1_SEQUENCE) break;
            
            ASN1Element attr_type((uint8_t*)rdn_seq.get_content_ptr());
            if (attr_type.tag == ASN1_OBJECT_IDENTIFIER) {
                string oid = parseOID(attr_type);
                ASN1Element attr_value((uint8_t*)attr_type.get_next_body_ptr());
                
                string value_str;
                if (attr_value.tag == ASN1_PRINTABLE_STRING || 
                    attr_value.tag == ASN1_UTF8_STRING || 
                    attr_value.tag == ASN1_IA5_STRING) {
                    uint32_t val_len = attr_value.get_content_length();
                    value_str = string((char*)attr_value.get_content_ptr(), val_len);
                }
                
                // 简化OID到名称映射
                if (oid.find("2.5.4.3") != string::npos) {
                    dn_parts.push_back("CN=" + value_str);
                } else if (oid.find("2.5.4.10") != string::npos) {
                    dn_parts.push_back("O=" + value_str);
                } else if (oid.find("2.5.4.6") != string::npos) {
                    dn_parts.push_back("C=" + value_str);
                } else {
                    dn_parts.push_back(oid + "=" + value_str);
                }
            }
            
            set_offset += rdn_seq.get_next_body_offset();
        }
        
        dn_offset += rdn_set.get_next_body_offset();
    }
    
    for (size_t i = 0; i < dn_parts.size(); i++) {
        if (i > 0) result += ", ";
        result += dn_parts[i];
    }
    
    return result.empty() ? "Empty DN" : result;
}



// ========== X509Certificate.cpp ==========
X509Certificate::X509Certificate(vector<uint8_t> data) {
    if (data.empty()) {
        parse_result_ = -1;
        return;
    }
    cert_data_ = std::move(data);
    parse_result_ = parseCertificate(cert_data_);
}

X509Certificate::X509Certificate(const string& filepath) 
    : X509Certificate(readCertFile(filepath)) {
}

X509Certificate::X509Certificate(const uint8_t* data, size_t length) 
    : X509Certificate(vector<uint8_t>(data, data + length)) {
}

vector<uint8_t> X509Certificate::readCertFile(const string& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOGE("[Native-TEE] 错误: 无法打开文件: %s", filename.c_str());
        return vector<uint8_t>();
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    vector<uint8_t> buffer(size);
    if (!file.read((char*)buffer.data(), size)) {
        LOGE("[Native-TEE] 错误: 无法读取文件: %s", filename.c_str());
        return vector<uint8_t>();
    }
    
    LOGE("[Native-TEE] 成功读取文件: %s, 大小: %zu bytes", filename.c_str(), buffer.size());
    return buffer;
}

int X509Certificate::parseCertificate(const vector<uint8_t>& cert) {
    // 解析主证书序列 (最外层的ASN.1结构)
    ASN1Element certificate_sequence(cert.data());
    LOGE("[Native-TEE] 证书序列: tag=0x%02X, tag_number=%u, length=%u, tag_size=%u, length_size=%u", 
         certificate_sequence.tag, certificate_sequence.get_tag_number(), 
         certificate_sequence.get_content_length(), certificate_sequence.tag_size, certificate_sequence.length_size);
    
    // 验证证书序列标签
    if (certificate_sequence.tag != ASN1_SEQUENCE) {
        LOGE("[Native-TEE] 错误: 证书不是SEQUENCE格式，tag=0x%02X", certificate_sequence.tag);
        return -1;
    }
    
    uint32_t cert_total_size = certificate_sequence.get_next_body_offset();
    LOGE("[Native-TEE] 证书序列总大小: %u bytes (文件大小: %zu bytes)", cert_total_size, cert.size());
    
    if (cert_total_size > cert.size()) {
        LOGE("[Native-TEE] 警告: 证书声明长度(%u)超过文件大小(%zu)", cert_total_size, cert.size());
    }
    
    // 解析TBSCertificate序列 (待签名证书)
    void* tbs_ptr = certificate_sequence.get_content_ptr();
    LOGE("[Native-TEE] TBSCertificate起始位置: %p (偏移: %td), 前4字节: 0x%02X 0x%02X 0x%02X 0x%02X", 
         tbs_ptr, (uint8_t*)tbs_ptr - cert.data(),
         ((uint8_t*)tbs_ptr)[0], ((uint8_t*)tbs_ptr)[1], 
         ((uint8_t*)tbs_ptr)[2], ((uint8_t*)tbs_ptr)[3]);
    
    // 验证TBSCertificate是否在文件范围内
    if ((uint8_t*)tbs_ptr >= cert.data() + cert.size()) {
        LOGE("[Native-TEE] 错误: TBSCertificate起始位置超出文件范围");
        return -1;
    }
    
    ASN1Element tbs_element((uint8_t*)tbs_ptr);
    tbs_certificate_ = new TBSCertificate(tbs_element);
    if (tbs_certificate_->getParseResult() != 0) {
        return -1;
    }
    
    // 解析签名算法
    void* sig_alg_ptr = tbs_element.get_next_body_ptr();
    ASN1Element signature_algorithm((uint8_t*)sig_alg_ptr);
    if (signature_algorithm.tag == ASN1_SEQUENCE) {
        ASN1Element sig_alg_oid((uint8_t*)signature_algorithm.get_content_ptr());
        if (sig_alg_oid.tag == 0x06) { // ASN1_OBJECT_IDENTIFIER
            signature_algorithm_ = parseOID(sig_alg_oid);
            LOGE("[Native-TEE] 签名算法: %s", signature_algorithm_.c_str());
        }
    }
    
    // 解析签名值
    void* sig_value_ptr = signature_algorithm.get_next_body_ptr();
    ASN1Element signature_value((uint8_t*)sig_value_ptr);
    if (signature_value.tag == 0x03) { // BIT STRING
        uint8_t* sig_data = (uint8_t*)signature_value.get_content_ptr();
        uint32_t sig_len = signature_value.get_content_length();
        // BIT STRING的第一个字节是未使用的位数，跳过
        if (sig_len > 0) {
            signature_value_.assign(sig_data + 1, sig_data + sig_len);
            LOGE("[Native-TEE] 签名值: %u bytes", sig_len - 1);
        }
    }
    
    return 0;
}

string X509Certificate::parseOID(const ASN1Element& oid_elem) {
    if (oid_elem.tag != 0x06) { // ASN1_OBJECT_IDENTIFIER
        return "Not an OID";
    }
    
    uint8_t* oid_data = (uint8_t*)oid_elem.get_content_ptr();
    uint32_t oid_len = oid_elem.get_content_length();
    
    if (oid_len == 0) return "Empty OID";
    
    // 解析OID的第一个字节（前两个数字）
    uint32_t first = oid_data[0] / 40;
    uint32_t second = oid_data[0] % 40;
    
    string oid_str = to_string(first) + "." + to_string(second);
    
    // 解析后续字节
    uint32_t value = 0;
    for (uint32_t i = 1; i < oid_len; i++) {
        value = (value << 7) | (oid_data[i] & 0x7F);
        if ((oid_data[i] & 0x80) == 0) {
            oid_str += "." + to_string(value);
            value = 0;
        }
    }
    
    // 常见OID映射
    if (oid_str == "1.2.840.113549.1.1.11") return "sha256WithRSAEncryption (1.2.840.113549.1.1.11)";
    if (oid_str == "1.2.840.113549.1.1.5") return "sha1WithRSAEncryption (1.2.840.113549.1.1.5)";
    if (oid_str == "1.2.840.10045.4.3.2") return "ecdsa-with-SHA256 (1.2.840.10045.4.3.2)";
    if (oid_str == "1.2.840.10045.4.3.3") return "ecdsa-with-SHA384 (1.2.840.10045.4.3.3)";
    if (oid_str == "1.2.840.10045.4.3.4") return "ecdsa-with-SHA512 (1.2.840.10045.4.3.4)";
    
    return oid_str;
}

X509Certificate::~X509Certificate() {
    delete tbs_certificate_;
}


// ========== zTeeCert.cpp ==========
zTeeCert::zTeeCert(const string& filepath) {
    cert_ = new X509Certificate(X509Certificate::readCertFile(filepath));
}

zTeeCert::zTeeCert(vector<uint8_t> data) {
    cert_ = new X509Certificate(std::move(data));
}

zTeeCert::~zTeeCert() {
    delete cert_;
}


