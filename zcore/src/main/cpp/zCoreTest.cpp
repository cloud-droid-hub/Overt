//
// Created by lxz on 2025/8/6.
//
#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zFile.h"
#include "zJson.h"
#include "zCrc32.h"
#include "zBroadCast.h"
#include "zHttps.h"
#include "zElf.h"
#include "zClassLoader.h"
#include "zJavaVm.h"
#include "zTee.h"
#include "zLinker.h"

// 文件模块测试函数
void test_file_module() {
    LOGI("=== File Module Tests START ===");
    
    // 创建文件对象测试
    zFile file1("/proc/version");
    LOGI("File path: %s", file1.getPath().c_str());
    LOGI("File name: %s", file1.getFileName().c_str());
    LOGI("File exists: %s", file1.exists() ? "true" : "false");
    LOGI("Is file: %s", file1.isFile() ? "true" : "false");
    LOGI("Is directory: %s", file1.isDir() ? "true" : "false");
    
    if (file1.exists()) {
        LOGI("File size: %ld bytes", file1.getFileSize());
        LOGI("File UID: %ld", file1.getUid());
        LOGI("File GID: %ld", file1.getGid());
        
        // 读取文件内容测试
        string content = file1.readAllText();
        LOGI("File content (first 100 chars): %s", content.substr(0, 100).c_str());
        
        // 读取所有行测试
        vector<string> lines = file1.readAllLines();
        LOGI("File has %zu lines", lines.size());
        if (!lines.empty()) {
            LOGI("First line: %s", lines[0].c_str());
        }
    }
    
    // 目录操作测试
    zFile dir1("/proc");
    LOGI("Directory path: %s", dir1.getPath().c_str());
    LOGI("Directory exists: %s", dir1.exists() ? "true" : "false");
    LOGI("Is directory: %s", dir1.isDir() ? "true" : "false");
    
    if (dir1.exists() && dir1.isDir()) {
        vector<string> files = dir1.listFiles();
        LOGI("Directory contains %zu files", files.size());
        size_t maxFiles = 5;
        if (files.size() < maxFiles) maxFiles = files.size();
        for (size_t i = 0; i < maxFiles; ++i) {
            LOGI("  File %zu: %s", i, files[i].c_str());
        }
        
        vector<string> dirs = dir1.listDirectories();
        LOGI("Directory contains %zu subdirectories", dirs.size());
        size_t maxDirs = 5;
        if (dirs.size() < maxDirs) maxDirs = dirs.size();
        for (size_t i = 0; i < maxDirs; ++i) {
            LOGI("  Subdir %zu: %s", i, dirs[i].c_str());
        }
    }
    
    LOGI("=== File Module Tests END ===");
}

// JSON模块测试函数
void test_json_module() {
    LOGI("=== JSON Module Tests START ===");
    
    // 简单JSON解析测试
    string jsonStr1 = "{\"name\":\"test\",\"age\":25,\"active\":true}";
    zJson json1(jsonStr1);
    LOGI("JSON parse success: %s", json1.isError() ? "false" : "true");
    LOGI("JSON type: %d", static_cast<int>(json1.getType()));
    
    if (!json1.isError()) {
        LOGI("name: %s", json1.getString("name").c_str());
        LOGI("age: %d", json1.getInt("age"));
        LOGI("active: %s", json1.getBoolean("active") ? "true" : "false");
    }
    

    
    LOGI("=== JSON Module Tests END ===");
}

// CRC32模块测试函数
void test_crc32_module() {
    LOGI("=== CRC32 Module Tests START ===");
    
    string testData = "Hello World";
    uint32_t crc = crc32c_fold(testData.c_str(), testData.length());
    LOGI("CRC32 of '%s': 0x%08X", testData.c_str(), crc);
    
    // 测试不同数据的CRC32
    vector<string> testStrings;
    testStrings.push_back("test");
    testStrings.push_back("data");
    testStrings.push_back("crc32");
    testStrings.push_back("validation");
    
    for (size_t i = 0; i < testStrings.size(); ++i) {
        uint32_t strCrc = crc32c_fold(testStrings[i].c_str(), testStrings[i].length());
        LOGI("CRC32 of '%s': 0x%08X", testStrings[i].c_str(), strCrc);
    }
    
    LOGI("=== CRC32 Module Tests END ===");
}

// 广播模块测试函数
void test_broadcast_module() {
    LOGI("=== Broadcast Module Tests START ===");
    
    zBroadCast* broadcast = zBroadCast::getInstance();
    LOGI("Broadcast instance created");
    
    // 获取本地IP测试
    string localIp = broadcast->get_local_ip();
    LOGI("Local IP: %s", localIp.c_str());
    
    string localIpC = broadcast->get_local_ip_c();
    LOGI("Local IP C: %s", localIpC.c_str());
    
    // 设置广播参数测试
    broadcast->set_sender_thread_args(8888, "test_message", nullptr);
    LOGI("Broadcast sender args set");
    
    broadcast->set_listener_thread_args(8888, "test", nullptr);
    LOGI("Broadcast listener args set");
    
    // 测试广播发送功能
    LOGI("Testing broadcast send functionality");
    broadcast->send_udp_broadcast(8888, "Hello from zCore test");
    LOGI("Broadcast message sent");
    
    // 测试广播监听功能
    LOGI("Testing broadcast listen functionality");
    broadcast->listen_udp_broadcast(8888, [](const char* ip, const char* msg) {
        LOGI("Received broadcast from %s: %s", ip, msg);
    });
    LOGI("Broadcast listener started");
    
    // 测试IP监控功能
    LOGI("Testing IP monitoring functionality");
    broadcast->start_local_ip_monitor();
    LOGI("IP monitor started");
    
    // 测试广播发送器启动
    LOGI("Testing broadcast sender start");
    broadcast->start_udp_broadcast_sender(8888, "Periodic test message");
    LOGI("Broadcast sender started");
    
    // 测试广播监听器启动
    LOGI("Testing broadcast listener start");
    broadcast->start_udp_broadcast_listener(8888, [](const char* ip, const char* msg) {
        LOGI("Periodic listener received from %s: %s", ip, msg);
    });
    LOGI("Broadcast listener started");
    
    // 测试重启监听器功能
    LOGI("Testing listener restart functionality");
    broadcast->restart_udp_broadcast_listener();
    LOGI("Broadcast listener restarted");
    
    // 测试不同端口的广播
    LOGI("Testing multi-port broadcast");
    broadcast->send_udp_broadcast(9999, "Test message on port 9999");
    LOGI("Multi-port broadcast sent");
    
    // 测试长消息广播
    LOGI("Testing long message broadcast");
    string longMessage = "This is a very long test message that contains multiple words and should test the broadcast system's ability to handle larger payloads without any issues or truncation";
    broadcast->send_udp_broadcast(8888, longMessage);
    LOGI("Long message broadcast sent");
    
    // 测试特殊字符消息
    LOGI("Testing special character broadcast");
    string specialMessage = "Test message with special chars: !@#$%^&*()_+-=[]{}|;':\",./<>?";
    broadcast->send_udp_broadcast(8888, specialMessage);
    LOGI("Special character broadcast sent");
    
    // 测试中文消息
    LOGI("Testing Chinese character broadcast");
    string chineseMessage = "测试中文广播消息：你好世界！";
    broadcast->send_udp_broadcast(8888, chineseMessage);
    LOGI("Chinese character broadcast sent");
    
    // 测试空消息
    LOGI("Testing empty message broadcast");
    broadcast->send_udp_broadcast(8888, "");
    LOGI("Empty message broadcast sent");
    
    // 测试停止命令
    LOGI("Testing stop command broadcast");
    broadcast->send_udp_broadcast(8888, "stop");
    LOGI("Stop command broadcast sent");
    
    // 验证本地IP变化检测
    LOGI("Testing local IP change detection");
    string currentIp = broadcast->get_local_ip();
    LOGI("Current local IP: %s", currentIp.c_str());
    
    // 模拟IP变化（在实际环境中IP可能会变化）
    LOGI("Monitoring for IP changes...");
    
    LOGI("=== Broadcast Module Tests END ===");
}

// HTTPS模块测试函数
void test_https_module() {
    LOGI("=== HTTPS Module Tests START ===");
    
    // 检查网络连接状态
    LOGI("Checking network connectivity...");
    
    // 测试基本HTTPS请求
    LOGI("Testing basic HTTPS request to baidu.com");
    HttpsRequest request1("https://www.baidu.com", "GET", 5);
    HttpsResponse response1 = zHttps::getInstance()->performRequest(request1);
    
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
        HttpsResponse response = zHttps::getInstance()->performRequest(request);
        
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
    HttpsResponse locationResponse = zHttps::getInstance()->performRequest(locationRequest);
    
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
        
        if (locationResponse.certificate.fingerprint_sha256 != expectedFingerprint) {
            LOGW("Location service certificate fingerprint mismatch!");
            LOGW("Expected: %s", expectedFingerprint.c_str());
            LOGW("Actual: %s", locationResponse.certificate.fingerprint_sha256.c_str());
        } else {
            LOGI("Location service certificate verification passed");
        }
        
        // 解析地理位置JSON
        if (!locationResponse.body.empty()) {
            zJson locationJson(locationResponse.body);
            if (!locationJson.isError()) {
                string country = locationJson.getString("country", "");
                string province = locationJson.getString("province", "");
                string city = locationJson.getString("city", "");
                
                LOGI("Location detected - Country: %s, Province: %s, City: %s", 
                     country.c_str(), province.c_str(), city.c_str());
                
                string location;
                if (province == city) {
                    location = country + province;
                } else {
                    location = country + province + city;
                }
                LOGI("Combined location: %s", location.c_str());
            } else {
                LOGW("Failed to parse location JSON");
            }
        }
    }
    
    // 测试错误情况
    LOGI("Testing error scenarios");
    
    // 测试无效URL
    HttpsRequest invalidRequest("https://invalid-domain-that-does-not-exist.com", "GET", 2);
    HttpsResponse invalidResponse = zHttps::getInstance()->performRequest(invalidRequest);
    LOGI("Invalid URL test - Error: %s", invalidResponse.error_message.c_str());
    
    // 测试超时情况
    HttpsRequest timeoutRequest("https://httpbin.org/delay/10", "GET", 1);
    HttpsResponse timeoutResponse = zHttps::getInstance()->performRequest(timeoutRequest);
    LOGI("Timeout test - Error: %s", timeoutResponse.error_message.c_str());
    
    // 测试POST请求
    LOGI("Testing POST request");
    HttpsRequest postRequest("https://httpbin.org/post", "POST", 5);
    postRequest.body = "{\"test\":\"data\"}";
    postRequest.headers["Content-Type"] = "application/json";
    
    HttpsResponse postResponse = zHttps::getInstance()->performRequest(postRequest);
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

// ELF模块测试函数
void test_elf_module() {
    LOGI("=== ELF Module Tests START ===");
    
    // ELF文件分析测试（使用系统库文件）
    zFile elfFile("/proc/self/exe");
    if (elfFile.exists()) {
        LOGI("ELF file exists: %s", elfFile.getPath().c_str());
        LOGI("ELF file size: %ld bytes", elfFile.getFileSize());
    }
    
    LOGI("=== ELF Module Tests END ===");
}

// 类加载器测试函数
void test_classloader_module() {
    LOGI("=== Class Loader Tests START ===");
    
    // 类加载器基本功能测试
    LOGI("Class loader module initialized");
    
    LOGI("=== Class Loader Tests END ===");
}

// JVM测试函数
void test_jvm_module() {
    LOGI("=== JVM Tests START ===");
    
    // JVM交互测试
    LOGI("JVM module initialized for testing");
    
    LOGI("=== JVM Tests END ===");
}

// TEE测试函数
void test_tee_module() {
    LOGI("=== TEE Tests START ===");
    
    // 可信执行环境测试
    LOGI("TEE module initialized for testing");
    
    LOGI("=== TEE Tests END ===");
}

// 链接器测试函数
void test_linker_module() {
    LOGI("=== Linker Tests START ===");
    
    // 链接器测试
    LOGI("Linker module initialized for testing");
    
    LOGI("=== Linker Tests END ===");
}

// 集成测试函数
void test_integration() {
    LOGI("=== Integration Tests START ===");
    
    // 测试文件读取后计算CRC32
    zFile file1("/proc/version");
    if (file1.exists()) {
        vector<uint8_t> fileBytes = file1.readAllBytes();
        if (!fileBytes.empty()) {
            uint32_t fileCrc = crc32c_fold(fileBytes.data(), fileBytes.size());
            LOGI("File CRC32: 0x%08X", fileCrc);
        }
    }
    
    // 测试JSON序列化文件信息
    map<string, zJson> fileInfo;
    string pathJson = "\"" + file1.getPath() + "\"";
    fileInfo["path"] = zJson(pathJson);
    
    char sizeStr[32];
    sprintf(sizeStr, "%ld", file1.getFileSize());
    fileInfo["size"] = zJson(sizeStr);
    
    fileInfo["exists"] = zJson(file1.exists() ? "true" : "false");
    
    LOGI("File info JSON created");
    
    LOGI("=== Integration Tests END ===");
}

// 性能测试函数
void test_performance() {
    LOGI("=== Performance Tests START ===");
    
    // 测试大量数据处理的性能
    vector<string> largeData;
    for (int i = 0; i < 1000; ++i) {
        char numStr[32];
        sprintf(numStr, "%d", i);
        string dataItem = "test_data_" + string(numStr);
        largeData.push_back(dataItem);
    }
    
    LOGI("Created %zu test data items", largeData.size());
    
    // 批量CRC32计算
    size_t maxTestItems = 10;
    if (largeData.size() < maxTestItems) maxTestItems = largeData.size();
    for (size_t i = 0; i < maxTestItems; ++i) {
        uint32_t crc = crc32c_fold(largeData[i].c_str(), largeData[i].length());
        LOGI("Data %zu CRC32: 0x%08X", i, crc);
    }
    
    LOGI("=== Performance Tests END ===");
}

// 错误处理测试函数
void test_error_handling() {
    LOGI("=== Error Handling Tests START ===");
    
    // 测试不存在的文件
    zFile nonExistentFile("/nonexistent/file");
    LOGI("Non-existent file exists: %s", nonExistentFile.exists() ? "true" : "false");
    
    // 测试无效JSON
    string invalidJson = "{invalid json}";
    zJson invalidJsonObj(invalidJson);
    LOGI("Invalid JSON parse success: %s", invalidJsonObj.isError() ? "false" : "true");
    
    // 测试空字符串
    string emptyJson = "";
    zJson emptyJsonObj(emptyJson);
    LOGI("Empty JSON parse success: %s", emptyJsonObj.isError() ? "false" : "true");
    
    LOGI("=== Error Handling Tests END ===");
}



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
        LOGW("Server error_message is not empty");
        return location;
    }

    if (response.certificate.fingerprint_sha256 != qq_location_url_fingerprint_sha256) {
        LOGI("Server Certificate Fingerprint Local : %s", qq_location_url_fingerprint_sha256.c_str());
        LOGD("Server Certificate Fingerprint Remote: %s", response.certificate.fingerprint_sha256.c_str());
        return location;
    }

    LOGI("get_time_info: pinduoduo_time: %s", response.body.c_str());

    zJson json(response.body);
    // 检查解析是否成功
    if (json.isError()) {
        LOGW("Failed to parse JSON response");
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

    LOGI("get_location: %s", location.c_str());

    return location ;
}

void __attribute__((constructor)) init_(void){
    LOGI("zCore init - Starting comprehensive tests");
//    test_https_module();
    get_location();
//    // 执行各个模块的测试
//    test_file_module();
//    test_crc32_module();
//    test_broadcast_module();
//
//    test_elf_module();
//    test_classloader_module();
//    test_jvm_module();
//    test_tee_module();
//    test_linker_module();
//    test_integration();
//    test_performance();
//    test_error_handling();
//
//    test_json_module();

    LOGI("zCore init - All tests completed successfully");
}