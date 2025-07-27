# Overt - Android设备安全检测工具

[![Android](https://img.shields.io/badge/Android-23+-green.svg)](https://developer.android.com/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build](https://img.shields.io/badge/Build-Gradle-orange.svg)](https://gradle.org/)
[![NDK](https://img.shields.io/badge/NDK-CMake%203.22.1+-yellow.svg)](https://developer.android.com/ndk)

Overt是一个功能强大的Android设备安全检测工具，采用Java + Native C++混合架构，提供全面的设备安全状态检测和分析功能。项目集成了先进的TEE（可信执行环境）证书解析技术，能够深度分析Android设备的安全状态。

## 🚀 主要功能

### 🔒 TEE (可信执行环境) 检测
- **设备锁定状态检测**: 检查设备是否处于锁定状态
- **启动验证状态**: 验证系统启动的完整性
  - Verified: 已验证
  - Self-signed: 自签名
  - Unverified: 未验证
  - Failed: 验证失败
- **安全级别评估**: 评估设备的安全防护等级
- **硬件级安全验证**: 基于Android KeyStore的硬件级安全检测
- **证书链解析**: 深度解析X.509证书中的TEE扩展信息
- **RootOfTrust验证**: 验证可信根信息

### 🔍 Root检测
- **Root文件检测**: 检测常见的root工具文件
  - `/sbin/su`
  - `/system/bin/su`
  - `/system/xbin/su`
  - `/data/local/xbin/su`
  - 等多个路径
- **Magisk检测**: 检测Magisk等root管理工具

### 📱 应用安全检测
- **黑名单应用检测**: 检测危险应用
  - Hunter (反调试工具)
  - MT管理器
  - 抓包帮手
  - 无线ADB工具
  - 快应用调试器
  - Apatch
  - LSPosed
  - KernelSU
  - 等
- **白名单应用检测**: 检查必要应用是否安装
  - QQ、微信、支付宝
  - 抖音等常用应用

### 🛡️ 系统安全检测
- **系统属性检测**: 检查系统关键属性
- **挂载点信息**: 分析文件系统挂载状态
- **链接器信息**: 检测动态链接库加载
- **内存映射检测**: 分析进程内存映射
- **类加载器检测**: 监控Java类加载情况
- **时间信息检测**: 检查系统时间异常
- **端口信息检测**: 检测网络端口使用情况
- **任务信息检测**: 分析系统进程状态

### 🔧 动态分析检测
- **Frida检测**: 检测Frida等动态分析工具
- **调试工具检测**: 识别各种调试和分析工具

### ⚙️ 系统设置检测
- **开发者模式检测**: 检查是否启用开发者模式
- **USB调试检测**: 检查USB调试是否启用
- **代理设置检测**: 检查网络代理配置
- **VPN检测**: 检查VPN连接状态
- **锁屏密码检测**: 检查设备锁屏设置
- **充电状态检测**: 检查设备充电状态
- **SIM卡检测**: 检查SIM卡状态
- **安装来源检测**: 检查应用安装来源

## 🏗️ 技术架构

### 分层架构
```
┌─────────────────────────────────────┐
│            Java层 (UI层)             │
│  ├── MainActivity                   │
│  ├── InfoCardContainer              │
│  ├── InfoCard                       │
│  └── MainApplication                │
├─────────────────────────────────────┤
│            JNI桥接层                 │
│  ├── native-lib.cpp                 │
│  └── JNI接口函数                    │
├─────────────────────────────────────┤
│          Native C++层                │
│  ├── zDevice (设备信息管理)          │
│  ├── TEE证书解析模块                 │
│  │  ├── tee_cert_parser.cpp        │
│  │  ├── tee_cert_parser.h          │
│  │  └── tee_info.cpp               │
│  ├── 检测模块                        │
│  │  ├── root_file_info             │
│  │  ├── package_info               │
│  │  ├── system_prop_info           │
│  │  ├── mounts_info                │
│  │  ├── linker_info                │
│  │  ├── maps_info                  │
│  │  ├── class_loader_info          │
│  │  ├── time_info                  │
│  │  ├── port_info                  │
│  │  ├── task_info                  │
│  │  └── system_setting_info        │
│  └── 工具类                         │
│      ├── zFile                     │
│      ├── zLog                      │
│      ├── zJavaVm                   │
│      ├── zClassLoader              │
│      ├── zLinker                   │
│      ├── zElf                      │
│      ├── zUtil                     │
│      └── crc                       │
└─────────────────────────────────────┘
```

### 技术特点
- **混合架构**: Java负责UI，C++负责核心检测逻辑
- **模块化设计**: 每个检测功能独立成模块
- **单例模式**: 统一管理设备信息
- **性能优化**: Native层提供高性能检测
- **安全性**: 核心逻辑在Native层，更难被反编译
- **TEE深度解析**: 基于mbedTLS的ASN.1解析技术
- **证书链验证**: 完整的X.509证书解析流程



### 依赖库

- **AndroidX**: AppCompat, Material Design, ConstraintLayout
- **mbedTLS**: 加密库 (libmbedtls.a, libmbedx509.a, libmbedcrypto.a)
- **Native库**: Android NDK, Log库



## 📊 检测项目说明

### TEE检测
- **DeviceLocked**: 设备锁定状态
- **VerifiedBootState**: 启动验证状态
  - Verified: 已验证
  - Self-signed: 自签名
  - Unverified: 未验证
  - Failed: 验证失败

### Root检测
检测常见的root工具文件，如果存在则标记为高风险。

### 应用检测
- **黑名单应用**: 检测危险工具应用
- **白名单应用**: 检查必要应用是否缺失

### 系统设置检测
- **开发者模式**: 检查是否启用开发者选项
- **USB调试**: 检查USB调试是否启用
- **代理设置**: 检查网络代理配置
- **VPN状态**: 检查VPN连接状态
- **锁屏密码**: 检查设备锁屏设置
- **充电状态**: 检查设备充电状态
- **SIM卡状态**: 检查SIM卡是否存在
- **安装来源**: 检查应用安装来源

## 🔬 TEE证书解析技术

### 技术原理
项目实现了基于mbedTLS的TEE证书解析技术，能够：

1. **证书获取**: 通过Android KeyStore获取TEE证书
2. **ASN.1解析**: 解析X.509证书中的TEE扩展
3. **RootOfTrust提取**: 提取可信根信息
4. **安全状态验证**: 验证设备安全状态

### 核心组件
- **tee_cert_parser.cpp**: C++实现的证书解析器
- **tee_info.cpp**: TEE信息获取和处理

### 解析流程
```
1. 生成TEE密钥对
   ↓
2. 获取证书链
   ↓
3. 提取TEE扩展 (OID: 1.3.6.1.4.1.11129.2.1.17)
   ↓
4. 解析ASN.1结构
   ↓
5. 提取RootOfTrust信息
   ↓
6. 验证安全状态
```
