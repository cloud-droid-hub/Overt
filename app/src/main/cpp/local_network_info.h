//
// Created by lxz on 2025/8/2.
//

#ifndef OVERT_LOCAL_NETWORK_INFO_H
#define OVERT_LOCAL_NETWORK_INFO_H

#include "config.h"

/**
 * 获取本地网络信息
 * 通过UDP广播检测同一网络中的其他Overt设备
 * 使用广播机制发现网络中的潜在威胁设备
 * @return 包含检测结果的Map，格式：{IP地址 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_local_network_info();

#endif //OVERT_LOCAL_NETWORK_INFO_H
