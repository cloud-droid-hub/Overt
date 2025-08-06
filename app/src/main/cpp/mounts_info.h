//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_MOUNTS_INFO_H
#define OVERT_MOUNTS_INFO_H

#include "zStd.h"

/**
 * 获取挂载点信息
 * 检测系统中异常的挂载点，如dex2oat、APatch、shamiko等
 * 这些挂载点通常与Root工具或系统修改相关
 * @return 包含检测结果的Map，格式：{挂载信息 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_mounts_info();

#endif //OVERT_MOUNTS_INFO_H
