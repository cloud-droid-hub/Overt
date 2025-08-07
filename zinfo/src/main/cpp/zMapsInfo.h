//
// Created by lxz on 2025/7/11.
//

#ifndef OVERT_MAPS_INFO_H
#define OVERT_MAPS_INFO_H

#include "zStd.h"

/**
 * 获取内存映射信息
 * 分析/proc/self/maps文件，检测关键系统库是否被篡改
 * 主要检测libart.so和libc.so等关键库的映射数量和权限是否正确
 * @return 包含检测结果的Map，格式：{库名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_maps_info();

#endif //OVERT_MAPS_INFO_H
