//
// Created by lxz on 2025/7/12.
//

#ifndef OVERT_TASK_INFO_H
#define OVERT_TASK_INFO_H

#include "config.h"

/**
 * 获取任务信息
 * 检测当前进程的所有线程，查找Frida等调试工具注入的痕迹
 * 通过分析/proc/self/task目录下的线程状态信息进行检测
 * @return 包含检测结果的Map，格式：{线程信息 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_task_info();

#endif //OVERT_TASK_INFO_H
