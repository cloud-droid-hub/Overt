//
// Created by liuxi on 2025/10/29.
//

#ifndef OVERT_ZSIDECHANNELINFO_H
#define OVERT_ZSIDECHANNELINFO_H

#include "zStd.h"

/**
 * 获取侧信道信息
 * 通过侧信道攻击检测系统环境异常
 * 通过比较不同系统调用的执行时间来判断是否存在调试工具或Hook框架
 * @return 包含检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_side_channel_info();

#endif //OVERT_ZSIDECHANNELINFO_H
