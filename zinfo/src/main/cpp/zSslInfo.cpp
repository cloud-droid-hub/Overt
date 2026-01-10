//
// Created by lxz on 2025/7/27.
//

#include "zLog.h"
#include "zHttps.h"
#include "zJson.h"
#include "zSslInfo.h"

/**
 * 获取地理位置信息
 * 通过HTTPS请求获取设备的地理位置信息
 * 使用腾讯新闻API获取IP地址对应的地理位置
 * @return 地理位置字符串，格式：国家+省份+城市
 */
string get_location() {

    string location = "";

    string qq_location_url = "https://r.inews.qq.com/api/ip2city";
    string qq_location_url_fingerprint_sha256 = "A58095F1C26CA01A5AAC2666DCAA66182BE423BE47973BBD1F3CCFF9ACA59D14";

    zHttps https_client(5);
    HttpsRequest request(qq_location_url, "GET", 3);
    HttpsResponse response = https_client.performRequest(request);

    // 输出证书信息
    if (!response.error_message.empty()) {
        LOGW("Server error_message is not empty");
        return location;
    }

    if (response.certificate.fingerprint_sha256 != qq_location_url_fingerprint_sha256) {
        LOGI("Server Certificate Fingerprint Local : %s", qq_location_url_fingerprint_sha256.c_str());
        LOGI("Server Certificate Fingerprint Remote: %s", response.certificate.fingerprint_sha256.c_str());
        return location;
    }

    LOGI("get_time_info: pinduoduo_time: %s", response.body.c_str());

    try {
        zJson json = zJson::parse(response.body.c_str());

        string country = json.value("country", "");

        string province = json.value("province", "");

        string city = json.value("city", "");

        if (province == city) {
            location = country + province;
        } else {
            location = country + province + city;
        }

        LOGI("get_location: %s", location.c_str());

        return location;
    } catch (zJson::parse_error &e) {
        LOGE("zJson::parse_error:%s", e.what());
        return location;
    }
}

/**
 * 获取SSL信息的主函数
 * 检测HTTPS连接的SSL证书指纹，验证网络通信的安全性
 * 通过对比预定义的证书指纹，检测是否存在中间人攻击或证书伪造
 * @return 包含检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_ssl_info() {

    map<string, map<string, string>> info;

    // 定义需要检测的URL和对应的证书指纹
    map<string, string> url_info{
            {"https://www.baidu.com",  "0D822C9A905AEFE98F3712C0E02630EE95332C455FE7745DF08DBC79F4B0A149"},
    };

    // 检测每个URL的SSL证书指纹
    for (auto &item: url_info) {
        LOGI("=== Testing URL: %s ===", item.first.c_str());

        zHttps https_client(5);
        HttpsRequest request(item.first, "GET", 3);
        HttpsResponse response = https_client.performRequest(request);

        // 输出证书信息
        if (!response.error_message.empty()) {
            LOGW("Server error_message is not empty");
            info[item.first]["risk"] = "error";
            info[item.first]["explain"] = response.error_message;
            continue;
        }
        if (response.certificate.fingerprint_sha256 != item.second) {
            LOGI("Server Url : %s", item.first.c_str());
            LOGI("Server Certificate Fingerprint Local : %s", item.second.c_str());
            LOGD("Server Certificate Fingerprint Remote: %s", response.certificate.fingerprint_sha256.c_str());
            info[item.first]["risk"] = "error";
            info[item.first]["explain"] = "Certificate Fingerprint is wrong " + response.certificate.fingerprint_sha256;
            continue;
        }
        LOGI("=== Testing2 URL: %s ===", item.first.c_str());
    }

    // 检测地理位置信息
    string location = get_location();
    if (location.empty()) {
        LOGW("get_location failed");
        info["location"]["risk"] = "error";
        info["location"]["explain"] = "get_location failed";
    } else if (string_start_with(location.c_str(), "中国")) {
        LOGI("get_location succeed");
        info["location"]["risk"] = "safe";
        info["location"]["explain"] = location;
    } else {
        LOGW("get_location succeed but error");
        info["location"]["risk"] = "error";
        info["location"]["explain"] = location;
    }

    return info;
}