//
// Created by lxz on 2025/8/2.
//

#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"

#include "local_network_info.h"
#include "zBroadCast.h"

// 存储检测到的本地Overt设备IP地址和消息的Map
map<string, string> local_overt_ip_map = {};

/**
 * UDP消息接收回调函数
 * 当接收到UDP广播消息时被调用
 * @param ip 发送方的IP地址
 * @param msg 接收到的消息内容
 */
void on_receive(const char* ip, const char* msg){
    LOGE("ip:%s msg:%s", ip, msg);
    // 将接收到的IP和消息存储到Map中
    local_overt_ip_map[ip] = msg;
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

    // 处理检测到的Overt设备
    for(auto item : local_overt_ip_map){
        // 将检测到的设备标记为警告级别
        info[item.first]["risk"] = "warn";
        info[item.first]["explain"] = "overt device";
    }
    
    // 清空检测结果，避免重复报告
    local_overt_ip_map.clear();

    return info;
}