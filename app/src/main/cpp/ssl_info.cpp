//
// Created by lxz on 2025/7/27.
//



#include "zLog.h"
#include "zHttps.h"

#include "ssl_info.h"
#include "zUtil.h"

map<string, map<string, string>> get_ssl_info(){

    map<string, map<string, string>> info;

    map<string, string> url_info{
        {"https://www.baidu.com", "0D822C9A905AEFE98F3712C0E02630EE95332C455FE7745DF08DBC79F4B0A149"},
        {"https://www.jd.com", "109CC20D1518DC00F3CEEE91A8AE4AF45E878C9556E611A1DC90C301366A63C2"},
        {"https://www.taobao.com", "3D4949784246FFF7529B6B82DF7E544BF9BAD834141D2167634E5B62A1D885B5"},
    };

    for (auto& item : url_info) {
        LOGI("=== Testing URL: %s ===", item.first.c_str());

        // 创建HTTPS请求 - 使用2秒超时
        HttpsRequest request(item.first, "GET", 3);

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
    }

    return info;
}