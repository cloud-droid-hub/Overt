# Overt - Android设备安全检测工具

## 项目概述

Overt是一个Android设备安全检测工具，通过收集和分析各种系统信息来检测设备是否被Root、被调试工具注入或存在其他安全风险。项目采用模块化架构设计，将不同功能分散到独立的模块中，便于维护和扩展。

## 项目架构

### 模块化结构

项目采用多模块架构，包含以下模块：

#### 1. 主应用模块 (app)
- **功能**: 主应用程序入口，提供用户界面和交互
- **包名**: `com.example.overt`
- **主要组件**: MainActivity、InfoCard、InfoCardContainer
- **Native库**: native-lib、zManager

#### 2. 核心模块 (zcore)
- **功能**: 核心功能库，提供基础工具类和系统接口
- **包名**: `com.example.zcore`
- **主要组件**: 
  - 文件操作: zFile
  - 网络通信: zHttps
  - JSON处理: zJson
  - 线程管理: zThread
  - 广播处理: zBroadCast
  - 类加载器: zClassLoader
  - 链接器: zLinker
  - CRC校验: zCrc32
  - ELF文件处理: zElf
  - TEE接口: zTee
  - JVM接口: zJavaVm

#### 3. 信息收集模块 (zinfo)
- **功能**: 设备信息收集和分析
- **包名**: `com.example.zinfo`
- **检测项目**:
  - 本地网络信息: zLocalNetworkInfo
  - 任务信息: zTaskInfo
  - 内存映射: zMapsInfo
  - Root文件: zRootFileInfo
  - 挂载点: zMountsInfo
  - 系统属性: zSystemPropInfo
  - 链接器信息: zLinkerInfo
  - 端口信息: zPortInfo
  - 类加载器: zClassLoaderInfo
  - 包信息: zPackageInfo
  - 系统设置: zSystemSettingInfo
  - TEE信息: zTeeInfo
  - SSL信息: zSslInfo
  - 时间信息: zTimeInfo

#### 4. 配置模块 (zconfig)
- **功能**: 配置管理和测试
- **包名**: `com.example.zconfig`
- **主要组件**: zConfig、zConfigTest

#### 5. 日志模块 (zlog)
- **功能**: 日志记录和管理
- **包名**: `com.example.zlog`
- **主要组件**: zLog、zLogTest

#### 6. 标准库模块 (zstd)
- **功能**: 标准库工具和字符串处理
- **包名**: `com.example.zstd`
- **主要组件**: zString、zStdUtil、vector、map

#### 7. 系统调用模块 (zlibc)
- **功能**: 系统调用封装和工具
- **包名**: `com.example.zlibc`
- **主要组件**: zLibc、zLibcUtil、zLibcTest

## 编译环境

### 开发环境要求

#### Android开发环境
- **Android Gradle Plugin**: 8.3.0
- **Gradle版本**: 8.4

#### 编译配置
- **compileSdk**: 34
- **targetSdk**: 34
- **minSdk**: 23
- **ABI支持**: arm64-v8a
- **Java版本**: 17
- **NDK版本**: 27.2.12479018
- **CMake版本**: 3.22.1

#### 第三方库
- **mbedTLS**: 用于加密和安全通信

## 主要功能

### 1. 多维度安全检测

#### 系统文件检测
- Root文件检测（su、mu等）
- 系统库完整性检查
- 关键文件权限验证

#### 进程检测
- 线程状态分析
- 调试工具注入检测（Frida等）
- 进程权限检查

#### 内存检测
- 库映射分析
- 内存篡改检测
- 动态链接库加载监控

#### 网络检测
- 本地网络设备扫描
- 端口异常检测
- 网络连接监控

#### 系统属性检测
- 关键系统属性验证
- 系统设置异常检测
- 安全配置检查

### 2. 性能优化特性

#### 线程优化
- CPU核心绑定
- 线程优先级提升
- 异步信息收集

#### 内存管理
- 智能内存分配
- 缓存机制
- 内存泄漏防护

#### 并发安全
- 读写锁保护
- 单例模式
- 线程安全设计

## 检测逻辑

### 1. Root检测
- 检查常见Root文件（su、mu等）
- 检查系统属性（ro.secure、ro.debuggable等）
- 检查挂载点异常

### 2. 调试工具检测
- 检测Frida注入的线程（gmain、pool-frida）
- 检查关键库的映射数量和权限
- 分析动态链接库加载情况

### 3. 系统完整性检测
- 检查系统属性值是否正确
- 检查关键库是否被篡改
- 检测系统挂载点异常

## 使用方式

### 1. 环境准备
```bash
# 确保已安装Android SDK和NDK
# 确保已安装CMake 3.22.1或更高版本
```

### 2. 编译项目
```bash
# 编译所有模块
./gradlew assembleDebug

# 编译特定模块
./gradlew :app:assembleDebug
./gradlew :zcore:assembleDebug
./gradlew :zinfo:assembleDebug
```
