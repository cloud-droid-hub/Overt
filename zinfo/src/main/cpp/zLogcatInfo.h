//
// Created by lxz on 2025/8/11.
//

#ifndef OVERT_ZLOGCATINFO_H
#define OVERT_ZLOGCATINFO_H

#include "zStd.h"

/**
 * 获取日志信息
 * 检测系统日志中的可疑记录
 * 主要用于检测Zygisk等Root框架的痕迹
 * @return 包含检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_logcat_info();

#endif //OVERT_ZLOGCATINFO_H
