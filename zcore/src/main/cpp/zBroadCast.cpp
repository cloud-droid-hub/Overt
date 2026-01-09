//
// Created by lxz on 2025/8/2.
//

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <pthread.h>
#include <net/if.h>
#include <netinet/in.h>

#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStdUtil.h"
#include "zBroadCast.h"
#include <mutex>

#define BROAD_CAST_SLEEP_TIME 3

zBroadCast* zBroadCast::instance = nullptr;

/**
 * 获取单例实例
 * 采用线程安全的懒加载模式，首次调用时创建实例
 * @return zBroadCast单例指针
 */
zBroadCast* zBroadCast::getInstance() {
    // 使用 std::call_once 确保线程安全的单例初始化
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        try {
            instance = new zBroadCast();
            LOGI("zBroadCast: Created singleton instance");
        } catch (const std::exception& e) {
            LOGE("zBroadCast: Failed to create singleton instance: %s", e.what());
        } catch (...) {
            LOGE("zBroadCast: Failed to create singleton instance with unknown error");
        }
    });

    return instance;
}

// 构造函数实现
zBroadCast::zBroadCast() {
    LOGI("zBroadCast constructor called - initializing broadcast system");
    // 初始化代码可以在这里添加
    LOGD("zBroadCast constructor completed successfully");
}

// 析构函数实现
zBroadCast::~zBroadCast() {
    LOGW("zBroadCast destructor called - cleaning up broadcast system");
    // 清理代码可以在这里添加
    LOGI("zBroadCast destructor completed");
}

// 监控局域网 ip 的线程相关函数
string get_local_ip() {
    LOGD("get_local_ip called - attempting to get local IP address");
    static char ip[INET_ADDRSTRLEN] = {0};
    struct ifconf ifc;
    struct ifreq ifr[10]; // 最多支持10个接口

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        LOGE("Failed to create socket for getting local IP");
        return NULL;
    }
    LOGD("Socket created for IP detection, fd: %d", sock);

    ifc.ifc_len = sizeof(ifr);
    ifc.ifc_req = ifr;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        LOGE("Failed to get interface configuration (errno: %d)", errno);
        close(sock);
        return NULL;
    }
    LOGD("Interface configuration retrieved successfully");

    for (int i = 0; i < ifc.ifc_len / sizeof(struct ifreq); i++) {
        struct ifreq* item = &ifr[i];
        if (strcmp(item->ifr_name, "lo") == 0) {
            LOGD("Skipping loopback interface: %s", item->ifr_name);
            continue; // 跳过回环接口
        }

        struct sockaddr_in* addr = (struct sockaddr_in*)&item->ifr_addr;
        if (addr->sin_family == AF_INET) {
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
            LOGI("Found local IP address: %s on interface: %s", ip, item->ifr_name);
            break;
        }
    }

    close(sock);
    LOGD("Socket %d closed after IP detection", sock);

    if (ip[0]) {
        LOGI("Returning local IP: %s", ip);
        return ip;
    } else {
        LOGW("No valid local IP address found");
        return "";
    }
}

string get_local_ip_c(string local_ip){

    if(local_ip.empty()) return "";

    vector<string> local_ip_split = split_str(local_ip, ".");

    if(local_ip_split.size() != 4) return "";

    return local_ip_split[0] + "." + local_ip_split[1] + "." +local_ip_split[2] + ".255";
}

void* local_ip_monitor_thread(void* args) {
    LOGI("local_ip_monitor_thread started");
    LOGD("Local IP monitor thread running with args: %p", args);

    while (true){
        string local_ip = get_local_ip();
        LOGD("Local IP monitor - current IP: %s", local_ip.c_str());

        if(local_ip != zBroadCast::getInstance()->get_local_ip()){
            LOGI("Local IP changed from %s to %s",
                 zBroadCast::getInstance()->get_local_ip().c_str(),
                 local_ip.c_str());

            zBroadCast::getInstance()->set_local_ip(local_ip);
            zBroadCast::getInstance()->set_local_ip_c(get_local_ip_c(local_ip));
            zBroadCast::getInstance()->restart_udp_broadcast_listener();
        } else {
            LOGD("Local IP unchanged: %s", local_ip.c_str());
        }

        LOGD("Local IP monitor - sleeping for 5 seconds");
        sleep(BROAD_CAST_SLEEP_TIME);
    }
    LOGW("local_ip_monitor_thread exiting");
    pthread_exit(nullptr);
}

void zBroadCast::start_local_ip_monitor(){
    LOGI("start_local_ip_monitor called");
    if (local_ip_monitor_tid != 0){
        LOGW("UDP broadcast local_ip_monitor already running (tid: %lu)", local_ip_monitor_tid);
        return;
    }
    if (pthread_create(&local_ip_monitor_tid, nullptr, local_ip_monitor_thread, nullptr) != 0) {
        LOGE("Failed to create local IP monitor thread");
    } else {
        LOGI("Local IP monitor thread created successfully");
    }
}

void zBroadCast::set_local_ip(string local_ip){
    std::shared_lock<std::shared_mutex> lock(local_ip_mutex);
    this->local_ip = local_ip;
}

string zBroadCast::get_local_ip(){
    return local_ip;
}

void zBroadCast::set_local_ip_c(string local_ip_c){
    std::shared_lock<std::shared_mutex> lock(local_ip_c_mutex);
    this->local_ip_c = local_ip_c;
}

string zBroadCast::get_local_ip_c(){
    return local_ip_c;
}


// 发送局域网广播线程的相关函数
void* send_udp_broadcast_thread(void* args) {
    LOGI("send_udp_broadcast_thread started");
    LOGD("Broadcast sender thread running with args: %p", args);

    while (true){
        LOGD("Broadcast sender thread - sending broadcast message");
        zBroadCast::getInstance()->send_udp_broadcast();
        LOGD("Broadcast sender thread - sleeping for 5 seconds");
        sleep(BROAD_CAST_SLEEP_TIME);
    }
    LOGW("send_udp_broadcast_thread exiting");
    pthread_exit(nullptr);
}
void zBroadCast::send_udp_broadcast(int port, string message) {
    LOGI("send_udp_broadcast called - message: '%s', port: %d", message.c_str(), port);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        LOGE("Failed to create socket for UDP broadcast");
        return;
    }
    LOGD("UDP socket created successfully, fd: %d", sock);

    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        LOGE("Failed to set broadcast option on socket %d", sock);
        close(sock);
        return;
    }
    LOGD("Broadcast option set successfully on socket %d", sock);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // 使用更通用的广播地址
    addr.sin_addr.s_addr = inet_addr(get_local_ip_c().c_str());
    LOGD("Target address configured - IP: %s, Port: %d", get_local_ip_c().c_str(), port);

    ssize_t sent = sendto(sock, message.c_str(), strlen(message.c_str()), 0, (struct sockaddr*)&addr, sizeof(addr));
    if (sent < 0) {
        LOGE("Failed to send broadcast message: %s (errno: %d)", message.c_str(), errno);
    } else {
        LOGI("Broadcast message sent successfully: '%s' (%zd bytes)", message.c_str(), sent);
    }

    close(sock);
    LOGD("UDP socket %d closed", sock);
}
void zBroadCast::send_udp_broadcast() {
    LOGI("send_udp_broadcast is called");
    send_udp_broadcast(sender_thread_args.port, sender_thread_args.msg.c_str());
}
void zBroadCast::start_udp_broadcast_sender(int port, string msg) {
    LOGI("start_udp_broadcast_sender called - msg: '%s', port: %d", msg.c_str(), port);
    this->msg = msg;
    this->port = port;
    start_udp_broadcast_sender();
}
void zBroadCast::start_udp_broadcast_sender() {
    LOGI("start_udp_broadcast_sender (internal) called");
    if (sender_tid != 0){
        LOGW("UDP broadcast sender already running (tid: %lu)", sender_tid);
        return;
    }
    set_sender_thread_args(port, msg, nullptr);
    LOGD("Sender thread args set - port: %d, msg: '%s'", port, msg.c_str());

    if (pthread_create(&sender_tid, nullptr, send_udp_broadcast_thread, nullptr) != 0) {
        LOGE("Failed to create UDP broadcast sender thread");
    } else {
        LOGI("UDP broadcast sender thread created successfully (tid: %lu)", sender_tid);
    }
}
void zBroadCast::set_sender_thread_args(int port, string msg, void (*on_receive)(const char* ip, const char* msg)){
    std::shared_lock<std::shared_mutex> lock(sender_thread_args_mutex);
    sender_thread_args = {port, msg, on_receive};
}


// 监听局域网广播线程的相关函数
void* listen_udp_broadcast_thread(void* args) {
    LOGI("listen_udp_broadcast_thread started");
    LOGD("Broadcast listener thread running with args: %p", args);

    while (true){
        LOGD("Broadcast listener thread - starting listener");
        zBroadCast::getInstance()->listen_udp_broadcast();
        LOGD("Broadcast listener thread - sleeping for 5 seconds");
        sleep(BROAD_CAST_SLEEP_TIME);
    }
    LOGW("listen_udp_broadcast_thread exiting");
    pthread_exit(nullptr);
}
void zBroadCast::listen_udp_broadcast(int port, void (*on_receive)(const char* ip, const char* msg)) {
    LOGI("listen_udp_broadcast called - port: %d", port);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        LOGE("Failed to create socket for UDP listening");
        return;
    }
    LOGD("UDP listener socket created successfully, fd: %d", sock);

    // 设置地址重用
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        LOGW("Failed to set SO_REUSEADDR option on socket %d", sock);
    } else {
        LOGD("SO_REUSEADDR option set successfully on socket %d", sock);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    LOGD("Listener address configured - IP: INADDR_ANY, Port: %d", port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOGE("Failed to bind socket %d to port %d (errno: %d)", sock, port, errno);
        close(sock);
        return;
    }
    LOGI("UDP listener socket %d bound successfully to port %d", sock, port);

    char buffer[512];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    LOGI("Starting UDP broadcast listener loop on port %d", port);
    while (1) {
        int len = recvfrom(sock, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&sender, &sender_len);
        if (len > 0) {
            buffer[len] = '\0';
            const char* sender_ip = inet_ntoa(sender.sin_addr);
            LOGI("Received UDP message from %s: '%s' (%d bytes)", sender_ip, buffer, len);

            if(strcmp(buffer, "stop") == 0){
                LOGW("Received stop command from %s, exiting listener loop", sender_ip);
                break;
            }

            if(strcmp(sender_ip, this->get_local_ip().c_str()) == 0){
                LOGD("Ignoring message from local IP %s", sender_ip);
            }else{
                LOGI("Processing message from remote IP %s", sender_ip);
                if (on_receive) {
                    on_receive(sender_ip, buffer);
                }
            }
        } else if (len < 0) {
            LOGE("recvfrom failed on socket %d (errno: %d)", sock, errno);
            break;
        }
    }

    close(sock);
    LOGI("UDP listener socket %d closed", sock);
}
void zBroadCast::listen_udp_broadcast() {
    LOGI("listen_udp_broadcast is called");
    listen_udp_broadcast(listener_thread_args.port, listener_thread_args.on_receive);
}
void zBroadCast::start_udp_broadcast_listener(int port, void (*on_receive)(const char* ip, const char* msg)) {
    LOGI("start_udp_broadcast_listener called - port: %d", port);
    this->port = port;
    this->on_receive = on_receive;
    start_udp_broadcast_listener();
}
void zBroadCast::start_udp_broadcast_listener() {
    LOGI("start_udp_broadcast_listener (internal) called");
    if (listener_tid != 0){
        LOGW("UDP broadcast listener already running (tid: %lu)", listener_tid);
        return;
    }
    set_listener_thread_args(port, "overt", on_receive);
    LOGD("Listener thread args set - port: %d", port);

    if (pthread_create(&listener_tid, nullptr, listen_udp_broadcast_thread, nullptr) != 0) {
        LOGE("Failed to create UDP broadcast listener thread");
    } else {
        LOGI("UDP broadcast listener thread created successfully (tid: %lu)", listener_tid);
    }
}

void zBroadCast::restart_udp_broadcast_listener() {
    LOGI("restart_udp_broadcast_listener called");
    set_stop_thread_args(port, "stop", nullptr);
    LOGD("Sending stop command to restart listener");
    zBroadCast::getInstance()->send_udp_broadcast(stop_thread_args.port, stop_thread_args.msg.c_str());
}
void zBroadCast::set_stop_thread_args(int port, string msg, void (*on_receive)(const char* ip, const char* msg)){
    std::shared_lock<std::shared_mutex> lock(stop_thread_args_mutex);
    stop_thread_args = {port, msg, on_receive};
}
void zBroadCast::set_listener_thread_args(int port, string msg, void (*on_receive)(const char* ip, const char* msg)) {
    std::shared_lock<std::shared_mutex> lock(listener_thread_args_mutex);
    listener_thread_args = {port, msg, on_receive};
}


