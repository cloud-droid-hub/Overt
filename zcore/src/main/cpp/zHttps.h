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

/**
 * 证书信息结构体
 * 存储SSL证书的详细信息，用于证书验证和固定
 */
struct CertificateInfo {
    string serial_number;        // 证书序列号（十六进制）
    string fingerprint_sha256;   // 证书SHA256指纹
    string subject;              // 证书主题（Subject）
    string issuer;               // 证书颁发者（Issuer）
    string valid_from;           // 证书有效期开始时间
    string valid_to;             // 证书有效期结束时间
    string public_key_type;      // 公钥类型（如RSA、ECDSA等）
    size_t public_key_size = 0;  // 公钥长度（位）
    bool is_valid = false;       // 证书是否有效
    bool is_expired = false;     // 证书是否已过期
    bool is_future = false;      // 证书是否还未生效
};

/**
 * HTTPS请求结构体
 * 封装HTTPS请求的所有信息，包括URL、方法、头部、主体等
 */
struct HttpsRequest {
    string url;                    // 完整的HTTPS URL
    string method;                 // HTTP方法（GET、POST等）
    string host;                   // 主机名（从URL解析）
    string path;                   // 请求路径（从URL解析）
    int port;                      // 端口号（从URL解析，默认443）
    map<string, string> headers;   // HTTP请求头
    string body;                   // 请求主体
    int timeout_seconds;           // 超时时间（秒）

    /**
     * 构造函数
     * @param url 完整的HTTPS URL
     * @param method HTTP方法
     * @param timeout 超时时间（秒），默认10秒
     */
    HttpsRequest(const string& url, const string& method, int timeout = 10)
            : url(url), method(method), timeout_seconds(timeout) {
        parseUrl();
    }

    /**
     * 解析URL，提取主机名、路径和端口
     */
    void parseUrl();
    
    /**
     * 构建HTTP请求字符串
     * @return 完整的HTTP请求字符串
     */
    string buildRequest() const;
};

/**
 * HTTPS响应结构体
 * 封装HTTPS响应的所有信息，包括状态码、头部、主体、证书信息等
 */
struct HttpsResponse {
    int status_code = 0;                    // HTTP状态码
    map<string, string> headers;            // HTTP响应头
    string body;                            // 响应主体
    string error_message;                   // 错误消息

    // 证书验证结果
    bool ssl_verification_passed = false;   // SSL证书验证是否通过
    bool certificate_pinning_passed = false; // 证书固定验证是否通过

    // 证书信息
    CertificateInfo certificate;            // 服务器证书信息
    CertificateInfo pinned_certificate;     // 固定的证书信息

    /**
     * 检查响应是否安全
     * @return true表示响应安全（SSL验证通过、证书固定通过、状态码有效）
     */
    bool isSecure() const {
        return ssl_verification_passed && certificate_pinning_passed && status_code > 0;
    }
};

/**
 * HTTPS客户端类
 * 基于mbedtls库实现的HTTPS客户端，支持证书固定和验证
 * 提供安全的HTTPS通信功能，包括TLS握手、证书验证、超时控制等
 */
class zHttps {
private:
    /**
     * 请求计时器结构体
     * 用于跟踪HTTPS请求各个阶段的耗时
     */
    struct RequestTimer {
        time_t start_time;           // 请求开始时间
        time_t connection_time;      // 连接建立时间
        time_t handshake_time;       // TLS握手时间
        time_t send_time;            // 发送请求时间
        time_t receive_time;         // 接收响应时间
        time_t total_time;           // 总耗时
        int timeout_seconds;         // 本次请求的超时时间

        /**
         * 构造函数
         * @param timeout 超时时间（秒），默认10秒
         */
        RequestTimer(int timeout = 10) : timeout_seconds(timeout) {
            reset();
        }

        /**
         * 重置计时器
         */
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

        /**
         * 检查是否超时
         * @return true表示已超时
         */
        bool isTimeout() const {
            time_t current_time = time(nullptr);
            return (current_time - start_time) >= timeout_seconds;
        }

        /**
         * 获取剩余时间
         * @return 剩余时间（秒）
         */
        int getRemainingTime() const {
            time_t current_time = time(nullptr);
            int elapsed = current_time - start_time;
            return (elapsed < timeout_seconds) ? (timeout_seconds - elapsed) : 0;
        }
    };

    /**
     * 请求资源结构体
     * 为每个请求创建独立的mbedtls资源，避免资源冲突
     */
    struct RequestResources {
        mbedtls_net_context server_fd;  // mbedtls网络上下文
        mbedtls_ssl_context ssl;        // SSL上下文
        mbedtls_ssl_config conf;        // SSL配置
        int sockfd;                     // 自定义socket文件描述符

        /**
         * 构造函数
         * 初始化mbedtls资源
         */
        RequestResources() : sockfd(-1) {
            mbedtls_net_init(&server_fd);
            mbedtls_ssl_init(&ssl);
            mbedtls_ssl_config_init(&conf);
        }

        /**
         * 析构函数
         * 自动清理资源
         */
        ~RequestResources() {
            cleanup();
        }

        /**
         * 清理资源
         * 释放mbedtls资源和socket连接
         */
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

        /**
         * 关闭socket连接
         * @param sockfd socket文件描述符
         */
        void closeSocket(int sockfd) {
            close(sockfd);
        }
    };

    // mbedtls 上下文
    mbedtls_net_context server_fd;        // mbedtls网络上下文
    mbedtls_ssl_context ssl;              // SSL上下文
    mbedtls_ssl_config conf;              // SSL配置
    mbedtls_x509_crt cacert;              // CA证书链
    mbedtls_ctr_drbg_context ctr_drbg;    // 随机数生成器
    mbedtls_entropy_context entropy;      // 熵源

    // 成员变量
    map<string, CertificateInfo> pinned_certificates;  // 固定的证书信息
    int default_timeout_seconds;                       // 默认超时时间（秒）
    bool initialized;                                  // 是否已初始化

public:
    /**
     * 构造函数
     * @param timeout_seconds 默认超时时间（秒），默认10秒
     */
    explicit zHttps(int timeout_seconds = 10);

    /**
     * 移动构造函数
     * @param other 要移动的zHttps对象
     */
    zHttps(zHttps&& other) noexcept;

    /**
     * 移动赋值运算符
     * @param other 要移动的zHttps对象
     * @return 当前对象的引用
     */
    zHttps& operator=(zHttps&& other) noexcept;

    // 禁用拷贝构造和赋值
    zHttps(const zHttps&) = delete;
    zHttps& operator=(const zHttps&) = delete;

    /**
     * 设置默认超时时间
     * @param timeout_seconds 超时时间（秒）
     */
    void setTimeout(int timeout_seconds);

    /**
     * 获取默认超时时间
     * @return 默认超时时间（秒）
     */
    int getTimeout() const;

    /**
     * 析构函数
     * 自动清理资源
     */
    ~zHttps();

    /**
     * 初始化mbedtls
     * @return true表示初始化成功，false表示失败
     */
    bool initialize();

    /**
     * 添加证书固定
     * @param hostname 主机名
     * @param expected_serial 期望的证书序列号
     * @param expected_fingerprint 期望的证书指纹
     * @param expected_subject 期望的证书主题（可选）
     */
    void addPinnedCertificate(const string& hostname,
                              const string& expected_serial,
                              const string& expected_fingerprint,
                              const string& expected_subject = "");

    /**
     * 执行HTTPS请求
     * @param request HTTPS请求对象
     * @return HTTPS响应对象
     */
    HttpsResponse performRequest(const HttpsRequest& request);

private:
    // 辅助函数
    /**
     * 提取证书信息
     * @param cert mbedtls证书对象
     * @return 证书信息结构体
     */
    CertificateInfo extractCertificateInfo(const mbedtls_x509_crt* cert);
    
    /**
     * 验证证书固定
     * @param cert mbedtls证书对象
     * @param hostname 主机名
     * @return true表示验证通过，false表示失败
     */
    bool verifyCertificatePinning(const mbedtls_x509_crt* cert, const string& hostname);
    
    /**
     * 获取证书序列号（十六进制）
     * @param cert mbedtls证书对象
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     */
    void getCertificateSerialHex(const mbedtls_x509_crt* cert, char* buffer, size_t buffer_size);
    
    /**
     * 获取证书SHA256指纹
     * @param cert mbedtls证书对象
     * @param fingerprint 输出指纹缓冲区
     * @return 0表示成功，其他值表示失败
     */
    int getCertificateFingerprintSha256(const mbedtls_x509_crt* cert, unsigned char* fingerprint);
    
    /**
     * 解析HTTPS响应
     * @param raw_response 原始响应字符串
     * @param response 解析后的响应对象
     */
    void parseHttpsResponse(const string& raw_response, HttpsResponse& response);
    
    /**
     * 处理分块传输编码的响应体
     * @param body 响应体字符串
     */
    void processChunkedBody(string& body);
    
    /**
     * 清理资源
     */
    void cleanup();

    /**
     * 检查是否超时
     * @param start_time 开始时间
     * @param timeout_seconds 超时时间（秒）
     * @return true表示已超时，false表示未超时
     */
    bool isTimeoutReached(time_t start_time, int timeout_seconds);

    // Socket连接相关方法
    /**
     * 带超时的socket连接
     * @param host 主机名
     * @param port 端口号
     * @param timeout_seconds 超时时间（秒）
     * @return socket文件描述符，失败返回-1
     */
    int connectWithTimeout(const string& host, int port, int timeout_seconds);
    
    /**
     * 关闭socket连接
     * @param sockfd socket文件描述符
     */
    void closeSocket(int sockfd);
};

#endif // SSLCHECK_ZHTTPS_H
