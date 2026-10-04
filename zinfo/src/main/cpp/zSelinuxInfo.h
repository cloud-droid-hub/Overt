//
// Created by lxz on 2025/8/11.
//

#ifndef OVERT_ZSELINUXINFO_H
#define OVERT_ZSELINUXINFO_H

#include "zStd.h"

/**
 * 获取 SELinux 检测信息(查杀分离 — 采集端)
 * 统一采集 SELinux 维度的可疑信号：
 *   1. 进程 SELinux context(/proc/self/attr/current)——含 magisk/su 域标记说明被提权
 *   2. logcat 中 zygisk 痕迹(u:r:su:s0 审计日志)——zygisk 注入的 SELinux 侧证据
 * 只上报原始数据，不做风险判定；判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{"selinux_context" -> {value: 上下文}} / {"logcat_record" -> {value: 命中行}}
 */
map<string, map<string, string>> get_selinux_info();

#endif //OVERT_ZSELINUXINFO_H