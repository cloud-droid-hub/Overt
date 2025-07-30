//
// Created by lxz on 2025/7/17.
// TEE Certificate Parser - C++ implementation based on BouncyCastle Java library
//


#include <string.h>
#include <string>
#include <vector>

#include "zLog.h"
#include "tee_cert_parser.h"

// TEE attestation extension OID
#define TEE_ATTESTATION_OID "1.3.6.1.4.1.11129.2.1.17"

// ASN.1 tag constants (following BouncyCastle)
#define ASN1_UNIVERSAL 0x00
#define ASN1_APPLICATION 0x40
#define ASN1_CONTEXT_SPECIFIC 0x80
#define ASN1_PRIVATE 0xC0

#define ASN1_CONSTRUCTED 0x20
#define ASN1_PRIMITIVE 0x00

#define ASN1_BOOLEAN 0x01
#define ASN1_INTEGER 0x02
#define ASN1_BIT_STRING 0x03
#define ASN1_OCTET_STRING_TAG 0x04
#define ASN1_NULL 0x05
#define ASN1_OBJECT_IDENTIFIER 0x06
#define ASN1_ENUMERATED 0x0A
#define ASN1_UTF8_STRING 0x0C
#define ASN1_SEQUENCE 0x10
#define ASN1_SET 0x11
#define ASN1_NUMERIC_STRING 0x12
#define ASN1_PRINTABLE_STRING 0x13
#define ASN1_T61_STRING 0x14
#define ASN1_VIDEOTEX_STRING 0x15
#define ASN1_IA5_STRING 0x16
#define ASN1_UTC_TIME 0x17
#define ASN1_GENERALIZED_TIME 0x18
#define ASN1_GRAPHIC_STRING 0x19
#define ASN1_VISIBLE_STRING 0x1A
#define ASN1_GENERAL_STRING 0x1B
#define ASN1_UNIVERSAL_STRING 0x1C
#define ASN1_BMP_STRING 0x1E

#define ASN1_CONSTRUCTED_SEQUENCE (ASN1_CONSTRUCTED | ASN1_SEQUENCE)

// Keymaster tags
#define KEYMASTER_TAG_TYPE_MASK 0x0000FF00
#define KM_TAG_ROOT_OF_TRUST 0x000002C0

// Verified boot states
#define VERIFIED_BOOT_STATE_VERIFIED 0
#define VERIFIED_BOOT_STATE_SELF_SIGNED 1
#define VERIFIED_BOOT_STATE_UNVERIFIED 2
#define VERIFIED_BOOT_STATE_FAILED 3

// ASN.1 object structure
typedef struct {
    int tag;
    int tag_class;
    int is_constructed;
    int length;
    const unsigned char *data;
    int data_len;
} asn1_object_t;

// Forward declarations
static int parse_asn1_tag(const unsigned char *data, int len, int *offset, asn1_object_t *obj);

static int parse_asn1_length(const unsigned char *data, int len, int *offset);

static int parse_asn1_tag_number(const unsigned char *data, int len, int *offset, int tag);

static int parse_x509_certificate(const unsigned char *data, int len, tee_info_t *tee_info);

static int parse_attestation_record(const unsigned char *data, int len, tee_info_t *tee_info);

static int find_root_of_trust_in_authorization_list(const unsigned char *data, int len,
                                                    root_of_trust_t *root_of_trust);

static int
parse_root_of_trust_sequence(const unsigned char *data, int len, root_of_trust_t *root_of_trust);

static void
print_hex_data(const char *prefix, const unsigned char *data, int len, const char *suffix);

// Get extension by OID using manual ASN.1 parsing (for debugging OID comparison)
static int get_extension_by_oid_manual(const unsigned char *data, int len, const char *oid_str,
                                       unsigned char **ext_data, int *ext_len) {
    *ext_data = NULL;
    *ext_len = 0;

    LOGD("[Native-TEE] get_extension_by_oid_manual: called with len=%d, oid=%s", len, oid_str);
    LOGD("[Native-TEE] get_extension_by_oid_manual: data pointer=%p", data);

    if (!data || len <= 0) {
        LOGE("[Native-TEE] get_extension_by_oid_manual: invalid parameters");
        return -1;
    }

    // Print first few bytes of the extensions data
    print_hex_data("ExtensionsData", data, (len < 64) ? len : 64, NULL);

    LOGE("[Native-TEE] Searching for extension OID: %s", oid_str);

    int offset = 0;
    asn1_object_t obj;

    while (offset < len) {
        LOGE("[Native-TEE] get_extension_by_oid_manual: parsing at offset %d, remaining len %d",
             offset, len - offset);

        if (parse_asn1_tag(data, len, &offset, &obj) != 0) {
            LOGE("[Native-TEE] get_extension_by_oid_manual: failed to parse tag at offset %d",
                 offset);
            break;
        }

        LOGE("[Native-TEE] get_extension_by_oid_manual: found object tag=0x%02x, length=%d, constructed=%d",
             obj.tag, obj.length, obj.is_constructed);

        if ((obj.tag & 0x1F) == ASN1_SEQUENCE && obj.is_constructed) {
            LOGE("[Native-TEE] get_extension_by_oid_manual: found SEQUENCE, parsing content");

            // Parse each extension within the sequence
            int ext_offset = 0;
            asn1_object_t ext_obj;

            while (ext_offset < obj.data_len) {
                LOGE("[Native-TEE] get_extension_by_oid_manual: parsing extension at offset %d, remaining %d",
                     ext_offset, obj.data_len - ext_offset);

                // Parse the entire extension sequence
                asn1_object_t extension_sequence;
                if (parse_asn1_tag(obj.data, obj.data_len, &ext_offset, &extension_sequence) != 0) {
                    LOGE("[Native-TEE] get_extension_by_oid_manual: failed to parse extension sequence");
                    break;
                }

                LOGE("[Native-TEE] Extension sequence: tag=0x%02x, length=%d",
                     extension_sequence.tag, extension_sequence.length);

                // Parse extension ID (OID) - first element in sequence
                int seq_offset = 0;
                if (parse_asn1_tag(extension_sequence.data, extension_sequence.data_len,
                                   &seq_offset, &ext_obj) != 0) {
                    LOGE("[Native-TEE] get_extension_by_oid_manual: failed to parse extension OID");
                    ext_offset += extension_sequence.length;
                    continue;
                }

                LOGE("[Native-TEE] Extension OID: tag=0x%02x, length=%d", ext_obj.tag,
                     ext_obj.length);
                print_hex_data("ExtensionOID", ext_obj.data, ext_obj.data_len, NULL);

                // Print OID as string for debugging
                if (ext_obj.tag == ASN1_OBJECT_IDENTIFIER) {
                    char oid_str_debug[256] = {0};
                    int oid_len = ext_obj.data_len;
                    if (oid_len > 0) {
                        // Convert ASN.1 encoded OID to string format
                        int first_byte = ext_obj.data[0];
                        int first_two = first_byte / 40;
                        int second = first_byte % 40;

                        snprintf(oid_str_debug, sizeof(oid_str_debug), "%d.%d", first_two, second);

                        unsigned long value = 0;
                        for (int i = 1; i < oid_len; i++) {
                            value = (value << 7) | (ext_obj.data[i] & 0x7F);
                            if ((ext_obj.data[i] & 0x80) == 0) {
                                snprintf(oid_str_debug + strlen(oid_str_debug),
                                         sizeof(oid_str_debug) - strlen(oid_str_debug), ".%lu",
                                         value);
                                value = 0;
                            }
                        }
                        LOGE("[Native-TEE] Extension OID as string: %s", oid_str_debug);
                    }
                }

                // Check if this is the attestation extension OID
                // ext_obj.data contains only the OID content, not the full ASN.1 object
                if (ext_obj.tag == ASN1_OBJECT_IDENTIFIER) {
                    // For "1.3.6.1.4.1.11129.2.1.17", the expected encoded bytes are:
                    // 0x2B 0x06 0x01 0x04 0x01 0xD6 0x79 0x02 0x01 0x11
                    if (ext_obj.data_len == 10) {
                        if (ext_obj.data[0] == 0x2B && ext_obj.data[1] == 0x06 &&
                            ext_obj.data[2] == 0x01 &&
                            ext_obj.data[3] == 0x04 && ext_obj.data[4] == 0x01 &&
                            ext_obj.data[5] == 0xD6 &&
                            ext_obj.data[6] == 0x79 && ext_obj.data[7] == 0x02 &&
                            ext_obj.data[8] == 0x01 &&
                            ext_obj.data[9] == 0x11) {
                            LOGE("[Native-TEE] Found attestation extension OID!");

                            // Skip critical flag if present (second element, optional)
                            if (seq_offset < extension_sequence.data_len) {
                                asn1_object_t critical_obj;
                                if (parse_asn1_tag(extension_sequence.data,
                                                   extension_sequence.data_len, &seq_offset,
                                                   &critical_obj) == 0) {
                                    if (critical_obj.tag == ASN1_BOOLEAN) {
                                        LOGE("[Native-TEE] Extension critical flag: %d",
                                             critical_obj.data[0]);
                                        // Continue to parse the extension value (third element)
                                    } else {
                                        // This is the extension value, not critical flag
                                        LOGE("[Native-TEE] No critical flag, this is the extension value: tag=0x%02x",
                                             critical_obj.tag);
                                        if (critical_obj.tag == ASN1_OCTET_STRING_TAG) {
                                            LOGE("[Native-TEE] Found attestation extension value: length=%d",
                                                 critical_obj.data_len);
                                            *ext_data = (unsigned char *) critical_obj.data;
                                            *ext_len = critical_obj.data_len;
                                            return 0;  // SUCCESS - found the extension!
                                        }
                                        // If we get here, something went wrong
                                        LOGE("[Native-TEE] Expected OCTET_STRING for extension value, got tag=0x%02x",
                                             critical_obj.tag);
                                        return -1;
                                    }
                                }
                            }

                            // Parse extension value (third element, if critical flag was present)
                            if (seq_offset < extension_sequence.data_len) {
                                if (parse_asn1_tag(extension_sequence.data,
                                                   extension_sequence.data_len, &seq_offset,
                                                   &ext_obj) == 0) {
                                    if (ext_obj.tag == ASN1_OCTET_STRING_TAG) {
                                        LOGE("[Native-TEE] Found attestation extension value: length=%d",
                                             ext_obj.data_len);
                                        *ext_data = (unsigned char *) ext_obj.data;
                                        *ext_len = ext_obj.data_len;
                                        return 0;  // SUCCESS - found the extension!
                                    } else {
                                        LOGE("[Native-TEE] Expected OCTET_STRING for extension value, got tag=0x%02x",
                                             ext_obj.tag);
                                        return -1;
                                    }
                                }
                            }

                            // If we get here, something went wrong with parsing the extension value
                            LOGE("[Native-TEE] Failed to parse extension value for OID %s",
                                 oid_str);
                            return -1;
                        }
                    }
                }

                LOGE("[Native-TEE] OID mismatch, continuing to next extension");

                // Move to next extension within the sequence
                // ext_offset is already advanced by parse_asn1_tag for the extension sequence
            }
        } else {
            LOGE("[Native-TEE] get_extension_by_oid_manual: skipping non-SEQUENCE object (tag=0x%02x)",
                 obj.tag);
        }

        // Move to next extension in the outer sequence
        LOGE("[Native-TEE] get_extension_by_oid_manual: moving to next extension, current offset=%d, obj.length=%d",
             offset, obj.length);
        offset += obj.length;
        LOGE("[Native-TEE] get_extension_by_oid_manual: new offset=%d, remaining len=%d", offset,
             len - offset);
    }

    LOGE("[Native-TEE] Attestation extension not found");
    return -1;
}

// Print detailed hex data for debugging
static void
print_hex_data(const char *prefix, const unsigned char *data, int len, const char *suffix) {
    if (len <= 0) {
        LOGE("[Native-TEE] %s: empty data%s", prefix, suffix);
        return;
    }

    // Calculate how many bytes we can print in one line (max 32 bytes per line)
    int max_per_line = 32;
    int lines = (len + max_per_line - 1) / max_per_line;

    for (int line = 0; line < lines; line++) {
        int start = line * max_per_line;
        int end = (start + max_per_line < len) ? start + max_per_line : len;

        char hex_buf[256] = {0};
        int pos = 0;

        // Print line header
        pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, "[Native-TEE] %s[%d-%d]: ", prefix,
                        start, end - 1);

        // Print hex bytes
        for (int i = start; i < end; i++) {
            pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, "0x%02x", data[i]);
            if (i < end - 1) {
                pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, ", ");
            }
        }

        if (suffix) {
            pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, "%s", suffix);
        }

        LOGE("%s", hex_buf);
    }
}

// Parse ASN.1 tag and length, return the data pointer and length
// This mimics BouncyCastle's readObject() approach
static int parse_asn1_object(const unsigned char *data, int len, int *offset,
                             const unsigned char **object_data, int *object_len) {
    if (*offset >= len) {
        LOGE("[Native-TEE] parse_asn1_object: EOF reached, offset=%d, len=%d", *offset, len);
        return -1;  // EOF
    }

    // Read tag
    int tag = data[*offset];
    (*offset)++;

    LOGE("[Native-TEE] parse_asn1_object: tag=0x%02x, offset=%d", tag, *offset);

    // Read tag number (simplified - assume no high tag numbers)
    int tag_no = tag & 0x1f;
    LOGE("[Native-TEE] parse_asn1_object: tag_no=%d", tag_no);

    // Read length
    if (*offset >= len) {
        LOGE("[Native-TEE] parse_asn1_object: EOF when reading length");
        return -1;
    }

    int length = data[*offset];
    (*offset)++;

    LOGE("[Native-TEE] parse_asn1_object: length byte=0x%02x", length);

    // Short form length
    if ((length >> 7) == 0) {
        LOGE("[Native-TEE] parse_asn1_object: short form length=%d", length);
    } else if (length == 0x80) {
        LOGE("[Native-TEE] parse_asn1_object: indefinite length not supported");
        return -1;
    } else {
        // Long form length
        int octets_count = length & 0x7F;
        length = 0;

        LOGE("[Native-TEE] parse_asn1_object: long form, octets_count=%d", octets_count);

        for (int i = 0; i < octets_count; i++) {
            if (*offset >= len) {
                LOGE("[Native-TEE] parse_asn1_object: EOF reading length octets");
                return -1;
            }

            if ((length >> 23) != 0) {
                LOGE("[Native-TEE] parse_asn1_object: length too large");
                return -1;
            }

            int octet = data[*offset];
            (*offset)++;

            length = (length << 8) + octet;
            LOGE("[Native-TEE] parse_asn1_object: octet[%d]=0x%02x, length=%d", i, octet, length);
        }
    }

    LOGE("[Native-TEE] parse_asn1_object: final length=%d", length);

    // Check if we have enough data
    if (*offset + length > len) {
        LOGE("[Native-TEE] parse_asn1_object: not enough data, need %d, have %d",
             length, len - *offset);
        return -1;
    }

    // Set the object data pointer and length
    *object_data = data + *offset;
    *object_len = length;

    // Advance offset past the object data
    *offset += length;

    LOGE("[Native-TEE] parse_asn1_object: success, tag=0x%02x, length=%d, constructed=%d",
         tag, length, (tag & 0x20) != 0);

    return tag;
}

// Parse ASN.1 tag number (based on readTagNumber in ASN1InputStream.java)
static int parse_asn1_tag_number(const unsigned char *data, int len, int *offset, int tag) {
    int tag_no = tag & 0x1f;

    // Handle high tag number
    if (tag_no == 0x1f) {
        if (*offset >= len) {
            return -1;  // EOF
        }

        int b = data[*offset];
        (*offset)++;

        if (b < 31) {
            return -1;  // Invalid high tag number
        }

        tag_no = b & 0x7f;

        // Check for invalid high tag number
        if (tag_no == 0) {
            return -1;
        }

        while ((b & 0x80) != 0) {
            if ((tag_no >> 24) != 0) {
                return -1;  // Tag number too large
            }

            tag_no <<= 7;

            if (*offset >= len) {
                return -1;  // EOF
            }

            b = data[*offset];
            (*offset)++;

            tag_no |= (b & 0x7f);
        }
    }

    return tag_no;
}

// Parse ASN.1 length (based on readLength in ASN1InputStream.java)
static int parse_asn1_length(const unsigned char *data, int len, int *offset) {
    if (*offset >= len) {
        LOGE("[Native-TEE] parse_asn1_length: EOF reached, offset=%d, len=%d", *offset, len);
        return -1;  // EOF
    }

    int length = data[*offset];
    (*offset)++;

    LOGE("[Native-TEE] parse_asn1_length: first byte=0x%02x", length);

    // Short form length (bit 7 is 0)
    if ((length >> 7) == 0) {
        LOGE("[Native-TEE] parse_asn1_length: short form length=%d", length);
        return length;
    }

    // Indefinite length
    if (length == 0x80) {
        LOGE("[Native-TEE] parse_asn1_length: indefinite length");
        return -1;
    }

    // Invalid length
    if (length < 0) {
        LOGE("[Native-TEE] parse_asn1_length: EOF found when length expected");
        return -1;
    }

    // Long form length
    int octets_count = length & 0x7F;
    length = 0;

    LOGE("[Native-TEE] parse_asn1_length: long form, octets_count=%d", octets_count);

    for (int i = 0; i < octets_count; i++) {
        if (*offset >= len) {
            LOGE("[Native-TEE] parse_asn1_length: EOF found reading length");
            return -1;  // EOF
        }

        if ((length >> 23) != 0) {
            LOGE("[Native-TEE] parse_asn1_length: long form definite-length more than 31 bits");
            return -1;  // Length too large
        }

        int octet = data[*offset];
        (*offset)++;

        length = (length << 8) | octet;  // 使用位或操作符，确保正确拼接
        LOGE("[Native-TEE] parse_asn1_length: octet[%d]=0x%02x, length=%d", i, octet, length);
    }

    LOGE("[Native-TEE] parse_asn1_length: final length=%d", length);
    return length;
}

// Parse ASN.1 tag and length
static int parse_asn1_tag(const unsigned char *data, int len, int *offset, asn1_object_t *obj) {
    if (*offset >= len) {
        LOGE("[Native-TEE] parse_asn1_tag: EOF reached, offset=%d, len=%d", *offset, len);
        return -1;  // EOF
    }

    int tag = data[*offset];
    (*offset)++;

    LOGE("[Native-TEE] parse_asn1_tag: tag=0x%02x, offset=%d", tag, *offset);

    obj->tag = tag;
    obj->tag_class = tag & 0xE0; // ASN1_FLAGS
    obj->is_constructed = (tag & 0x20) != 0;

    int tag_no = parse_asn1_tag_number(data, len, offset, tag);
    if (tag_no < 0) {
        LOGE("[Native-TEE] parse_asn1_tag: Failed to parse tag number");
        return -1;
    }

    LOGE("[Native-TEE] parse_asn1_tag: tag_no=%d", tag_no);

    int length = parse_asn1_length(data, len, offset);
    if (length < 0) {
        LOGE("[Native-TEE] parse_asn1_tag: Failed to parse length");
        return -1;
    }

    LOGE("[Native-TEE] parse_asn1_tag: length=%d", length);

    obj->length = length;
    obj->data = data + *offset;  // Point to the actual data content
    obj->data_len = length;

    // Advance offset past the object data
    *offset += length;

    LOGE("[Native-TEE] parse_asn1_tag: success, tag=0x%02x, length=%d, constructed=%d, data_len=%d, new_offset=%d",
         obj->tag, obj->length, obj->is_constructed, obj->data_len, *offset);

    return 0;
}

// Parse X.509 certificate using manual ASN.1 parsing (for debugging OID comparison)
static int parse_x509_certificate(const unsigned char *data, int len, tee_info_t *tee_info) {
    int offset = 0;
    const unsigned char *cert_data;
    int cert_len;

    LOGE("[Native-TEE] Starting X.509 certificate parsing, total length: %d", len);

    // Parse certificate sequence
    int tag = parse_asn1_object(data, len, &offset, &cert_data, &cert_len);
    if (tag != ASN1_CONSTRUCTED_SEQUENCE) {
        LOGE("[Native-TEE] Expected certificate sequence, got tag: 0x%02x", tag);
        return -1;
    }

    LOGE("[Native-TEE] Certificate sequence length: %d", cert_len);

    // Parse certificate body (the first element in the sequence)
    const unsigned char *body_data;
    int body_len;
    int body_offset = 0;

    tag = parse_asn1_object(cert_data, cert_len, &body_offset, &body_data, &body_len);
    if (tag != ASN1_CONSTRUCTED_SEQUENCE) {
        LOGE("[Native-TEE] Expected certificate body sequence, got tag: 0x%02x", tag);
        return -1;
    }

    LOGE("[Native-TEE] Certificate body sequence length: %d", body_len);

    // Skip version, serial number, signature algorithm, issuer, validity, subject
    // We need to parse 6 elements before reaching subject public key info
    for (int i = 0; i < 6; i++) {
        const unsigned char *element_data;
        int element_len;
        int element_offset = 0;

        tag = parse_asn1_object(body_data, body_len, &element_offset, &element_data, &element_len);
        if (tag < 0) {
            LOGE("[Native-TEE] Failed to parse element %d", i);
            return -1;
        }

        LOGE("[Native-TEE] Skipped element %d: tag=0x%02x, length=%d", i, tag, element_len);

        // Move to next element
        body_data += element_offset;
        body_len -= element_offset;
    }

    // Parse subject public key info
    const unsigned char *spki_data;
    int spki_len;
    int spki_offset = 0;

    tag = parse_asn1_object(body_data, body_len, &spki_offset, &spki_data, &spki_len);
    if (tag != ASN1_CONSTRUCTED_SEQUENCE) {
        LOGE("[Native-TEE] Expected subject public key info sequence, got tag: 0x%02x", tag);
        return -1;
    }

    LOGE("[Native-TEE] Subject public key info: tag=0x%02x, length=%d", tag, spki_len);

    // Move past subject public key info
    body_data += spki_offset;
    body_len -= spki_offset;

    // Check for extensions (optional)
    if (body_len > 0) {
        const unsigned char *ext_data;
        int ext_len;
        int ext_offset = 0;

        tag = parse_asn1_object(body_data, body_len, &ext_offset, &ext_data, &ext_len);
        LOGE("[Native-TEE] Found extension area: tag=0x%02x, length=%d", tag, ext_len);

        // Extensions can be either SEQUENCE (0x30) or context-specific tag 3 (0xa3)
        if (tag == ASN1_CONSTRUCTED_SEQUENCE || tag == 0xa3) {
            LOGE("[Native-TEE] Found extensions sequence, length: %d", ext_len);

            // Look for attestation extension using manual parsing
            unsigned char *extension_data = NULL;
            int extension_len = 0;

            if (get_extension_by_oid_manual(ext_data, ext_len, TEE_ATTESTATION_OID, &extension_data,
                                            &extension_len) == 0) {
                LOGE("[Native-TEE] Found attestation extension");
                tee_info->has_attestation_extension = 1;

                // Print extension value for comparison with Java
                print_hex_data("ExtensionValue", extension_data, extension_len, NULL);

                // Parse attestation extension - extension_data already contains the SEQUENCE data
                if (parse_attestation_record(extension_data, extension_len, tee_info) != 0) {
                    LOGE("[Native-TEE] Failed to parse attestation extension");
                    return -1;
                }
            } else {
                LOGE("[Native-TEE] Attestation extension not found");
                tee_info->has_attestation_extension = 0;
            }
        } else {
            LOGE("[Native-TEE] No extensions found, tag: 0x%02x", tag);
            tee_info->has_attestation_extension = 0;
        }
    } else {
        LOGE("[Native-TEE] No extensions found in certificate");
        tee_info->has_attestation_extension = 0;
    }

    LOGE("[Native-TEE] X.509 certificate parsing completed");
    return 0;
}

// Parse attestation record
static int parse_attestation_record(const unsigned char *data, int len, tee_info_t *tee_info) {
    LOGE("[Native-TEE] Parsing attestation record, length: %d", len);

    int offset = 0;
    asn1_object_t obj;

    // Parse attestation sequence
    if (parse_asn1_tag(data, len, &offset, &obj) != 0) {
        LOGE("[Native-TEE] Failed to parse attestation sequence");
        return -1;
    }

    if ((obj.tag & 0x1F) != ASN1_SEQUENCE || !obj.is_constructed) {
        LOGE("[Native-TEE] Expected attestation sequence, got tag=0x%02x", obj.tag);
        return -1;
    }

    LOGE("[Native-TEE] Attestation sequence length: %d", obj.length);

    // Parse attestation elements
    int att_offset = 0;
    asn1_object_t att_obj;

    // Element 0: attestation version (skip)
    if (parse_asn1_tag(obj.data, obj.data_len, &att_offset, &att_obj) != 0) {
        LOGE("[Native-TEE] Failed to parse attestation version");
        return -1;
    }

    // Element 1: attestation security level
    if (parse_asn1_tag(obj.data, obj.data_len, &att_offset, &att_obj) != 0) {
        LOGE("[Native-TEE] Failed to parse security level");
        return -1;
    }

    if ((att_obj.tag == ASN1_INTEGER || att_obj.tag == ASN1_ENUMERATED) && att_obj.data_len > 0) {
        tee_info->security_level = att_obj.data[0];
        LOGE("[Native-TEE] Security level: %d", tee_info->security_level);
    }

    // Skip elements 2-5 (keymaster version, attestation challenge, software enforced, tee enforced)
    for (int i = 2; i <= 5; i++) {
        if (parse_asn1_tag(obj.data, obj.data_len, &att_offset, &att_obj) != 0) {
            LOGE("[Native-TEE] Failed to skip element %d", i);
            return -1;
        }
    }

    // Element 6: software enforced authorization list
    if (parse_asn1_tag(obj.data, obj.data_len, &att_offset, &att_obj) != 0) {
        LOGE("[Native-TEE] Failed to parse software enforced list");
        return -1;
    }

    if ((att_obj.tag & 0x1F) == ASN1_SEQUENCE && att_obj.is_constructed) {
        LOGE("[Native-TEE] Software enforced list length: %d", att_obj.length);
        find_root_of_trust_in_authorization_list(att_obj.data, att_obj.data_len,
                                                 &tee_info->root_of_trust);
    }

    // Element 7: tee enforced authorization list
    if (parse_asn1_tag(obj.data, obj.data_len, &att_offset, &att_obj) != 0) {
        LOGE("[Native-TEE] Failed to parse tee enforced list");
        return -1;
    }

    if ((att_obj.tag & 0x1F) == ASN1_SEQUENCE && att_obj.is_constructed) {
        LOGE("[Native-TEE] TEE enforced list length: %d", att_obj.length);
        // If root of trust not found in software enforced, try tee enforced
        if (!tee_info->root_of_trust.valid) {
            find_root_of_trust_in_authorization_list(att_obj.data, att_obj.data_len,
                                                     &tee_info->root_of_trust);
        }
    }

    return 0;
}

// Find root of trust in authorization list
static int find_root_of_trust_in_authorization_list(const unsigned char *data, int len,
                                                    root_of_trust_t *root_of_trust) {
    LOGE("[Native-TEE] Searching for root of trust in authorization list, length: %d", len);

    int offset = 0;
    asn1_object_t obj;

    while (offset < len) {
        if (parse_asn1_tag(data, len, &offset, &obj) != 0) {
            break;
        }

        // Check if this is a tagged object with root of trust tag
        // KM_TAG_ROOT_OF_TRUST = 704, which corresponds to tag 0xbf
        if ((obj.tag & ASN1_CONTEXT_SPECIFIC) && (obj.tag & 0x1F) == 0x1F) {
            // This is a high tag number, we need to parse it
            // We already have the tag_no from parse_asn1_tag, so we can use it directly
            if (obj.tag == 0xbf) {  // This is tag 704 (KM_TAG_ROOT_OF_TRUST)
                LOGE("[Native-TEE] Found root of trust tag (tag_no=704)");

                // Parse root of trust sequence
                if (parse_root_of_trust_sequence(obj.data, obj.data_len, root_of_trust) == 0) {
                    LOGE("[Native-TEE] Successfully parsed root of trust");
                    return 0;
                } else {
                    LOGE("[Native-TEE] Failed to parse root of trust sequence");
                }
            }
        }
    }

    return -1;
}

// Parse root of trust sequence
static int
parse_root_of_trust_sequence(const unsigned char *data, int len, root_of_trust_t *root_of_trust) {
    LOGE("[Native-TEE] Parsing root of trust sequence, length: %d", len);

    int offset = 0;
    asn1_object_t obj;

    // Parse root of trust sequence
    if (parse_asn1_tag(data, len, &offset, &obj) != 0) {
        LOGE("[Native-TEE] Failed to parse root of trust sequence");
        return -1;
    }

    LOGE("[Native-TEE] Root of trust data: tag=0x%02x, length=%d, constructed=%d", obj.tag,
         obj.length, obj.is_constructed);

    if ((obj.tag & 0x1F) != ASN1_SEQUENCE || !obj.is_constructed) {
        LOGE("[Native-TEE] Expected root of trust sequence, got tag=0x%02x", obj.tag);
        return -1;
    }

    LOGE("[Native-TEE] Root of trust sequence length: %d", obj.length);

    // Parse root of trust elements
    int rot_offset = 0;
    asn1_object_t rot_obj;

    // Element 0: verified boot key
    if (parse_asn1_tag(obj.data, obj.data_len, &rot_offset, &rot_obj) != 0) {
        LOGE("[Native-TEE] Failed to parse verified boot key");
        return -1;
    }

    if (rot_obj.tag == ASN1_OCTET_STRING_TAG) {
        bytes_to_hex(rot_obj.data, rot_obj.data_len, root_of_trust->verified_boot_key_hex);
        LOGE("[Native-TEE] Verified boot key: %s", root_of_trust->verified_boot_key_hex);
    }

    // Element 1: device locked
    if (parse_asn1_tag(obj.data, obj.data_len, &rot_offset, &rot_obj) != 0) {
        LOGE("[Native-TEE] Failed to parse device locked");
        return -1;
    }

    if (rot_obj.tag == ASN1_BOOLEAN && rot_obj.data_len > 0) {
        root_of_trust->device_locked = (rot_obj.data[0] != 0);
        LOGE("[Native-TEE] Device locked: %d", root_of_trust->device_locked);
    }

    // Element 2: verified boot state
    if (parse_asn1_tag(obj.data, obj.data_len, &rot_offset, &rot_obj) != 0) {
        LOGE("[Native-TEE] Failed to parse verified boot state");
        return -1;
    }

    if ((rot_obj.tag == ASN1_INTEGER || rot_obj.tag == ASN1_ENUMERATED) && rot_obj.data_len > 0) {
        root_of_trust->verified_boot_state = rot_obj.data[0];
        LOGE("[Native-TEE] Verified boot state: %d", root_of_trust->verified_boot_state);
    }

    // Element 3: verified boot hash (optional)
    if (rot_offset < obj.data_len) {
        if (parse_asn1_tag(obj.data, obj.data_len, &rot_offset, &rot_obj) == 0) {
            if (rot_obj.tag == ASN1_OCTET_STRING_TAG) {
                LOGE("[Native-TEE] Verified boot hash length: %d", rot_obj.data_len);
            }
        }
    }

    root_of_trust->valid = 1;
    return 0;
}

// Convert bytes to hex string
void bytes_to_hex(const unsigned char *data, int len, char *hex_str) {
    for (int i = 0; i < len; i++) {
        sprintf(hex_str + i * 2, "%02x", data[i]);
    }
    hex_str[len * 2] = '\0';
}

// Main function to parse TEE certificate
int parse_tee_certificate(const unsigned char *cert_data, int cert_len, tee_info_t *tee_info) {
    if (!cert_data || !tee_info || cert_len <= 0) {
        LOGE("[Native-TEE] Invalid parameters");
        return -1;
    }

    // Initialize tee_info
    memset(tee_info, 0, sizeof(tee_info_t));

    LOGE("[Native-TEE] Starting TEE certificate parsing, length: %d", cert_len);

    // Log first few bytes for debugging
    if (cert_len > 0) {
        char hex_data[65];
        bytes_to_hex(cert_data, cert_len > 32 ? 32 : cert_len, hex_data);
        LOGE("[Native-TEE] Certificate data (first 32 bytes): %s", hex_data);
    }

    // Parse X.509 certificate
    if (parse_x509_certificate(cert_data, cert_len, tee_info) != 0) {
        LOGE("[Native-TEE] Failed to parse X.509 certificate");
        return -1;
    }

    LOGI("[Native-TEE] TEE certificate parsing completed");
    LOGI("[Native-TEE] Security level: %d", tee_info->security_level);
    LOGE("[Native-TEE] Has attestation extension: %d", tee_info->has_attestation_extension);

    if (tee_info->root_of_trust.valid) {
        LOGE("[Native-TEE] Root of trust: %s, device locked: %d, verified boot state: %d",
             tee_info->root_of_trust.verified_boot_key_hex,
             tee_info->root_of_trust.device_locked,
             tee_info->root_of_trust.verified_boot_state);
    }

    return 0;
}

// Parse ASN.1 INTEGER
int parse_asn1_integer(const unsigned char *data, int len) {
    if (len <= 0) return 0;

    int result = 0;
    int sign = 1;

    // Check for negative number (first bit of first byte is 1)
    if (data[0] & 0x80) {
        sign = -1;
    }

    // Parse the integer value
    for (int i = 0; i < len; i++) {
        result = (result << 8) | data[i];
    }

    // Handle negative numbers (two's complement)
    if (sign == -1) {
        // For negative numbers, we need to handle two's complement
        // This is a simplified version - for full correctness we'd need more complex logic
        result = -(~(result - 1));
    }

    return result;
} 