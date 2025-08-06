//
// Created by lxz on 2025/7/17.
// TEE Certificate Parser Header - C implementation based on BouncyCastle Java library
//

#ifndef TEE_CERT_PARSER_H
#define TEE_CERT_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

// Structure to hold RootOfTrust information
typedef struct {
    char verified_boot_key_hex[65];  // 32 bytes = 64 hex chars + null terminator
    char verified_boot_hash_hex[65]; // 32 bytes = 64 hex chars + null terminator
    unsigned char verified_boot_key[32];
    unsigned char verified_boot_hash[32];
    int verified_boot_key_len;
    int verified_boot_hash_len;
    int device_locked;
    int verified_boot_state;
    int valid;
} root_of_trust_t;

// Structure to hold authorization list
typedef struct {
    root_of_trust_t root_of_trust;
} authorization_list_t;

// Structure to hold TEE info
typedef struct {
    int security_level;
    root_of_trust_t root_of_trust;
    authorization_list_t software_enforced;
    authorization_list_t tee_enforced;
    int has_attestation_extension;
} tee_info_t;

// Main function to parse TEE certificate
// Returns 0 on success, -1 on failure
int parse_tee_certificate(const unsigned char* cert_data, int cert_len, tee_info_t* tee_info);

// Get security level from parsed TEE info
// Returns security level (0-3) or -1 if not available
int get_tee_security_level(const tee_info_t* tee_info);

// Get RootOfTrust information
// Returns 0 on success, -1 if RootOfTrust not available
int get_tee_root_of_trust(const tee_info_t* tee_info, root_of_trust_t* root_of_trust);

// Check if device is locked and verified boot state
// Returns 0 on success, -1 if information not available
int check_tee_security_status(const tee_info_t* tee_info, int* device_locked, int* verified_boot_state);

// Check if attestation extension is present
// Returns 1 if present, 0 if not present
int has_tee_attestation_extension(const tee_info_t* tee_info);

// Utility functions
int parse_asn1_integer(const unsigned char* data, int len);
void bytes_to_hex(const unsigned char* data, int len, char* hex_str);
int compare_oid(const unsigned char* data, int len, const char* oid);

#ifdef __cplusplus
}
#endif

#endif // TEE_CERT_PARSER_H 