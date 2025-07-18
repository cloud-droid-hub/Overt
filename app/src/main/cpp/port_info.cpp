//
// Created by lxz on 2025/7/17.
//

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <android/log.h>
#include "port_info.h"
#include "zFile.h"
#include "zLog.h"

// 判断端口是否被监听
bool is_port_in_use(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    struct sockaddr_in sa = {};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    // 设置非阻塞
    fcntl(sock, F_SETFL, O_NONBLOCK);
    int result = connect(sock, (struct sockaddr*)&sa, sizeof(sa));

    bool in_use = false;

    if (result == 0) {
        // 立即连接成功，说明有监听者
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
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error == 0) {
                in_use = true;
            }
        }
    }

    close(sock);
    return in_use;
}

std::map<std::string, std::map<std::string, std::string>> get_port_info(){

    std::map<std::string, std::map<std::string, std::string>> info;

    std::map<int, std::string> tcp_info{
        {27042, "frida"},
        {27043, "frida"},
        {23946, "ida"},
    };

    for (auto& item : tcp_info) {
        if (is_port_in_use(item.first)) {
            info[std::to_string(item.first)]["risk"] = "error";
            info[std::to_string(item.first)]["explain"] = "black port is in use " + item.second;
        }
    }

    return info;
}