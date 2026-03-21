//
// Created by lxz on 2025/8/2.
//

#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include <arpa/inet.h>
#include <chrono>
#include <mutex>

#include "zLocalNetworkInfo.h"
#include "zBroadCast.h"

// 存储检测到的本地Overt设备IP地址和最后一次出现时间
static map<string, std::chrono::steady_clock::time_point> local_overt_ip_map = {};
static std::mutex local_overt_ip_map_mtx;
static constexpr int LOCAL_OVERT_TTL_SECONDS = 15;
static constexpr size_t LOCAL_OVERT_MAX_TRACKED_IPS = 512;

static bool is_valid_ipv4(const char* ip) {
    if (ip == nullptr || ip[0] == '\0') {
        return false;
    }
    struct in_addr addr {};
    return inet_pton(AF_INET, ip, &addr) == 1;
}

static void cleanup_expired_locked(const std::chrono::steady_clock::time_point& now) {
    for (auto it = local_overt_ip_map.begin(); it != local_overt_ip_map.end();) {
        auto diff_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
        if (diff_seconds > LOCAL_OVERT_TTL_SECONDS) {
            it = local_overt_ip_map.erase(it);
            continue;
        }
        ++it;
    }
}

static bool evict_oldest_locked() {
    if (local_overt_ip_map.empty()) {
        return false;
    }
    auto oldest = local_overt_ip_map.begin();
    for (auto it = local_overt_ip_map.begin(); it != local_overt_ip_map.end(); ++it) {
        if (it->second < oldest->second) {
            oldest = it;
        }
    }
    LOGW("evict oldest tracked ip: %s", oldest->first.c_str());
    local_overt_ip_map.erase(oldest);
    return true;
}

/**
 * UDP消息接收回调函数
 * 当接收到UDP广播消息时被调用
 * @param ip 发送方的IP地址
 * @param msg 接收到的消息内容
 */
static void on_receive(const char* ip, const char* msg){
    if (ip == nullptr || msg == nullptr) {
        return;
    }
    if (!is_valid_ipv4(ip)) {
        LOGW("ignore invalid ipv4 sender: %s", ip ? ip : "null");
        return;
    }
    if (strcmp(msg, "overt") != 0) {
        LOGD("ignore non-overt message from %s: %s", ip, msg);
        return;
    }
    LOGD("receive overt broadcast from ip:%s msg:%s", ip, msg);
    // 记录最后一次收到该设备广播的时间
    std::lock_guard<std::mutex> lock(local_overt_ip_map_mtx);
    auto now = std::chrono::steady_clock::now();
    cleanup_expired_locked(now);

    string ip_str = ip;
    auto existed = local_overt_ip_map.find(ip_str);
    if (existed == local_overt_ip_map.end() && local_overt_ip_map.size() >= LOCAL_OVERT_MAX_TRACKED_IPS) {
        if (!evict_oldest_locked()) {
            LOGW("local overt ip map is full(%zu), drop new ip: %s", local_overt_ip_map.size(), ip);
            return;
        }
    }
    local_overt_ip_map[ip_str] = now;
}

/**
 * 获取本地网络信息
 * 通过UDP广播机制检测同一网络中的其他Overt设备
 * 使用端口7476进行广播通信
 * @return 包含检测结果的Map，格式：{IP地址 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_local_network_info(){
    map<string, map<string, string>> info;

    // 启动本地IP监控
    zBroadCast::getInstance()->start_local_ip_monitor();
    
    // 启动UDP广播发送器，向端口7476发送"overt"消息
    zBroadCast::getInstance()->start_udp_broadcast_sender( 7476, "overt");
    
    // 启动UDP广播监听器，监听端口7476，使用on_receive回调处理接收到的消息
    zBroadCast::getInstance()->start_udp_broadcast_listener(7476, on_receive);

    // 按TTL收集仍然活跃的设备，避免单次丢包导致结果抖动
    vector<string> active_ips;
    {
        std::lock_guard<std::mutex> lock(local_overt_ip_map_mtx);
        auto now = std::chrono::steady_clock::now();
        cleanup_expired_locked(now);
        for (const auto& item : local_overt_ip_map) {
            active_ips.push_back(item.first);
        }
    }

    // 处理检测到的Overt设备
    for (const auto& ip : active_ips) {
        // 将检测到的设备标记为警告级别
        info[ip]["risk"] = "warn";
        info[ip]["explain"] = "overt device";
    }

    return info;
}
