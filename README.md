# Overt - Android设备安全检测工具

[![Android](https://img.shields.io/badge/Android-23+-green.svg)](https://developer.android.com/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build](https://img.shields.io/badge/Build-Gradle-orange.svg)](https://gradle.org/)

Overt是一个功能强大的Android设备安全检测工具，采用Java + Native C++混合架构，提供全面的设备安全状态检测和分析功能。

## 🚀 主要功能

### 🔒 TEE (可信执行环境) 检测
- **设备锁定状态检测**: 检查设备是否处于锁定状态
- **启动验证状态**: 验证系统启动的完整性
- **安全级别评估**: 评估设备的安全防护等级
- **硬件级安全验证**: 基于Android KeyStore的硬件级安全检测

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

### 🔧 动态分析检测
- **Frida检测**: 检测Frida等动态分析工具
- **调试工具检测**: 识别各种调试和分析工具

## 🏗️ 技术架构

### 分层架构
```
┌─────────────────────────────────────┐
│            Java层 (UI层)             │
│  ├── MainActivity                   │
│  ├── DeviceInfoProvider             │
│  └── TEEStatus                      │
├─────────────────────────────────────┤
│            JNI桥接层                 │
├─────────────────────────────────────┤
│          Native C++层                │
│  ├── zDevice (设备信息管理)          │
│  ├── 检测模块                        │
│  │  ├── root_file_info             │
│  │  ├── package_info               │
│  │  ├── system_prop_info           │
│  │  ├── mounts_info                │
│  │  ├── linker_info                │
│  │  ├── maps_info                  │
│  │  ├── class_loader_info          │
│  │  └── time_info                  │
│  └── 工具类                         │
│      ├── zFile                     │
│      ├── zLog                      │
│      └── util                      │
└─────────────────────────────────────┘
```

### 技术特点
- **混合架构**: Java负责UI，C++负责核心检测逻辑
- **模块化设计**: 每个检测功能独立成模块
- **单例模式**: 统一管理设备信息
- **性能优化**: Native层提供高性能检测
- **安全性**: 核心逻辑在Native层，更难被反编译

## 📋 系统要求

- **Android版本**: API 23+ (Android 6.0+)
- **架构支持**: ARM64-v8a
- **权限要求**:
  - `ACCESS_NETWORK_STATE`
  - `QUERY_ALL_PACKAGES`
  - `ACCESS_WIFI_STATE`
  - `READ_PHONE_STATE`
  - `INTERNET`

## 🛠️ 构建说明

### 环境准备
1. **Android Studio**: 最新版本
2. **Android SDK**: API 34
3. **NDK**: 支持CMake 3.22.1+
4. **Gradle**: 8.0+

### 构建步骤
```bash
# 克隆项目
git clone [repository-url]
cd Overt

# 构建项目
./gradlew assembleDebug

# 或者使用Android Studio
# 1. 打开项目
# 2. 等待Gradle同步完成
# 3. 点击运行按钮
```

### 依赖库
- **AndroidX**: AppCompat, Material Design, ConstraintLayout
- **BouncyCastle**: 加密库 (bcprov-jdk15on:1.70)
- **Native库**: Android NDK, Log库

## 📱 使用方法

### 安装应用
1. 下载并安装APK文件
2. 授予必要权限
3. 启动应用

### 查看检测结果
应用启动后会自动执行所有检测模块，结果以卡片形式展示：

- **红色卡片**: 高风险项目 (error级别)
- **黄色卡片**: 中风险项目 (warn级别)
- **绿色卡片**: 正常项目

每个检测项包含：
- 检测项目名称
- 风险等级
- 详细说明

## 🔧 开发指南

### 添加新的检测模块

1. **创建Native检测函数**
```cpp
// 在cpp目录下创建新文件
// example_detection.h
std::map<std::string, std::map<std::string, std::string>> get_example_detection();

// example_detection.cpp
std::map<std::string, std::map<std::string, std::string>> get_example_detection() {
    std::map<std::string, std::map<std::string, std::string>> info;
    // 实现检测逻辑
    return info;
}
```

2. **在CMakeLists.txt中添加源文件**
```cmake
add_library(${CMAKE_PROJECT_NAME} SHARED
    # ... 其他文件
    example_detection.cpp
)
```

3. **在native-lib.cpp中调用**
```cpp
// 在init_函数中添加
zDevice::getInstance()->get_device_info()["example_detection"] = get_example_detection();
```

### 自定义UI显示
修改 `DeviceInfoProvider.java` 中的 `addInfoCard` 方法来调整显示效果。

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

## 🤝 贡献指南

1. Fork 项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## ⚠️ 免责声明

本工具仅用于安全研究和教育目的。使用者需要遵守当地法律法规，不得用于非法用途。开发者不承担因使用本工具而产生的任何法律责任。

## 📞 联系方式

如有问题或建议，请通过以下方式联系：

- 提交 Issue
- 发送邮件至 [your-email@example.com]
- 项目主页: [repository-url]

---

**注意**: 本工具需要root权限才能进行某些检测。在非root设备上，部分检测功能可能无法正常工作。 