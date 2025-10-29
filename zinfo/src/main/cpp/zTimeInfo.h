//
// Created by lxz on 2025/6/12.
//

#ifndef OVERT_TIME_INFO_H
#define OVERT_TIME_INFO_H

#include "zStd.h"

/**
 * 获取时间信息
 * 检测系统时间、启动时间等时间相关信息
 * 主要用于检测时间篡改、系统重启等异常情况
 * @return 包含检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_time_info();

#endif //OVERT_TIME_INFO_H
