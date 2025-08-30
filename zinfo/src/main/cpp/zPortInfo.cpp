//
// Created by lxz on 2025/7/17.
//

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>


#include "zFile.h"
#include "zLog.h"
#include "zPortInfo.h"

// 判断端口是否被监听
bool is_port_in_use(int port) {
    LOGD("is_port_in_use: checking port %d", port);
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("is_port_in_use: failed to create socket for port %d, errno: %d", port, errno);
        return false;
    }

    // 设置socket选项，避免地址重用问题
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOGW("is_port_in_use: failed to set SO_REUSEADDR for port %d", port);
    }

    struct sockaddr_in sa = {};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    
    // 使用更安全的地址转换
    if (inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr) <= 0) {
        LOGE("is_port_in_use: failed to convert address for port %d", port);
        close(sock);
        return false;
    }

    // 设置非阻塞
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        LOGE("is_port_in_use: failed to get socket flags for port %d", port);
        close(sock);
        return false;
    }
    
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOGE("is_port_in_use: failed to set non-blocking for port %d", port);
        close(sock);
        return false;
    }
    
    int result = connect(sock, (struct sockaddr*)&sa, sizeof(sa));
    bool in_use = false;

    if (result == 0) {
        // 立即连接成功，说明有监听者
        LOGD("is_port_in_use: port %d is in use (immediate connect)", port);
        in_use = true;
    } else if (errno == EINPROGRESS) {
        // 连接正在建立，select 等待判断是否成功
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        struct timeval tv = {0, 200000}; // 200ms
        result = select(sock + 1, nullptr, &writefds, nullptr, &tv);
        if (result > 0) {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) == 0) {
                if (so_error == 0) {
                    LOGD("is_port_in_use: port %d is in use (select success)", port);
                    in_use = true;
                } else {
                    LOGD("is_port_in_use: port %d is not in use (select error: %d)", port, so_error);
                }
            } else {
                LOGW("is_port_in_use: failed to get socket error for port %d", port);
            }
        } else if (result == 0) {
            LOGD("is_port_in_use: port %d is not in use (select timeout)", port);
        } else {
            LOGW("is_port_in_use: select failed for port %d, errno: %d", port, errno);
        }
    } else {
        LOGD("is_port_in_use: port %d is not in use (connect failed, errno: %d)", port, errno);
    }

    close(sock);
    return in_use;
}


map<string, map<string, string>> get_port_info(){
    LOGI("get_port_info: starting port detection");
    
    map<string, map<string, string>> info;

    map<int, string> tcp_info{
        {27042, "frida"},
        {27043, "frida"},
        {27047, "frida"},
        {23946, "ida"},
    };

    LOGI("get_port_info: checking %zu ports", tcp_info.size());

    for (auto& item : tcp_info) {
        try {
            LOGD("get_port_info: checking port %d for %s", item.first, item.second.c_str());
            
            if (is_port_in_use(item.first)) {
                LOGI("get_port_info: detected suspicious port %d (%s)", item.first, item.second.c_str());
                info[item.second]["risk"] = "error";
                info[item.second]["explain"] = "black port is in use " + item.second;
            } else {
                LOGD("get_port_info: port %d (%s) is not in use", item.first, item.second.c_str());
            }
        } catch (...) {
            LOGE("get_port_info: exception occurred while checking port %d", item.first);
            // 继续检查其他端口，不因为一个端口失败而停止
        }
    }

    LOGI("get_port_info: completed, found %zu suspicious ports", info.size());
    return info;
}