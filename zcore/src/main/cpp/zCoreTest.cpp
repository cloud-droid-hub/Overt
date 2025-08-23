//
// Created by lxz on 2025/8/6.
// zCore 综合测试模块 - 重新设计版本
// 专注于 HTTPS 超时机制测试
//

#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zHttps.h"


// 全局测试统计
static int g_testsPassed = 0;
static int g_testsFailed = 0;
static int g_testsWarning = 0;

// 测试结果记录
void recordTestResult(bool passed, bool warning = false) {
    if (passed) {
        g_testsPassed++;
    } else {
        g_testsFailed++;
    }
    if (warning) {
        g_testsWarning++;
    }
}



// HTTPS模块测试函数
void test_https_module() {
    LOGI("=== HTTPS Module Tests START ===");

    // 检查网络连接状态
    LOGI("Checking network connectivity...");

    // 测试基本HTTPS请求
    LOGI("Testing basic HTTPS request to baidu.com");
    zHttps https_client(5);
    HttpsRequest request1("https://www.baidu.com", "GET", 5);
    HttpsResponse response1 = https_client.performRequest(request1);

    if (!response1.error_message.empty()) {
        LOGW("HTTPS request failed: %s", response1.error_message.c_str());
        LOGW("This might be due to:");
        LOGW("1. Missing network permissions in AndroidManifest.xml");
        LOGW("2. No internet connection");
        LOGW("3. Android network policy restrictions");
        LOGW("4. Running in emulator with network limitations");
    } else {
        LOGI("HTTPS request successful, status: %d", response1.status_code);
        LOGI("Response body length: %zu", response1.body.length());
        LOGI("Response headers count: %zu", response1.headers.size());
        LOGI("SSL verification passed: %s", response1.ssl_verification_passed ? "true" : "false");
        LOGI("Certificate pinning passed: %s", response1.certificate_pinning_passed ? "true" : "false");

        // 输出响应头信息
        LOGI("Response headers:");
        for (const auto& header : response1.headers) {
            LOGI("  %s: %s", header.first.c_str(), header.second.c_str());
        }

        // 输出证书信息
        if (response1.certificate.is_valid) {
            LOGI("Certificate serial: %s", response1.certificate.serial_number.c_str());
            LOGI("Certificate fingerprint: %s", response1.certificate.fingerprint_sha256.c_str());
            LOGI("Certificate subject: %s", response1.certificate.subject.c_str());
            LOGI("Certificate issuer: %s", response1.certificate.issuer.c_str());
            LOGI("Certificate valid from: %s", response1.certificate.valid_from.c_str());
            LOGI("Certificate valid to: %s", response1.certificate.valid_to.c_str());
        }

        // 输出响应体内容（前200字符）
        if (!response1.body.empty()) {
            LOGI("Response body (first 200 chars): %s", response1.body.substr(0, 200).c_str());
        } else {
            LOGW("Response body is empty");
        }
    }

    // 测试证书指纹验证
    LOGI("Testing certificate fingerprint verification");
    map<string, string> testUrls = {
            {"https://www.baidu.com", "0D822C9A905AEFE98F3712C0E02630EE95332C455FE7745DF08DBC79F4B0A149"},
            {"https://www.jd.com", "109CC20D1518DC00F3CEEE91A8AE4AF45E878C9556E611A1DC90C301366A63C2"},
            {"https://www.taobao.com", "3D4949784246FFF7529B6B82DF7E544BF9BAD834141D2167634E5B62A1D885B5"}
    };

    for (auto& item : testUrls) {
        LOGI("Testing URL: %s", item.first.c_str());
        LOGI("Expected fingerprint: %s", item.second.c_str());

        HttpsRequest request(item.first, "GET", 3);
        HttpsResponse response = https_client.performRequest(request);

        if (!response.error_message.empty()) {
            LOGW("Request failed: %s", response.error_message.c_str());
            continue;
        }

        LOGI("Request successful, status: %d", response.status_code);
        LOGI("Response body length: %zu", response.body.length());

        if (response.certificate.fingerprint_sha256 != item.second) {
            LOGW("Certificate fingerprint mismatch!");
            LOGW("Expected: %s", item.second.c_str());
            LOGW("Actual: %s", response.certificate.fingerprint_sha256.c_str());
        } else {
            LOGI("Certificate fingerprint verification passed");
        }

        LOGI("Certificate serial: %s", response.certificate.serial_number.c_str());
        LOGI("Certificate subject: %s", response.certificate.subject.c_str());
        LOGI("Certificate issuer: %s", response.certificate.issuer.c_str());
    }

    // 测试地理位置获取
    LOGI("Testing location detection");
    string locationUrl = "https://r.inews.qq.com/api/ip2city";
    string expectedFingerprint = "DD8D04E8BCC7390E2BA8C21F6730C7595D3424B8E8C614F06B750ABE99AF16C7";

    HttpsRequest locationRequest(locationUrl, "GET", 3);
    HttpsResponse locationResponse = https_client.performRequest(locationRequest);

    if (!locationResponse.error_message.empty()) {
        LOGW("Location request failed: %s", locationResponse.error_message.c_str());
    } else {
        LOGI("Location request successful");
        LOGI("Location response status: %d", locationResponse.status_code);
        LOGI("Location response body length: %zu", locationResponse.body.length());
        LOGI("Location response headers count: %zu", locationResponse.headers.size());

        // 输出响应头
        LOGI("Location response headers:");
        for (const auto& header : locationResponse.headers) {
            LOGI("  %s: %s", header.first.c_str(), header.second.c_str());
        }

        // 输出响应体
        if (!locationResponse.body.empty()) {
            LOGI("Location response body: %s", locationResponse.body.c_str());
        } else {
            LOGW("Location response body is empty");
        }
    }

    // 测试错误情况
    LOGI("Testing error scenarios");

    // 测试无效URL
    HttpsRequest invalidRequest("https://invalid-domain-that-does-not-exist.com", "GET", 2);
    HttpsResponse invalidResponse = https_client.performRequest(invalidRequest);
    LOGI("Invalid URL test - Error: %s", invalidResponse.error_message.c_str());

    // 测试超时情况
    HttpsRequest timeoutRequest("https://httpbin.org/delay/10", "GET", 1);
    HttpsResponse timeoutResponse = https_client.performRequest(timeoutRequest);
    LOGI("Timeout test - Error: %s", timeoutResponse.error_message.c_str());

    // 测试POST请求
    LOGI("Testing POST request");
    HttpsRequest postRequest("https://httpbin.org/post", "POST", 5);
    postRequest.body = "{\"test\":\"data\"}";
    postRequest.headers["Content-Type"] = "application/json";

    HttpsResponse postResponse = https_client.performRequest(postRequest);
    if (!postResponse.error_message.empty()) {
        LOGW("POST request failed: %s", postResponse.error_message.c_str());
    } else {
        LOGI("POST request successful, status: %d", postResponse.status_code);
        LOGI("POST response body length: %zu", postResponse.body.length());
        if (!postResponse.body.empty()) {
            LOGI("POST response body (first 200 chars): %s", postResponse.body.substr(0, 200).c_str());
        }
    }

    // 网络连接建议
    LOGI("Network troubleshooting suggestions:");
    LOGI("1. Ensure INTERNET permission is added to AndroidManifest.xml");
    LOGI("2. Check if device/emulator has internet connection");
    LOGI("3. Try running on a physical device instead of emulator");
    LOGI("4. Check Android network security configuration");

    LOGI("=== HTTPS Module Tests END ===");
}

// ==================== 主测试函数 ====================
void __attribute__((constructor)) init_(void) {
    LOGI("🚀 zCore 初始化 - 启动全面测试");
    

    // HTTPS超时机制测试
    test_https_module();
    
    // 输出测试结果
    LOGI("=== 测试结果汇总 ===");
    LOGI("通过: %d", g_testsPassed);
    LOGI("失败: %d", g_testsFailed);
    LOGI("警告: %d", g_testsWarning);
    LOGI("总计: %d", g_testsPassed + g_testsFailed + g_testsWarning);
    
    return;
}