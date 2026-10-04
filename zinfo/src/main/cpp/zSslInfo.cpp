//
// Created by lxz on 2025/7/27.
//

#include "zLog.h"
#include "zHttps.h"
#include "zJson.h"
#include "zSslInfo.h"

static bool valid_place(const string &text, bool required) {
    bool filled = false;
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = text[i];
        if (c < 0x20 || c == 0x7f) return false;
        if (c != ' ') filled = true;
    }
    return !required || filled;
}

/**
 * 获取地理位置信息
 * 通过HTTPS请求获取设备的地理位置信息
 * 使用腾讯新闻API获取IP地址对应的地理位置
 * @return 地理位置字符串，格式：国家+省份+城市
 */
string get_location() {

    string location = "";

    string qq_location_url = "https://r.inews.qq.com/api/ip2city";
    string loc_pin = "EA725FF9B6B1A8D8A823A6DE0C59A24496FC38E937C03EF6F5D84C66F107C421";

    zHttps https_client(5);
    HttpsRequest request(qq_location_url, "GET", 3);
    HttpsResponse response = https_client.performRequest(request);

    // 输出证书信息
    if (!response.error_message.empty() || !response.ssl_verification_passed ||
        response.status_code != 200) {
        LOGW("Location request failed");
        return location;
    }

    if (response.certificate.fingerprint_sha256 != loc_pin) {
        LOGI("Server Certificate Fingerprint Local : %s", loc_pin.c_str());
        LOGI("Server Certificate Fingerprint Remote: %s", response.certificate.fingerprint_sha256.c_str());
        return location;
    }

    LOGI("get_location response: %s", response.body.c_str());

    try {
        zJson json = zJson::parse(response.body.c_str());

        if (!json.is_object() || !json.contains("ret") ||
            !json.at("ret").is_number_integer() || json.at("ret") != 0 ||
            !json.contains("country") || !json.at("country").is_string() ||
            (json.contains("province") && !json.at("province").is_string()) ||
            (json.contains("city") && !json.at("city").is_string())) {
            return location;
        }

        string country = json.value("country", "");

        string province = json.value("province", "");

        string city = json.value("city", "");

        if (!valid_place(country, true) || !valid_place(province, false) ||
            !valid_place(city, false)) {
            return location;
        }

        if (province == city) {
            location = country + province;
        } else {
            location = country + province + city;
        }

        LOGI("get_location: %s", location.c_str());

        return location;
    } catch (const zJson::exception &e) {
        LOGE("Location JSON error:%s", e.what());
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
            {"https://www.baidu.com",  "CA5688C552685190E98B94C40E94F842EE7FDA39B08846FBD4D7E2ED7211B4F2"},
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
    } else {
        LOGI("get_location succeed");
        info["location"]["risk"] = "safe";
        info["location"]["explain"] = location;
    }

    return info;
}
