//
// Created by lxz on 2025/7/27.
//

#ifndef SSLCHECK_SSL_INFO_H
#define SSLCHECK_SSL_INFO_H

#include "zStd.h"

/**
 * 获取SSL信息
 * 检测HTTPS连接的SSL证书指纹，验证网络通信的安全性
 * 通过对比预定义的证书指纹，检测是否存在中间人攻击或证书伪造
 * @return 包含检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_ssl_info();

#endif //SSLCHECK_SSL_INFO_H
