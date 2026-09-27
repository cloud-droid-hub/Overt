//
// Created by lxz on 2025/7/27.
//

#include "zLog.h"
#include "zHttps.h"
#include "zJson.h"
#include "zSslInfo.h"

/**
 * 解析地理位置(查杀分离 — 采集端)
 * 从腾讯新闻 ip2city 响应的 body 中解析出地理位置(国家+省份+城市)。
 * 只负责解析,不发起请求、不做指纹校验(指纹采集由 get_ssl_info 的 urls[] 统一负责)。
 * @param body 腾讯新闻 ip2city 的响应 body
 * @return 地理位置字符串，格式：国家+省份+城市
 */
string get_location(const string& body) {

    string location = "";

    try {
        zJson json = zJson::parse(body.c_str());
        string country = json.value("country", "");
        string province = json.value("province", "");
        string city = json.value("city", "");
        if (province == city) {
            location = country + province;
        } else {
            location = country + province + city;
        }
        LOGI("get_location: %s", location.c_str());
    } catch (zJson::parse_error &e) {
        LOGE("zJson::parse_error:%s", e.what());
    }

    return location;
}

/**
 * 获取SSL信息(查杀分离 — 采集端)
 * 采集每个URL的HTTPS证书观察指纹/错误信息 与 地理位置(全量原始数据)，不做风险判定；
 * 证书指纹期望值(不硬编码，由 zengine 动态获取)比对与"中国"地区判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：
 *   {URL -> {value: "证书指纹", error: "错误信息(空串=无错误)"}} + {"location" -> {value: 位置}}
 */
map<string, map<string, string>> get_ssl_info() {

    map<string, map<string, string>> info;

    // 采集范围：需要探测证书的目标URL(仅WHERE to look；期望指纹不在采集端硬编码)
    const char* urls[] = {
            "https://www.baidu.com",
            "https://r.inews.qq.com/api/ip2city",   // 腾讯新闻 ip2city(地理位置)
    };

    // 检测每个URL的SSL证书指纹，全量上报(不做比对)
    for (const char* url : urls) {
        LOGI("=== Testing URL: %s ===", url);

        zHttps https_client(5);
        HttpsRequest request(url, "GET", 3);
        HttpsResponse response = https_client.performRequest(request);

        info[url]["value"] = response.certificate.fingerprint_sha256;
        info[url]["error"] = response.error_message;

        // qq ip2city: 额外解析 location(原始数据,地区判定交 zengine)
        if (strcmp(url, "https://r.inews.qq.com/api/ip2city") == 0) {
            string location = get_location(response.body);
            info["location"]["value"] = location;
        }
    }

    LOGI("ssl_info raw count=%zu", info.size());
    return info;
}