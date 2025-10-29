//
// Created by lxz on 2025/7/17.
//

#ifndef OVERT_PORT_INFO_H
#define OVERT_PORT_INFO_H

#include "zStd.h"

/**
 * 获取端口信息
 * 检测系统中可疑端口的使用情况
 * 主要用于检测Frida、IDA等调试工具的端口监听
 * @return 包含检测结果的Map，格式：{工具名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_port_info();

#endif //OVERT_PORT_INFO_H
