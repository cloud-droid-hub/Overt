//
// Created by lxz on 2025/7/27.
//



#include "zLog.h"
#include "zHttps.h"

#include "ssl_info.h"
#include "zUtil.h"

/* 成功：返回时间戳；失败：返回 (time_t)-1 */
time_t vore_extract_time(const char *json)
{
    if (!json) return (time_t)-1;

    const char *key = "\"time\":";
    const char *p   = strstr(json, key);
    if (!p) return (time_t)-1;

    p += strlen(key);
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;

    char *endptr = NULL;
    long long val = strtoll(p, &endptr, 10);
    if (endptr == p) return (time_t)-1;

    return (time_t)val;
}

/* 查找 key 后第一个引号之间的内容（不含引号） */
static const char* extract_quoted(const char* json, const char* key)
{
    const char* p = strstr(json, key);
    if (!p) return nullptr;
    p += strlen(key);
    while (*p && *p != '"') ++p;          /* 跳到首引号 */
    if (*p != '"') return nullptr;
    ++p;                                  /* 跳过首引号 */
    return p;                             /* 指向内容开始 */
}

/* 拼接结果： "中国" + info1   或  "海外" + info1 */
string vore_location(const char* json)
{
    if (!json) return string();

    /* 1. 判断 cnip */
    bool cnip = false;
    const char* cnip_key = "\"cnip\":";
    const char* p = strstr(json, cnip_key);
    if (p) {
        p += strlen(cnip_key);
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
        cnip = (*p == 't');               /* true 以 't' 开头 */
    }

    /* 2. 提取 info1 */
    const char* info1_start = extract_quoted(json, "\"info1\":");
    if (!info1_start) return string();
    const char* info1_end = strchr(info1_start, '"');
    if (!info1_end) return string();

    /* 构造结果串 */
    string result;
    result.reserve(8 + (info1_end - info1_start));
    result.append(cnip ? "中国" : "海外");
    result.append(info1_start, info1_end - info1_start);
    return result;
}

map<string, map<string, string>> get_ssl_info(){

    map<string, map<string, string>> info;

    map<string, string> url_info{
            {"https://www.baidu.com", "0D822C9A905AEFE98F3712C0E02630EE95332C455FE7745DF08DBC79F4B0A149"},
            {"https://api.vore.top/api/IPdata", "1F945F3BBF12BF39CE65AD84467F88950FDD30A16873C03F515B2E76D734BC77"},
    };

    for (auto& item : url_info) {
        LOGI("=== Testing URL: %s ===", item.first.c_str());

        // 创建HTTPS请求 - 使用2秒超时
        HttpsRequest request(item.first, "GET", 10);

        // 执行HTTPS请求并获取响应对象
        HttpsResponse response = zHttps::getInstance()->performRequest(request);

        // 输出证书信息
        if (!response.error_message.empty()) {
            LOGE("Server error_message is not empty");
            info[item.first]["risk"] = "error";
            info[item.first]["explain"] = response.error_message;
            continue;
        }
        if (response.certificate.fingerprint_sha256 != item.second) {
            LOGE("Server Certificate Fingerprint Local : %s", item.second.c_str());
            LOGE("Server Certificate Fingerprint Remote: %s", response.certificate.fingerprint_sha256.c_str());
            info[item.first]["risk"] = "error";
            info[item.first]["explain"] = "Certificate Fingerprint is wrong " + response.certificate.fingerprint_sha256;
            continue;
        }

        if(item.first=="https://api.vore.top/api/IPdata"){

            time_t timestamp = vore_extract_time(response.body.c_str());
            if (timestamp == -1) {
                LOGE("Failed to extract timestamp from response body");
                info[item.first]["risk"] = "error";
                info[item.first]["explain"] = "Failed to extract timestamp from response body";
                continue;
            }

            string location = vore_location(response.body.c_str());
            LOGI("Server location: %s", location.c_str());
            if(location.empty()){
                LOGE("Failed to extract location from response body");
                info[item.first]["risk"] = "error";
                info[item.first]["explain"] = "Failed to extract location from response body";
                continue;
            }

            info[item.first]["risk"] = "safe";
            info[item.first]["explain"] += "Server location is " + location + ", timestamp is " + format_timestamp(timestamp);

        }

    }

    return info;
}