//
// Created by lxz on 2025/8/30.
//
#include <dlfcn.h>
#include <regex>

#include "zLog.h"
#include "zFile.h"
#include "zZip.h"
#include "zSha256.h"
#include "zSignatureInfo.h"

// 获取应用私有目录路径
string get_app_specific_dir_path() {

    Dl_info dlInfo;
    dladdr((void *) get_app_specific_dir_path, &dlInfo);
    LOGE("dlInfo.dli_fname %s", dlInfo.dli_fname);


    std::cmatch matchs;
    std::regex rx("(/data/app/.+?==)(?:/base.apk!)*/lib");
    bool found = std::regex_search((const char *) dlInfo.dli_fname,
                                   (const char *) dlInfo.dli_fname +
                                   strlen((char *) dlInfo.dli_fname), matchs, rx);
    if (found) {
        return matchs.str(1).c_str();
    }
    return "";
}
string memory_to_hex_string(char *ptr, size_t size) {
    string ret = "";
    for (size_t i = 0; i < size; i++) {
        char buf[3]; // 每个字节需要两个字符，再加上字符串结尾的'\0'
        sprintf(buf, "%02X", *(ptr + i)); // 格式化输出到字符数组
        ret += buf;
    }
    return ret;
}

string getSha256byBaseApk() {
    string app_specific_dir_path = get_app_specific_dir_path();
    LOGE("get_app_specific_dir_path %s", app_specific_dir_path.c_str());

    string base_apk_path = app_specific_dir_path.append("/base.apk");
    LOGE("get_base_apk_path %s", base_apk_path.c_str());

    mz_bool status = 0;
    mz_zip_archive zip_archive = {};
    size_t uncomp_size = 0;

    zFile base_apk_file(base_apk_path);
    vector<uint8_t> base_apk_bytes = base_apk_file.readAllBytes();
    status = mz_zip_reader_init_mem(&zip_archive, base_apk_bytes.data(), base_apk_bytes.size(), 0);

    if(status == 0) {
        LOGE("open zip failed %s", base_apk_path.c_str());
        return "";
    }

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        mz_bool reader_file_stat = mz_zip_reader_file_stat(&zip_archive, i, &file_stat);
        if (!reader_file_stat) {
            LOGE("mz_zip_reader_file_stat failed id %d", i);
            continue;
        }

        std::cmatch matchs;
        std::regex rx("META-INF/(.+)\\.RSA"); // 匹配 "META-INF 中的 .RSA" 文件
        bool found = std::regex_search((const char *) file_stat.m_filename,
                                       (const char *) file_stat.m_filename +
                                       strlen((char *) file_stat.m_filename), matchs, rx);
        if (!found) {
            continue;
        }

        LOGE("find file %s", file_stat.m_filename);

        char *p_file = (char *) mz_zip_reader_extract_file_to_heap(&zip_archive,
                                                                   file_stat.m_filename,
                                                                   &uncomp_size, 0);
        if(!p_file) {
            LOGE("find file %s", file_stat.m_filename);
        }

        char *rsa_addr = p_file + 0x3c;
        int rsa_size = ((*(short*)(p_file + 0x3a) << 8) & 0xff00) | ((*(short*)(p_file + 0x3a) >> 8) & 0xff);

        LOGE("rsa_addr %p", file_stat.m_filename);
        LOGE("rsa_size 0x%x", rsa_size);

        unsigned char hash[30];
        sha256((const void *) rsa_addr, rsa_size, hash);

        string sha256byBaseApk = memory_to_hex_string((char *) hash, SHA256_SIZE_BYTES);

        LOGE("RSA_hash %s", sha256byBaseApk.c_str());

        return sha256byBaseApk;
    }

    mz_zip_reader_end(&zip_archive);
    return "";

}

map<string, map<string, string>> get_signature_info(){
    LOGD("get_signature_info called");
    map<string, map<string, string>> info;
    string sha256_real = "4D8ADE7A8C33C37B774F402EF0ED88D69C6E543DC11CAC7C573077EEF2903F6F";
    string sha256_baseapk = getSha256byBaseApk();
    LOGI("sha256_baseapk:%s", sha256_baseapk.c_str());

    if(sha256_baseapk != sha256_real){
        LOGI("sha256_baseapk: %s", sha256_baseapk.c_str());
        LOGI("sha256_real: %s", sha256_real.c_str());
        info["signature"]["risk"] = "error";
        info["signature"]["explain"] = "signature is not " + sha256_real;
    }

    return info;
}