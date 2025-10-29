//
// Created by lxz on 2025/7/27.
//


#include <errno.h>

#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zHttps.h"

// HttpsRequest方法实现
/**
 * 解析URL
 * 从完整的HTTPS URL中提取主机名、端口号和路径
 * 只支持HTTPS协议，确保安全性
 */
void HttpsRequest::parseUrl() {
    LOGD("parseUrl called for URL: %s", url.c_str());
    // 强制只支持HTTPS的URL解析
    if (url.substr(0, 8) != "https://") {
        LOGD("parseUrl: URL is not HTTPS, skipping");
        return;
    }

    // 移除协议部分
    string url_without_protocol = url.substr(8);

    // 查找主机名和路径的分隔符
    size_t path_start = url_without_protocol.find('/');
    if (path_start != string::npos) {
        host = url_without_protocol.substr(0, path_start);
        path = url_without_protocol.substr(path_start);
    } else {
        host = url_without_protocol;
        path = "/";
    }

    // 检查主机名是否包含端口
    size_t port_start = host.find(':');
    if (port_start != string::npos) {
        string port_str = host.substr(port_start + 1);
        try {
            port = atoi(port_str.c_str());
            host = host.substr(0, port_start);
        } catch (const std::exception& e) {
            LOGW("Invalid port number: %s", port_str.c_str());
            port = 443; // 默认HTTPS端口
        }
    } else {
        port = 443; // 默认HTTPS端口
    }

    LOGI("parseUrl: parsed host=%s, port=%d, path=%s", host.c_str(), port, path.c_str());
}

/**
 * 构建HTTP请求
 * 根据解析的URL信息构建完整的HTTP请求字符串
 * 包括请求行、头部和可选的请求体
 * @return 构建的HTTP请求字符串
 */
string HttpsRequest::buildRequest() const {
    LOGD("buildRequest called");
    string request = method + " " + path + " HTTP/1.1\r\n";
    request += "Host: ";
    request += host;
    // 只有在非默认端口时才添加端口号
    if (port != 443) {
        request += ":";
        request += to_string(port);
    }
    request += "\r\n";
    // 添加一些标准的HTTP头部
    request += "User-Agent: Overt/1.0\r\n";
    request += "Accept: */*\r\n";
    request += "Connection: close\r\n";
    for (const auto& header : headers) {
        request += header.first + ": " + header.second + "\r\n";
    }
    if (!body.empty()) {
        request += "Content-Length: ";
        request += to_string(body.length());
        request += "\r\n";
    }
    request += "\r\n";
    if (!body.empty()) {
        request += body;
    }
    LOGI("buildRequest: request data=%s", request.c_str());
    LOGI("buildRequest: request length=%zu", request.length());
    return request;
}

// zHttps方法实现
/**
 * 构造函数
 * 初始化HTTPS客户端，设置默认超时时间
 * 不在构造函数中初始化mbedtls资源，而是在需要时初始化
 * @param timeout_seconds 默认超时时间（秒）
 */
zHttps::zHttps(int timeout_seconds) : initialized(false), default_timeout_seconds(timeout_seconds) {
    LOGD("Constructor called with timeout: %d seconds", timeout_seconds);
    // 不在构造函数中初始化mbedtls资源，而是在需要时初始化
    // 这样可以避免资源泄漏和状态污染
}

/**
 * 移动构造函数
 * 高效地移动另一个zHttps对象的资源
 * 避免不必要的资源复制，提高性能
 * @param other 要移动的zHttps对象
 */
zHttps::zHttps(zHttps&& other) noexcept 
    : initialized(other.initialized)
    , default_timeout_seconds(other.default_timeout_seconds)
    , pinned_certificates(std::move(other.pinned_certificates)) {
    
    LOGD("Move constructor called");
    
    // 移动mbedtls资源
    server_fd = other.server_fd;
    ssl = other.ssl;
    conf = other.conf;
    cacert = other.cacert;
    ctr_drbg = other.ctr_drbg;
    entropy = other.entropy;
    
    // 重置源对象
    other.initialized = false;
    other.default_timeout_seconds = 10;
    
    // 重新初始化源对象的mbedtls资源
    mbedtls_net_init(&other.server_fd);
    mbedtls_ssl_init(&other.ssl);
    mbedtls_ssl_config_init(&other.conf);
    mbedtls_x509_crt_init(&other.cacert);
    mbedtls_ctr_drbg_init(&other.ctr_drbg);
    mbedtls_entropy_init(&other.entropy);
}

/**
 * 移动赋值运算符
 * 高效地移动另一个zHttps对象的资源到当前对象
 * 先清理当前对象的资源，然后移动新资源
 * @param other 要移动的zHttps对象
 * @return 当前对象的引用
 */
zHttps& zHttps::operator=(zHttps&& other) noexcept {
    LOGD("Move assignment operator called");
    
    if (this != &other) {
        // 清理当前资源
        cleanup();
        
        // 移动数据
        initialized = other.initialized;
        default_timeout_seconds = other.default_timeout_seconds;
        pinned_certificates = std::move(other.pinned_certificates);
        
        // 移动mbedtls资源
        server_fd = other.server_fd;
        ssl = other.ssl;
        conf = other.conf;
        cacert = other.cacert;
        ctr_drbg = other.ctr_drbg;
        entropy = other.entropy;
        
        // 重置源对象
        other.initialized = false;
        other.default_timeout_seconds = 10;
        
        // 重新初始化源对象的mbedtls资源
        mbedtls_net_init(&other.server_fd);
        mbedtls_ssl_init(&other.ssl);
        mbedtls_ssl_config_init(&other.conf);
        mbedtls_x509_crt_init(&other.cacert);
        mbedtls_ctr_drbg_init(&other.ctr_drbg);
        mbedtls_entropy_init(&other.entropy);
    }
    
    return *this;
}

/**
 * 设置默认超时时间
 * 为后续的HTTPS请求设置默认的超时时间
 * @param timeout_seconds 超时时间（秒）
 */
void zHttps::setTimeout(int timeout_seconds) {
    LOGD("setTimeout called with %d", timeout_seconds);
    default_timeout_seconds = timeout_seconds;
    LOGI("Default timeout set to %d seconds", timeout_seconds);
}

/**
 * 获取默认超时时间
 * 返回当前设置的默认超时时间
 * @return 默认超时时间（秒）
 */
int zHttps::getTimeout() const {
    LOGD("getTimeout called");
    return default_timeout_seconds;
}

/**
 * 析构函数
 * 自动清理所有mbedtls资源
 * 确保没有资源泄漏
 */
zHttps::~zHttps() {
    LOGD("Destructor called");
    cleanup();
}

/**
 * 初始化mbedtls
 * 初始化所有mbedtls相关的资源，包括SSL上下文、证书、随机数生成器等
 * 加载系统CA证书用于证书验证
 * @return true表示初始化成功，false表示失败
 */
bool zHttps::initialize() {
    LOGD("initialize called");
    if (initialized) {
        LOGD("initialize: already initialized");
        return true;
    }

    // 初始化mbedtls资源
    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    const char* pers = "https_client";
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    (const unsigned char*)pers, strlen(pers));
    if (ret != 0) {
        LOGW("Failed to seed random number generator: %d", ret);
        cleanup();
        return false;
    }

    // 加载CA证书
    ret = mbedtls_x509_crt_parse_path(&cacert, "/system/etc/security/cacerts");
    if (ret < 0) {
        ret = mbedtls_x509_crt_parse_path(&cacert, "/system/etc/ssl/certs");
        if (ret < 0) {
            LOGW("Warning: No CA certificates loaded, continuing without verification");
        }
    }

    initialized = true;
    LOGI("initialize: mbedtls initialized successfully");
    return true;
}

/**
 * 添加证书固定
 * 为指定的主机名添加证书固定信息，用于防止中间人攻击
 * 通过验证证书的序列号、指纹和主题来确保连接的安全性
 * @param hostname 主机名
 * @param expected_serial 期望的证书序列号
 * @param expected_fingerprint 期望的证书SHA256指纹
 * @param expected_subject 期望的证书主题（可选）
 */
void zHttps::addPinnedCertificate(const string& hostname,
                                  const string& expected_serial,
                                  const string& expected_fingerprint,
                                  const string& expected_subject) {
    LOGD("addPinnedCertificate called for hostname: %s", hostname.c_str());
    CertificateInfo cert_info;
    cert_info.serial_number = expected_serial;
    cert_info.fingerprint_sha256 = expected_fingerprint;
    cert_info.subject = expected_subject;
    cert_info.issuer = ""; // 初始化issuer字段
    cert_info.is_valid = true;
    cert_info.is_expired = false;
    cert_info.is_future = false;
    cert_info.public_key_size = 0;

    pinned_certificates[hostname] = cert_info;
    LOGI("Added pinned certificate for %s", hostname.c_str());
}

/**
 * 提取证书信息
 * 从mbedtls证书对象中提取各种证书信息
 * 包括序列号、指纹、主题、颁发者、有效期等
 * @param cert mbedtls证书对象
 * @return 证书信息结构体
 */
CertificateInfo zHttps::extractCertificateInfo(const mbedtls_x509_crt* cert) {
    LOGD("extractCertificateInfo called");
    CertificateInfo info;

    if (!cert) {
        LOGD("extractCertificateInfo: cert is null");
        return info;
    }

    // 序列号
    char serial_hex[256];
    getCertificateSerialHex(cert, serial_hex, sizeof(serial_hex));
    info.serial_number = serial_hex;

    // 指纹
    unsigned char fingerprint[32];
    if (getCertificateFingerprintSha256(cert, fingerprint) == 0) {
        char fingerprint_hex[65];
        for (int i = 0; i < 32; i++) {
            snprintf(fingerprint_hex + i * 2, 3, "%02X", fingerprint[i]);
        }
        info.fingerprint_sha256 = fingerprint_hex;
    }

    // 主题
    char subject[512];
    mbedtls_x509_dn_gets(subject, sizeof(subject), &cert->subject);
    info.subject = subject;

    // 颁发者
    char issuer[512];
    mbedtls_x509_dn_gets(issuer, sizeof(issuer), &cert->issuer);
    info.issuer = issuer;

    // 有效期
    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "%04d-%02d-%02d %02d:%02d:%02d",
             cert->valid_from.year, cert->valid_from.mon, cert->valid_from.day,
             cert->valid_from.hour, cert->valid_from.min, cert->valid_from.sec);
    info.valid_from = time_buf;

    snprintf(time_buf, sizeof(time_buf), "%04d-%02d-%02d %02d:%02d:%02d",
             cert->valid_to.year, cert->valid_to.mon, cert->valid_to.day,
             cert->valid_to.hour, cert->valid_to.min, cert->valid_to.sec);
    info.valid_to = time_buf;

    // 公钥信息
    info.public_key_type = mbedtls_pk_get_name(&cert->pk);
    info.public_key_size = mbedtls_pk_get_bitlen(&cert->pk);

    // 有效性检查
    info.is_expired = mbedtls_x509_time_is_past(&cert->valid_to);
    info.is_future = mbedtls_x509_time_is_future(&cert->valid_from);
    info.is_valid = !info.is_expired && !info.is_future;

    LOGI("extractCertificateInfo: extracted certificate info for subject: %s", info.subject.c_str());
    return info;
}

/**
 * 验证证书固定
 * 验证服务器证书是否与预定义的固定证书匹配
 * 通过比较序列号、指纹和主题来确保证书的真实性
 * @param cert mbedtls证书对象
 * @param hostname 主机名
 * @return true表示验证通过，false表示失败
 */
bool zHttps::verifyCertificatePinning(const mbedtls_x509_crt* cert, const string& hostname) {
    LOGD("verifyCertificatePinning called for hostname: %s", hostname.c_str());
    auto it = pinned_certificates.find(hostname);
    if (it == pinned_certificates.end()) {
        LOGI("No pinned certificate for hostname: %s, skipping pinning check.", hostname.c_str());
        return true; // 没有固定证书时跳过验证
    }

    const CertificateInfo& pinned = it->second;
    bool serial_match = (pinned.serial_number == extractCertificateInfo(cert).serial_number);
    bool fingerprint_match = (pinned.fingerprint_sha256 == extractCertificateInfo(cert).fingerprint_sha256);

    // 只有当期望的subject不为空时才验证subject
    bool subject_match = true;
    if (!pinned.subject.empty()) {
        subject_match = (pinned.subject == extractCertificateInfo(cert).subject);
    }

    if (!serial_match || !fingerprint_match || !subject_match) {
        LOGW("Certificate pinning verification failed for %s", hostname.c_str());
        LOGD("serial_match %d fingerprint_match %d subject_match %d ", serial_match, fingerprint_match, subject_match);

        // 输出详细信息用于调试
        if (!serial_match) {
            LOGD("Expected serial: %s, Got: %s",
                 pinned.serial_number.c_str(),
                 extractCertificateInfo(cert).serial_number.c_str());
        }
        if (!fingerprint_match) {
            LOGD("Expected fingerprint: %s, Got: %s",
                 pinned.fingerprint_sha256.c_str(),
                 extractCertificateInfo(cert).fingerprint_sha256.c_str());
        }
        if (!subject_match && !pinned.subject.empty()) {
            LOGD("Expected subject: %s, Got: %s",
                 pinned.subject.c_str(),
                 extractCertificateInfo(cert).subject.c_str());
        }

        return false;
    }
    LOGI("Certificate pinning verification passed for %s", hostname.c_str());
    return true;
}

/**
 * 执行HTTPS请求
 * 执行完整的HTTPS请求流程，包括连接建立、TLS握手、证书验证、请求发送和响应接收
 * 支持超时控制、证书固定验证和详细的错误处理
 * @param request HTTPS请求对象
 * @return HTTPS响应对象
 */
HttpsResponse zHttps::performRequest(const HttpsRequest& request) {
    HttpsResponse response;

    // 使用请求的超时时间，如果没有设置则使用默认超时
    int timeout_seconds = request.timeout_seconds > 0 ? request.timeout_seconds : default_timeout_seconds;

    // 为本次请求创建独立的计时器和资源
    RequestTimer timer(timeout_seconds);
    RequestResources resources;
    LOGI("Starting HTTPS request with timeout: %d seconds", timer.timeout_seconds);

    // 安全检查：确保只使用HTTPS协议，但允许任意端口
    if (request.url.substr(0, 8) != "https://") {
        response.error_message = "Only HTTPS URLs are supported";
        LOGE("Security Error: Only HTTPS protocol is allowed");
        return response;
    }

    // 初始化mbedtls（如果需要）
    if (!initialized) {
        if (!initialize()) {
            response.error_message = "Failed to initialize mbedtls";
            LOGE("Failed to initialize mbedtls");
            return response;
        }
    }

    // 确保每次请求都使用干净的状态
    // 这可以防止前一次请求的失败状态影响当前请求
    LOGI("Starting new HTTPS request to %s", request.host.c_str());

    // 重置全局SSL状态，确保每次请求都是独立的
    // 这可以防止前一次请求的失败状态影响当前请求
    if (initialized) {
        // 重新初始化全局资源以确保干净状态
        cleanup();
        if (!initialize()) {
            response.error_message = "Failed to reinitialize mbedtls";
            LOGE("Failed to reinitialize mbedtls");
            return response;
        }
    }

    // 记录本地设置的证书固定信息
    auto it = pinned_certificates.find(request.host);
    if (it != pinned_certificates.end()) {
        response.pinned_certificate = it->second;
    }

    char error_buf[0x1000];
    int ret;

    // 使用自定义socket连接
    LOGI("Connecting to %s:%d with custom socket...", request.host.c_str(), request.port);

    // 检查超时
    if (timer.isTimeout()) {
        response.error_message = "Connection timeout before establishing connection";
        LOGE("Connection timeout before establishing connection");
        return response;
    }

    // 记录连接开始时间
    time_t connect_start = time(nullptr);
    LOGI("Connection attempt started at: %ld", connect_start);

    // 使用自定义socket连接，带超时控制
    resources.sockfd = connectWithTimeout(request.host, request.port, timeout_seconds);

    // 记录连接结束时间
    time_t connect_end = time(nullptr);
    int connect_duration = connect_end - connect_start;
    LOGI("Connection attempt completed in %d seconds", connect_duration);

    if (resources.sockfd < 0) {
        response.error_message = "Connection failed with custom socket";
        LOGE("Connection failed after %d seconds with custom socket", connect_duration);
        return response;
    }

    // 将socket文件描述符设置到mbedtls网络上下文
    resources.server_fd.fd = resources.sockfd;

    // 标记连接建立完成
    timer.markConnection();
    LOGI("Connection established successfully in %d seconds", timer.getConnectionDuration());

    // 设置socket为非阻塞模式
    ret = mbedtls_net_set_nonblock(&resources.server_fd);
    if (ret != 0) {
        LOGI("Warning: Failed to set socket to non-blocking mode");
    } else {
        LOGI("Socket set to non-blocking mode");
    }

    // 配置SSL - 强制使用安全的SSL配置
    ret = mbedtls_ssl_config_defaults(&resources.conf, MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        response.error_message = "SSL config failed: " + string(error_buf);
        LOGE("SSL config failed: %s", error_buf);
        // RequestResources会在析构时自动清理
        return response;
    }

    // 设置SSL读取超时（毫秒）
    uint32_t ssl_timeout_ms = timeout_seconds * 1000;
    mbedtls_ssl_conf_read_timeout(&resources.conf, ssl_timeout_ms);
    LOGI("SSL read timeout set to %u ms", ssl_timeout_ms);

    // 设置握手超时（DTLS，但对TLS也有影响）
    uint32_t handshake_timeout_ms = timeout_seconds * 1000;
    mbedtls_ssl_conf_handshake_timeout(&resources.conf, handshake_timeout_ms, handshake_timeout_ms * 2);
    LOGI("SSL handshake timeout set to %u-%u ms", handshake_timeout_ms, handshake_timeout_ms * 2);

    // 强制证书验证 - 不允许跳过验证
    mbedtls_ssl_conf_authmode(&resources.conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&resources.conf, &cacert, nullptr);
    mbedtls_ssl_conf_rng(&resources.conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    // 设置最小TLS版本为1.2（禁用TLS 1.0和1.1）
    mbedtls_ssl_conf_min_version(&resources.conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

    // 禁用不安全的加密套件
    mbedtls_ssl_conf_ciphersuites(&resources.conf, mbedtls_ssl_list_ciphersuites());

    ret = mbedtls_ssl_setup(&resources.ssl, &resources.conf);
    if (ret != 0) {
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        response.error_message = "SSL setup failed: " + string(error_buf);
        LOGE("SSL setup failed: %s", error_buf);
        // RequestResources会在析构时自动清理
        return response;
    }

    mbedtls_ssl_set_hostname(&resources.ssl, request.host.c_str());

    // 使用带超时的网络接收函数
    mbedtls_ssl_set_bio(&resources.ssl, &resources.server_fd, mbedtls_net_send, nullptr, mbedtls_net_recv_timeout);

    // TLS握手
    LOGI("Performing TLS handshake...");

    // 检查超时
    if (timer.isTimeout()) {
        response.error_message = "TLS handshake timeout before starting";
        LOGE("TLS handshake timeout before starting");
        return response;
    }

    // 使用轮询进行TLS握手，带超时检查
    do {
        ret = mbedtls_ssl_handshake(&resources.ssl);

        // 检查超时
        if (timer.isTimeout()) {
            response.error_message = "TLS handshake timeout during negotiation";
            LOGE("TLS handshake timeout during negotiation");
            return response;
        }

        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            // 使用轮询等待数据
            uint32_t poll_timeout = 100; // 100ms轮询间隔
            ret = mbedtls_net_poll(&resources.server_fd,
                                   (ret == MBEDTLS_ERR_SSL_WANT_READ) ? MBEDTLS_NET_POLL_READ : MBEDTLS_NET_POLL_WRITE,
                                   poll_timeout);
            if (ret < 0) {
                LOGI("Poll failed, continuing handshake...");
            }
            continue;
        }
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    if (ret != 0) {
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        response.error_message = "TLS handshake failed: " + string(error_buf);
        LOGE("TLS handshake failed: %s", error_buf);
        // RequestResources会在析构时自动清理
        return response;
    }

    // 标记TLS握手完成
    timer.markHandshake();
    LOGI("TLS handshake completed in %d seconds", timer.getHandshakeDuration());

    // 验证证书
    uint32_t flags = mbedtls_ssl_get_verify_result(&resources.ssl);
    if (flags != 0) {
        response.error_message = "Certificate verification failed";
        LOGE("Certificate verification failed");
        response.ssl_verification_passed = false;
    } else {
        response.ssl_verification_passed = true;
        LOGI("Certificate verification passed");
    }

    // 获取服务器证书
    const mbedtls_x509_crt* cert = mbedtls_ssl_get_peer_cert(&resources.ssl);
    if (cert) {
        response.certificate = extractCertificateInfo(cert);

        // 验证证书固定
        response.certificate_pinning_passed = verifyCertificatePinning(cert, request.host);
    }

    // 发送HTTPS请求
    string https_request = request.buildRequest();
    LOGI("Sending HTTPS request...");

    // 检查超时
    if (timer.isTimeout()) {
        response.error_message = "Request timeout before sending";
        LOGE("Request timeout before sending");
        return response;
    }

    ret = mbedtls_ssl_write(&resources.ssl, (const unsigned char*)https_request.c_str(), https_request.length());
    if (ret < 0) {
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        response.error_message = "Write failed: " + string(error_buf);
        LOGE("Write error: %s", error_buf);
        return response;
    }

    // 标记发送完成
    timer.markSend();
    LOGI("Request sent successfully in %d seconds", timer.getSendDuration());

    // 读取响应
    LOGI("Reading HTTPS response...");
    char response_buf[4096];
    string full_response;
    int read_count = 0;
    const int max_reads = 3; // 简化最大读取次数
    const int max_total_bytes = 64 * 1024; // 最大64KB响应
    bool found_headers = false;
    bool response_complete = false;

    do {
        // 检查超时
        if (timer.isTimeout()) {
            response.error_message = "Response read timeout";
            LOGE("Response read timeout after %d seconds", timer.timeout_seconds);
            break;
        }

        ret = mbedtls_ssl_read(&resources.ssl, (unsigned char*)response_buf, sizeof(response_buf) - 1);
        read_count++;

        if (ret > 0) {
            response_buf[ret] = '\0';
            full_response += response_buf;
            LOGI("Read %d bytes, total: %zu bytes", ret, full_response.length());

            // 检查是否找到HTTPS头
            if (!found_headers &&
                (full_response.find("\r\n\r\n") != string::npos ||
                 full_response.find("\n\n") != string::npos)) {
                found_headers = true;
                LOGI("Found HTTPS headers, continuing to read body...");
            }

            // 检查响应是否完整
            if (found_headers && !response_complete) {
                size_t header_end = full_response.find("\r\n\r\n");
                if (header_end == string::npos) {
                    header_end = full_response.find("\n\n");
                }

                if (header_end != string::npos) {
                    string headers = full_response.substr(0, header_end);

                    // 检查是否有Content-Length
                    LOGD("=== Content-Length 解析开始 ===");
                    LOGD("headers 长度: %zu", headers.length());
                    LOGD("headers 内容: '%s'", headers.c_str());

                    size_t content_length_pos = headers.find("Content-Length:");
                    LOGD("Content-Length 位置: %zu", content_length_pos);

                    if (content_length_pos != string::npos) {
                        LOGD("找到 Content-Length 头部");
                        LOGD("Content-Length 位置 + 15: %zu", content_length_pos + 15);

                        size_t value_start = headers.find_first_not_of(" \t", content_length_pos + 15);
                        LOGD("value_start 位置: %zu", value_start);

                        if (value_start != string::npos) {
                            LOGD("value_start 有效，开始查找结束位置");

                            size_t value_end = headers.find("\r\n", value_start);
                            LOGD("查找 \\r\\n 结束位置: %zu", value_end);

                            if (value_end == string::npos) {
                                value_end = headers.find("\n", value_start);
                                LOGD("未找到 \\r\\n，查找 \\n 结束位置: %zu", value_end);
                            }

                            LOGD("最终 value_end 位置: %zu", value_end);
                            LOGD("value_end > value_start 检查: %s", (value_end > value_start) ? "true" : "false");

                            if (value_end != string::npos && value_end > value_start) {
                                LOGD("开始提取长度字符串");
                                LOGD("提取范围: value_start=%zu, value_end=%zu, 长度=%zu",
                                     value_start, value_end, value_end - value_start);

                                string length_str = headers.substr(value_start, value_end - value_start);
                                LOGD("提取的长度字符串: '%s'", length_str.c_str());
                                LOGD("长度字符串长度: %zu", length_str.length());

                                try {
                                    LOGD("开始调用 atoi 转换");
                                    int expected_length = atoi(length_str.c_str());
                                    LOGD("atoi 转换结果: %d", expected_length);

                                    size_t body_length = full_response.length() - header_end - 4;
                                    LOGD("计算的实际体长度: %zu", body_length);
                                    LOGD("header_end: %zu, full_response.length(): %zu", header_end, full_response.length());

                                    if (body_length >= static_cast<size_t>(expected_length)) {
                                        LOGI("Response body complete (Content-Length: %d, actual: %zu)",
                                             expected_length, body_length);
                                        response_complete = true;
                                    } else {
                                        LOGD("体长度不足，期望: %d, 实际: %zu", expected_length, body_length);
                                    }
                                } catch (const std::exception& e) {
                                    LOGE("atoi 转换异常: %s", e.what());
                                }
                            } else {
                                LOGD("value_end 无效或长度检查失败");
                                if (value_end == string::npos) {
                                    LOGD("未找到结束标记");
                                } else if (value_end <= value_start) {
                                    LOGD("结束位置 <= 开始位置");
                                }
                            }
                        } else {
                            LOGD("value_start 无效 (string::npos)");
                        }
                    } else {
                        LOGD("未找到 Content-Length 头部");
                    }
                    LOGD("=== Content-Length 解析结束 ===");

                    // 检查是否有Transfer-Encoding: chunked
                    if (headers.find("Transfer-Encoding: chunked") != string::npos) {
                        if (full_response.find("\r\n0\r\n\r\n") != string::npos) {
                            LOGI("Chunked response complete");
                            response_complete = true;
                        }
                    }

                    // 检查Connection: close
                    if (headers.find("Connection: close") != string::npos &&
                        static_cast<size_t>(ret) < sizeof(response_buf) - 1) {
                        LOGI("Connection: close detected, response likely complete");
                        response_complete = true;
                    }
                }
            }

            // 检查是否超过最大响应大小
            if (full_response.length() > max_total_bytes) {
                LOGI("Response too large, stopping at %zu bytes", full_response.length());
                break;
            }
        } else if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            LOGI("SSL wants read/write, stopping (no retry)");
            break;
        } else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            LOGI("Peer closed connection");
            response_complete = true;
            break;
        } else if (ret == 0) {
            LOGI("Connection closed by peer (EOF)");
            break;
        } else {
            mbedtls_strerror(ret, error_buf, sizeof(error_buf));
            response.error_message = "Read failed: " + string(error_buf);
            LOGE("Read error: %s", error_buf);
            break;
        }

        // 防止无限循环
        if (read_count > max_reads) {
            LOGI("Reached max read count (%d), stopping", max_reads);
            break;
        }

        // 如果响应已完成，停止读取
        if (response_complete) {
            LOGI("Response marked as complete, stopping");
            break;
        }

        // 如果已经找到头，且读取了足够的数据，就停止
        if (found_headers && read_count > 1) {
            LOGI("Found headers and read enough data, stopping");
            break;
        }
    } while (ret > 0);

    // 标记接收完成
    timer.markReceive();
    LOGI("Response reading completed in %d seconds", timer.getReceiveDuration());

    LOGI("Finished reading response, total bytes: %zu, read attempts: %d, found headers: %s, complete: %s",
         full_response.length(), read_count, found_headers ? "yes" : "no", response_complete ? "yes" : "no");

    // 检查响应是否有效（即使没有标记为完整，只要有内容就继续）
    if (full_response.empty()) {
        response.error_message = "No response received";
        LOGE("No response received");
        return response;
    }

    // 检查是否至少找到了HTTP头
    if (!found_headers) {
        response.error_message = "No valid HTTP headers found";
        LOGE("No valid HTTP headers found");
        return response;
    }

    // 检查是否有响应内容
    if (full_response.empty()) {
        response.error_message = "No response received";
        LOGE("No response received");
        return response;
    }

    // 解析HTTPS响应
    parseHttpsResponse(full_response, response);

    // 标记请求完成
    timer.finish();
    LOGI("=== Request Timing Summary ===");
    LOGI("Connection: %d seconds", timer.getConnectionDuration());
    LOGI("TLS Handshake: %d seconds", timer.getHandshakeDuration());
    LOGI("Send Request: %d seconds", timer.getSendDuration());
    LOGI("Receive Response: %d seconds", timer.getReceiveDuration());
    LOGI("Total Duration: %d seconds", timer.getTotalDuration());

    // 资源会在RequestResources析构函数中自动清理
    LOGI("Request completed successfully, resources will be cleaned up automatically");

    return response;
}

/**
 * 获取证书序列号（十六进制）
 * 将证书的序列号转换为十六进制字符串格式
 * @param cert mbedtls证书对象
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 */
void zHttps::getCertificateSerialHex(const mbedtls_x509_crt* cert, char* buffer, size_t buffer_size) {
    size_t pos = 0;
    for (size_t i = 0; i < cert->serial.len && pos < buffer_size - 3; i++) {
        pos += snprintf(buffer + pos, buffer_size - pos, "%02X", cert->serial.p[i]);
        if (i < cert->serial.len - 1 && pos < buffer_size - 2) {
            pos += snprintf(buffer + pos, buffer_size - pos, ":");
        }
    }
}

/**
 * 获取证书SHA256指纹
 * 计算证书的SHA256指纹
 * @param cert mbedtls证书对象
 * @param fingerprint 输出指纹缓冲区（32字节）
 * @return 0表示成功，其他值表示失败
 */
int zHttps::getCertificateFingerprintSha256(const mbedtls_x509_crt* cert, unsigned char* fingerprint) {
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) return -1;
    return mbedtls_md(md_info, cert->raw.p, cert->raw.len, fingerprint);
}

/**
 * 解析HTTPS响应
 * 解析原始HTTP响应字符串，提取状态码、头部和响应体
 * 支持分块传输编码的响应体处理
 * @param raw_response 原始响应字符串
 * @param response 解析后的响应对象
 */
void zHttps::parseHttpsResponse(const string& raw_response, HttpsResponse& response) {
    LOGD("=== parseHttpsResponse 开始 ===");
    LOGD("raw_response 长度: %zu", raw_response.length());
    LOGD("raw_response 内容: '%s'", raw_response.c_str());

    if (raw_response.empty()) {
        response.error_message = "Empty response received";
        LOGE("Empty response");
        return;
    }

    // 查找头部和主体的分隔符
    LOGD("开始查找头部分隔符");
    size_t header_end = raw_response.find("\r\n\r\n");
    LOGD("查找 \\r\\n\\r\\n 结果: %zu", header_end);

    if (header_end == string::npos) {
        header_end = raw_response.find("\n\n");
        LOGD("查找 \\n\\n 结果: %zu", header_end);
    }

    if (header_end == string::npos) {
        response.error_message = "Invalid HTTPS response format - no header separator found";
        LOGE("Invalid HTTPS response format");
        return;
    }

    LOGD("找到头部结束位置: %zu", header_end);

    // 分离头部和主体
    LOGD("开始分离头部和主体");
    LOGD("提取头部范围: 0 到 %zu", header_end);
    string headers = raw_response.substr(0, header_end);
    LOGD("头部提取完成，长度: %zu", headers.length());

    LOGD("提取主体范围: %zu + 4 到结束", header_end);
    LOGD("检查边界: header_end + 4 = %zu, raw_response.length() = %zu", header_end + 4, raw_response.length());

    if (header_end + 4 >= raw_response.length()) {
        LOGD("边界检查失败，header_end + 4 超出或等于字符串长度");
        response.body = "";
        LOGD("主体设置为空字符串");
    } else {
        response.body = raw_response.substr(header_end + 4);
        LOGD("主体提取完成，长度: %zu", response.body.length());
    }

    LOGI("Headers length: %zu, Body length: %zu", headers.length(), response.body.length());

    // 打印响应体的预览
    if (!response.body.empty()) {
        size_t body_preview_length = response.body.length() < 1000 ? response.body.length() : 1000;
        string body_preview = response.body.substr(0, body_preview_length);
        LOGI("Response body preview (first %zu chars):", body_preview_length);
        LOGI("%s", body_preview.c_str());
    }

    // 解析状态行
    size_t first_line_end = headers.find('\n');
    if (first_line_end != string::npos) {
        string status_line = headers.substr(0, first_line_end);
        LOGI("Status line: %s", status_line.c_str());

        // 去除回车符
        if (!status_line.empty() && status_line.back() == '\r') {
            status_line.pop_back();
        }

        // 解析状态码
        LOGD("=== 状态码解析开始 ===");
        LOGD("status_line: '%s'", status_line.c_str());

        size_t space1 = status_line.find(' ');
        LOGD("space1 位置: %zu", space1);

        if (space1 != string::npos) {
            size_t space2 = status_line.find(' ', space1 + 1);
            LOGD("space2 位置: %zu", space2);

            if (space2 != string::npos) {
                LOGD("找到两个空格，使用第一种解析方式");
                try {
                    string status_code_str = status_line.substr(space1 + 1, space2 - space1 - 1);
                    LOGD("提取的状态码字符串: '%s'", status_code_str.c_str());
                    LOGD("状态码字符串长度: %zu", status_code_str.length());

                    LOGD("开始调用 atoi 转换状态码");
                    response.status_code = atoi(status_code_str.c_str());
                    LOGD("atoi 转换状态码结果: %d", response.status_code);
                    LOGI("Status code: %d", response.status_code);
                } catch (const std::exception& e) {
                    LOGE("Failed to parse status code: %s", e.what());
                    response.status_code = 0;
                }
            } else {
                LOGD("只找到一个空格，使用第二种解析方式");
                // 只有两个空格的情况
                try {
                    string status_code_str = status_line.substr(space1 + 1);
                    LOGD("提取的状态码字符串: '%s'", status_code_str.c_str());
                    LOGD("状态码字符串长度: %zu", status_code_str.length());

                    LOGD("开始调用 atoi 转换状态码");
                    response.status_code = atoi(status_code_str.c_str());
                    LOGD("atoi 转换状态码结果: %d", response.status_code);
                    LOGI("Status code: %d", response.status_code);
                } catch (const std::exception& e) {
                    LOGE("Failed to parse status code: %s", e.what());
                    response.status_code = 0;
                }
            }
        } else {
            LOGD("未找到空格，无法解析状态码");
        }
        LOGD("=== 状态码解析结束 ===");
    }

    // 解析头部
    size_t pos = first_line_end + 1;  // 跳过状态行
    int header_count = 0;
    string current_key;
    string current_value;

    while (pos < headers.length()) {
        // 找到行结束位置
        size_t line_end = headers.find('\n', pos);
        if (line_end == string::npos) {
            line_end = headers.length();
        }

        // 提取当前行
        string line = headers.substr(pos, line_end - pos);

        // 去除回车符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // 跳过空行
        if (line.empty()) {
            pos = line_end + 1;
            continue;
        }

        // 检查是否是续行（以空格或制表符开头）
        if ((line[0] == ' ' || line[0] == '\t') && !current_key.empty()) {
            // 这是续行，添加到当前值
            current_value += " " + line.substr(1);  // 去除前导空格
        } else {
            // 保存前一个头部（如果有的话）
            if (!current_key.empty()) {
                response.headers[current_key] = current_value;
                header_count++;
            }

            // 解析新的头部行
            size_t colon_pos = line.find(':');
            if (colon_pos != string::npos) {
                current_key = line.substr(0, colon_pos);
                current_value = line.substr(colon_pos + 1);

                // 去除前导空格
                if (!current_value.empty() && current_value[0] == ' ') {
                    current_value = current_value.substr(1);
                }
            } else {
                // 无效的头部行，重置
                current_key.clear();
                current_value.clear();
            }
        }

        pos = line_end + 1;
    }

    // 保存最后一个头部
    if (!current_key.empty()) {
        response.headers[current_key] = current_value;
        header_count++;
    }

    LOGI("Parsed %d headers", header_count);

    // 检查是否需要处理分块传输编码
    auto transfer_encoding_it = response.headers.find("Transfer-Encoding");
    if (transfer_encoding_it != response.headers.end() &&
        transfer_encoding_it->second.find("chunked") != string::npos) {
        LOGI("Detected chunked transfer encoding, processing...");
        processChunkedBody(response.body);
    }
}

/**
 * 处理分块传输编码的响应体
 * 解析分块传输编码的响应体，将分块数据合并为完整的响应体
 * 支持标准的HTTP分块传输编码格式
 * @param body 分块编码的响应体字符串
 */
void zHttps::processChunkedBody(string& body) {
    LOGI("Processing chunked body, original length: %zu", body.length());
    LOGI("Original body: '%s'", body.c_str());

    string processed_body;
    size_t pos = 0;
    int chunk_count = 0;

    while (pos < body.length()) {
        // 找到块大小行的结束位置
        size_t size_line_end = body.find('\n', pos);
        if (size_line_end == string::npos) {
            LOGE("No newline found for chunk size at position %zu", pos);
            break;
        }

        // 提取块大小行
        string size_line = body.substr(pos, size_line_end - pos);
        if (!size_line.empty() && size_line.back() == '\r') {
            size_line.pop_back();
        }

        LOGI("Chunk %d size line: '%s'", chunk_count + 1, size_line.c_str());

        // 解析块大小（十六进制），处理可能的扩展信息
        size_t semicolon_pos = size_line.find(';');
        string size_str = (semicolon_pos != string::npos) ?
                          size_line.substr(0, semicolon_pos) : size_line;

        size_t chunk_size = 0;
        try {
            chunk_size = stoul(size_str.c_str(), nullptr, 16);
            LOGI("Chunk %d size: %zu", chunk_count + 1, chunk_size);
        } catch (const std::exception& e) {
            LOGE("Failed to parse chunk size '%s': %s", size_str.c_str(), e.what());
            break;
        }

        if (chunk_size == 0) {
            LOGI("Found end chunk (size 0), processing complete");
            break;
        }

        // 移动到块数据开始位置（跳过块大小行和\r\n）
        pos = size_line_end + 1;

        // 检查是否有足够的数据
        if (pos + chunk_size > body.length()) {
            LOGW("Chunk data incomplete: need %zu bytes, have %zu bytes",
                 chunk_size, body.length() - pos);
            break;
        }

        // 提取块数据
        string chunk_data = body.substr(pos, chunk_size);
        LOGI("Chunk %d data: '%s'", chunk_count + 1, chunk_data.c_str());

        // 添加块数据到处理后的主体
        processed_body += chunk_data;

        // 移动到下一个块（跳过块数据和\r\n）
        pos += chunk_size + 2;  // +2 for \r\n after chunk data

        chunk_count++;
    }

    body = processed_body;
    LOGI("Processed chunked body: %d chunks, new length: %zu", chunk_count, body.length());
    LOGI("Final body: '%s'", body.c_str());
}

/**
 * 清理资源
 * 释放所有mbedtls相关的资源
 * 确保没有资源泄漏
 */
void zHttps::cleanup() {
    LOGD("cleanup called");
    if (initialized) {
        // 清理所有mbedtls资源
        mbedtls_net_free(&server_fd);
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&conf);
        mbedtls_x509_crt_free(&cacert);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        initialized = false;
        LOGI("zHttps resources cleaned up");
    }
}

/**
 * 检查是否超时
 * 检查从指定开始时间到现在是否已经超过了超时时间
 * @param start_time 开始时间
 * @param timeout_seconds 超时时间（秒）
 * @return true表示已超时，false表示未超时
 */
bool zHttps::isTimeoutReached(time_t start_time, int timeout_seconds) {
    LOGD("isTimeoutReached called with start_time: %ld, timeout_seconds: %d", start_time, timeout_seconds);
    time_t current_time = time(nullptr);
    bool result = (current_time - start_time) >= timeout_seconds;
    LOGD("isTimeoutReached returning %d", result);
    return result;
}

// Socket连接相关方法实现
/**
 * 带超时的socket连接
 * 建立到指定主机和端口的TCP连接，支持超时控制
 * 使用非阻塞socket和select实现超时机制
 * @param host 主机名
 * @param port 端口号
 * @param timeout_seconds 超时时间（秒）
 * @return socket文件描述符，失败返回-1
 */
int zHttps::connectWithTimeout(const string& host, int port, int timeout_seconds) {
    LOGD("connectWithTimeout called with host: %s, port: %d, timeout_seconds: %d", host.c_str(), port, timeout_seconds);

    // Linux/Android实现
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOGE("Socket creation failed");
        return -1;
    }

    // 设置非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // 解析主机名
    struct hostent *server = gethostbyname(host.c_str());
    if (server == nullptr) {
        LOGE("Failed to resolve hostname: %s", host.c_str());
        close(sockfd);
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    // 尝试连接
    int ret = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret < 0 && errno == EINPROGRESS) {
        // 连接正在进行中，等待完成或超时
        fd_set writefds, exceptfds;
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        FD_SET(sockfd, &writefds);
        FD_SET(sockfd, &exceptfds);

        struct timeval timeout;
        timeout.tv_sec = timeout_seconds;
        timeout.tv_usec = 0;

        ret = select(sockfd + 1, nullptr, &writefds, &exceptfds, &timeout);
        if (ret == 0) {
            LOGE("Connection timeout after %d seconds", timeout_seconds);
            close(sockfd);
            return -1;
        } else if (ret < 0) {
            LOGE("Select failed");
            close(sockfd);
            return -1;
        } else if (FD_ISSET(sockfd, &exceptfds)) {
            LOGE("Connection failed with exception");
            close(sockfd);
            return -1;
        }
    } else if (ret < 0) {
        LOGE("Connection failed");
        close(sockfd);
        return -1;
    }

    // 设置阻塞模式
    fcntl(sockfd, F_SETFL, flags);

    LOGI("Connection established successfully with sockfd: %d", sockfd);
    return sockfd;
}

/**
 * 关闭socket连接
 * 关闭指定的socket文件描述符
 * @param sockfd socket文件描述符
 */
void zHttps::closeSocket(int sockfd) {
    LOGD("closeSocket called with sockfd: %d", sockfd);
    close(sockfd);
}
