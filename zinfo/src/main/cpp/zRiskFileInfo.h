//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_RISK_FILE_INFO_H
#define OVERT_RISK_FILE_INFO_H

#include "zStd.h"

/**
 * 获取风险文件信息(查杀分离 — 采集端)
 * 统一探测多类风险特征文件(root 特征、模拟器特征等)的存在状态，
 * 只上报 {路径 -> {value: "1"存在 / "0"不存在}}，不掺任何类别/判定语义；
 * 哪个路径属于哪类风险、命中判什么等级，完全由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{文件路径 -> {value: "1"/"0"}}
 */
map<string, map<string, string>> get_risk_file_info();

#endif //OVERT_RISK_FILE_INFO_H