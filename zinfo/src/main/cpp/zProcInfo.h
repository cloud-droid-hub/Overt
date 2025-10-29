//
// Created by lxz on 2025/8/24.
//

#ifndef OVERT_ZPROCINFO_H
#define OVERT_ZPROCINFO_H

#include "zStd.h"

/**
 * 获取进程信息
 * 检测当前进程的各种状态信息，包括内存映射、挂载点、任务状态等
 * 主要用于检测Frida、IDA等调试工具的注入痕迹
 * @return 包含检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_proc_info();

#endif //OVERT_ZPROCINFO_H
