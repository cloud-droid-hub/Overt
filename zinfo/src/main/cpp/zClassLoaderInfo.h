//
// Created by lxz on 2025/7/3.
//

#ifndef OVERT_ZCLASSLOADERINFO_H
#define OVERT_ZCLASSLOADERINFO_H

#include "zStd.h"

/**
 * 获取类加载器信息
 * 分析系统中所有类加载器，检测可疑的类加载器类型
 * 主要用于检测LSPosed、Xposed等框架注入的类加载器
 * @return 包含类加载器风险信息的映射表
 */
map<string, map<string, string>> get_class_loader_info();

/**
 * 获取类信息
 * 分析系统中所有已加载的类，检测可疑的类名
 * 主要用于检测LSPosed、Xposed等框架相关的类
 * @return 包含类风险信息的映射表
 */
map<string, map<string, string>> get_class_info();

#endif //OVERT_ZCLASSLOADERINFO_H
