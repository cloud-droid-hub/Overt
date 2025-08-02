//
// Created by lxz on 2025/8/2.
//

#include "local_network_info.h"
#include "zBroadCast.h"

map<string, string> local_overt_ip_map = {};

void on_receive(const char* ip, const char* msg){
    LOGE("ip:%s msg:%s", ip, msg);
    local_overt_ip_map[ip] = msg;
}

map<string, map<string, string>> get_local_network_info(){
    map<string, map<string, string>> info;

    zBroadCast::getInstance()->start_local_ip_monitor();
    zBroadCast::getInstance()->start_udp_broadcast_sender( 7476, "overt");
    zBroadCast::getInstance()->start_udp_broadcast_listener(7476, on_receive);

    for(auto item : local_overt_ip_map){
        info[item.first]["risk"] = "warn";
        info[item.first]["explain"] = "overt device";
    }
    local_overt_ip_map.clear();

    return info;
}