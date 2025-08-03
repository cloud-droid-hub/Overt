//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_ROOT_FILE_INFO_H
#define OVERT_ROOT_FILE_INFO_H

#include "config.h"

/**
 * 获取Root文件信息
 * 检测系统中常见的Root相关文件，如su、mu等
 * 这些文件的存在通常表明设备已被Root
 * @return 包含检测结果的Map，格式：{文件路径 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_root_file_info();

#endif //OVERT_ROOT_FILE_INFO_H
