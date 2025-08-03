//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_SYSTEM_PROP_INFO_H
#define OVERT_SYSTEM_PROP_INFO_H

#include "config.h"

/**
 * 获取系统属性信息
 * 检测关键系统属性的值是否正确，如ro.secure、ro.debuggable等
 * 这些属性的异常值通常表明系统已被修改或Root
 * @return 包含检测结果的Map，格式：{属性名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_system_prop_info();

#endif //OVERT_SYSTEM_PROP_INFO_H
