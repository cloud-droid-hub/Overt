//
// Created by lxz on 2025/6/16.
//

#ifndef OVERT_PACKAGE_INFO_H
#define OVERT_PACKAGE_INFO_H

#include "zStd.h"

/**
 * 获取包信息
 * 检测系统中安装的应用程序包，识别可疑的调试工具和Root框架
 * 通过多种方式检测应用安装状态，包括PackageManager、文件系统、Shell命令等
 * @return 包含检测结果的Map，格式：{包名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_package_info();

#endif //OVERT_PACKAGE_INFO_H
