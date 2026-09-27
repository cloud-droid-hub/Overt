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
 * 获取SSL信息(查杀分离 — 采集端)
 * 采集每个URL的HTTPS证书观察指纹/错误信息 与 地理位置(全量原始数据)，不做风险判定；
 * 证书指纹期望值比对与"中国"地区判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：
 *   {URL -> {value: "证书指纹", error: "错误信息(空串=无错误)"}} + {"location" -> {value: 位置}}
 */
map<string, map<string, string>> get_ssl_info() {

    map<string, map<string, string>> info;

    // 采集范围：需要探测证书的目标URL(仅WHERE to look，不是风险判定)
    map<string, string> url_info{
            {"https://www.baidu.com",  "0D822C9A905AEFE98F3712C0E02630EE95332C455FE7745DF08DBC79F4B0A149"},
    };

    // 检测每个URL的SSL证书指纹，全量上报(不做比对)
    for (auto &item: url_info) {
        LOGI("=== Testing URL: %s ===", item.first.c_str());

        zHttps https_client(5);
        HttpsRequest request(item.first, "GET", 3);
        HttpsResponse response = https_client.performRequest(request);

        info[item.first]["value"] = response.certificate.fingerprint_sha256;
        info[item.first]["error"] = response.error_message;
    }

    // 采集地理位置(原始数据)
    string location = get_location();
    info["location"]["value"] = location;

    LOGI("ssl_info raw count=%zu", info.size());
    return info;
}