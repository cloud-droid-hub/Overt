//
// Created by lxz on 2025/8/11.
//

#ifndef OVERT_ZSHELL_H
#define OVERT_ZSHELL_H
#include "zStd.h"

/**
 * 执行Shell命令
 * 在Android系统中执行指定的Shell命令并返回输出结果
 * 使用fork和execve系统调用实现安全的命令执行
 * 主要用于检测系统中安装的应用程序和系统状态
 * @param cmd 要执行的Shell命令
 * @return 命令执行的输出结果字符串
 */
string runShell(string cmd);

#endif //OVERT_ZSHELL_H
