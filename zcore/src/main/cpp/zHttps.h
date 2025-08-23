//
// Created by lxz on 2025/7/27.
//

#ifndef SSLCHECK_ZHTTPS_H
#define SSLCHECK_ZHTTPS_H

// mbedtls headers
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"

// 证书信息结构体
struct CertificateInfo {
    string serial_number;
    string fingerprint_sha256;
    string subject;
    string issuer;
    string valid_from;
    string valid_to;
    string public_key_type;
    size_t public_key_size = 0;
    bool is_valid = false;
    bool is_expired = false;
    bool is_future = false;
};

// HTTPS请求结构
struct HttpsRequest {
    string url;
    string method;
    string host;
    string path;
    int port;
    map<string, string> headers;
    string body;
    int timeout_seconds; // 超时时间（秒）

    HttpsRequest(const string& url, const string& method, int timeout = 10)
            : url(url), method(method), timeout_seconds(timeout) {
        parseUrl();
    }

    void parseUrl();
    string buildRequest() const;
};

// HTTPS响应结构
struct HttpsResponse {
    int status_code = 0;
    map<string, string> headers;
    string body;
    string error_message;

    // 证书验证结果
    bool ssl_verification_passed = false;
    bool certificate_pinning_passed = false;

    // 证书信息
    CertificateInfo certificate;
    CertificateInfo pinned_certificate;

    // 安全状态检查
    bool isSecure() const {
        return ssl_verification_passed && certificate_pinning_passed && status_code > 0;
    }
};

// HTTPS客户端类
class zHttps {
private:
    // 请求计时器
    struct RequestTimer {
        time_t start_time;           // 请求开始时间
        time_t connection_time;      // 连接建立时间
        time_t handshake_time;       // TLS握手时间
        time_t send_time;            // 发送请求时间
        time_t receive_time;         // 接收响应时间
        time_t total_time;           // 总耗时
        int timeout_seconds;         // 本次请求的超时时间

        RequestTimer(int timeout = 10) : timeout_seconds(timeout) {
            reset();
        }

        void reset() {
            start_time = time(nullptr);
            connection_time = 0;
            handshake_time = 0;
            send_time = 0;
            receive_time = 0;
            total_time = 0;
        }

        void markConnection() { connection_time = time(nullptr); }
        void markHandshake() { handshake_time = time(nullptr); }
        void markSend() { send_time = time(nullptr); }
        void markReceive() { receive_time = time(nullptr); }
        void finish() { total_time = time(nullptr); }

        // 获取各阶段耗时
        int getConnectionDuration() const { return connection_time - start_time; }
        int getHandshakeDuration() const { return handshake_time - connection_time; }
        int getSendDuration() const { return send_time - handshake_time; }
        int getReceiveDuration() const { return receive_time - send_time; }
        int getTotalDuration() const { return total_time - start_time; }

        // 检查是否超时
        bool isTimeout() const {
            time_t current_time = time(nullptr);
            return (current_time - start_time) >= timeout_seconds;
        }

        // 获取剩余时间
        int getRemainingTime() const {
            time_t current_time = time(nullptr);
            int elapsed = current_time - start_time;
            return (elapsed < timeout_seconds) ? (timeout_seconds - elapsed) : 0;
        }
    };

    // 请求资源结构体 - 为每个请求创建独立的资源
    struct RequestResources {
        mbedtls_net_context server_fd;
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config conf;
        int sockfd;  // 自定义socket文件描述符

        RequestResources() : sockfd(-1) {
            mbedtls_net_init(&server_fd);
            mbedtls_ssl_init(&ssl);
            mbedtls_ssl_config_init(&conf);
        }

        ~RequestResources() {
            cleanup();
        }

        void cleanup() {
            // 只有在SSL连接已经建立的情况下才发送close_notify
            // 避免在连接失败时调用close_notify导致错误
            if (ssl.state != MBEDTLS_SSL_HELLO_REQUEST) {
                mbedtls_ssl_close_notify(&ssl);
            }

            // 释放mbedtls资源
            mbedtls_ssl_free(&ssl);
            mbedtls_ssl_config_free(&conf);

            // 手动关闭socket
            if (sockfd >= 0) {
                closeSocket(sockfd);
                sockfd = -1;
            }

            // 重新初始化mbedtls网络上下文，避免重复关闭
            mbedtls_net_init(&server_fd);
        }

        void closeSocket(int sockfd) {
            close(sockfd);
        }
    };

    // mbedtls 上下文
    mbedtls_net_context server_fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    // 成员变量
    map<string, CertificateInfo> pinned_certificates;
    int default_timeout_seconds;
    bool initialized;

public:
    // 构造函数
    explicit zHttps(int timeout_seconds = 10);

    // 移动构造函数
    zHttps(zHttps&& other) noexcept;

    // 移动赋值运算符
    zHttps& operator=(zHttps&& other) noexcept;

    // 禁用拷贝构造和赋值
    zHttps(const zHttps&) = delete;
    zHttps& operator=(const zHttps&) = delete;

    // 设置默认超时时间
    void setTimeout(int timeout_seconds);

    // 获取默认超时时间
    int getTimeout() const;

    ~zHttps();

    // 初始化mbedtls
    bool initialize();

    // 添加证书固定
    void addPinnedCertificate(const string& hostname,
                              const string& expected_serial,
                              const string& expected_fingerprint,
                              const string& expected_subject = "");

    // 执行HTTPS请求
    HttpsResponse performRequest(const HttpsRequest& request);

private:
    // 辅助函数
    CertificateInfo extractCertificateInfo(const mbedtls_x509_crt* cert);
    bool verifyCertificatePinning(const mbedtls_x509_crt* cert, const string& hostname);
    void getCertificateSerialHex(const mbedtls_x509_crt* cert, char* buffer, size_t buffer_size);
    int getCertificateFingerprintSha256(const mbedtls_x509_crt* cert, unsigned char* fingerprint);
    void parseHttpsResponse(const string& raw_response, HttpsResponse& response);
    void processChunkedBody(string& body);
    void cleanup();

    // 超时检测函数
    bool isTimeoutReached(time_t start_time, int timeout_seconds);

    // Socket连接相关方法
    int connectWithTimeout(const string& host, int port, int timeout_seconds);
    void closeSocket(int sockfd);
};

#endif // SSLCHECK_ZHTTPS_H
