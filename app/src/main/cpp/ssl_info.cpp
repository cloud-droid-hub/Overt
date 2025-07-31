//
// Created by lxz on 2025/7/27.
//

#include "zLog.h"
#include "zHttps.h"
#include "zUtil.h"
#include "zJson.h"

#include "ssl_info.h"

string get_location() {

  string location = "";

  string qq_location_url = "https://r.inews.qq.com/api/ip2city";
  string qq_location_url_fingerprint_sha256 = "DD8D04E8BCC7390E2BA8C21F6730C7595D3424B8E8C614F06B750ABE99AF16C7";

  // 创建HTTPS请求 - 使用2秒超时
  HttpsRequest request(qq_location_url, "GET", 3);

  // 执行HTTPS请求并获取响应对象
  HttpsResponse response = zHttps::getInstance()->performRequest(request);

  // 输出证书信息
  if (!response.error_message.empty()) {
            LOGW("[ssl_info] Server error_message is not empty");
    return location;
  }

  if (response.certificate.fingerprint_sha256 != qq_location_url_fingerprint_sha256) {
            LOGI("[ssl_info] Server Certificate Fingerprint Local : %s", qq_location_url_fingerprint_sha256.c_str());
            LOGD("[ssl_info] Server Certificate Fingerprint Remote: %s", response.certificate.fingerprint_sha256.c_str());
    return location;
  }

          LOGI("[ssl_info] get_time_info: pinduoduo_time: %s", response.body.c_str());

  zJson json(response.body);
  // 检查解析是否成功
  if (json.isError()) {
            LOGW("[ssl_info] Failed to parse JSON response");
    return location;
  }

  string country = json.getString("country", "");

  string province = json.getString("province", "");

  string city = json.getString("city", "");

  if(province == city){
      location = country + province;
  }else{
      location = country + province + city;
  }

          LOGI("[ssl_info] get_location: %s", location.c_str());

  return location ;
}

map<string, map<string, string>> get_ssl_info(){

    map<string, map<string, string>> info;

    map<string, string> url_info{
        {"https://www.baidu.com", "0D822C9A905AEFE98F3712C0E02630EE95332C455FE7745DF08DBC79F4B0A149"},
        {"https://www.jd.com", "109CC20D1518DC00F3CEEE91A8AE4AF45E878C9556E611A1DC90C301366A63C2"},
        {"https://www.taobao.com", "3D4949784246FFF7529B6B82DF7E544BF9BAD834141D2167634E5B62A1D885B5"},
    };

    for (auto& item : url_info) {
        LOGI("[ssl_info] === Testing URL: %s ===", item.first.c_str());

        // 创建HTTPS请求 - 使用2秒超时
        HttpsRequest request(item.first, "GET", 3);

        // 执行HTTPS请求并获取响应对象
        HttpsResponse response = zHttps::getInstance()->performRequest(request);

        // 输出证书信息
        if (!response.error_message.empty()) {
            LOGW("[ssl_info] Server error_message is not empty");
            info[item.first]["risk"] = "error";
            info[item.first]["explain"] = response.error_message;
            continue;
        }
        if (response.certificate.fingerprint_sha256 != item.second) {
            LOGI("[ssl_info] Server Certificate Fingerprint Local : %s", item.second.c_str());
            LOGD("[ssl_info] Server Certificate Fingerprint Remote: %s", response.certificate.fingerprint_sha256.c_str());
            info[item.first]["risk"] = "error";
            info[item.first]["explain"] = "Certificate Fingerprint is wrong " + response.certificate.fingerprint_sha256;
            continue;
        }
    }

    string location = get_location();
    if(location.empty()){
        LOGW("[ssl_info] get_location failed");
        info["location"]["risk"] = "error";
        info["location"]["explain"] = "get_location failed";
    }else if(string_start_with(location.c_str(), "中国")){
        LOGI("[ssl_info] get_location succeed");
        info["location"]["risk"] = "safe";
        info["location"]["explain"] = location;
    }else{
        LOGW("[ssl_info] get_location succeed but error");
        info["location"]["risk"] = "error";
        info["location"]["explain"] = location;
    }

    return info;
}