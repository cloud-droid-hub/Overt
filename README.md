# Overt - Android设备安全检测工具

## 项目概述

Overt是一个专业的Android设备安全检测工具，通过多维度收集和分析系统信息来检测设备是否被Root、被调试工具注入或存在其他安全风险。项目采用模块化架构设计，将不同功能分散到独立的模块中，便于维护和扩展。所有代码都经过详细注释，确保代码的可读性和可维护性。

## 核心特性

- **多维度安全检测**: 涵盖Root检测、调试工具检测、系统完整性检测等
- **高性能架构**: 采用多线程、CPU核心绑定、智能缓存等优化技术
- **模块化设计**: 清晰的模块分离，便于维护和功能扩展
- **详细代码注释**: 所有函数和类都有完整的Doxygen风格注释
- **安全通信**: 基于mbedTLS的HTTPS通信，支持证书固定验证
- **跨平台兼容**: 支持Android 6.0+ (API 23+) 系统

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
  - **文件操作**: zFile - 提供文件读写、权限检查、时间戳获取等功能
  - **网络通信**: zHttps - 基于mbedTLS的HTTPS客户端，支持证书固定验证
  - **Shell执行**: zShell - 安全的Shell命令执行，用于系统状态检测
  - **JSON处理**: zJson - 轻量级JSON解析和构建
  - **线程管理**: zThread - 轻量级工作线程，支持任务调度
  - **任务管理**: zTask - 任务封装和执行管理
  - **广播处理**: zBroadCast - Android广播接收和处理
  - **类加载器**: zClassLoader - 类加载器信息获取
  - **链接器**: zLinker - 动态链接器信息分析
  - **CRC校验**: zCrc32 - CRC32校验算法实现
  - **ELF文件处理**: zElf - ELF文件格式解析
  - **TEE接口**: zTee - 可信执行环境接口
  - **JVM接口**: zJavaVm - Java虚拟机接口封装

#### 3. 信息收集模块 (zinfo)
- **功能**: 设备信息收集和分析，提供全面的安全检测能力
- **包名**: `com.example.zinfo`
- **检测项目**:
  - **进程信息**: zProcInfo - 检测进程状态、内存映射、挂载点、任务信息等
  - **类加载器信息**: zClassLoaderInfo - 检测LSPosed、Xposed等框架注入的类加载器
  - **包信息**: zPackageInfo - 检测系统中安装的调试工具和Root框架
  - **SSL信息**: zSslInfo - 检测HTTPS连接的SSL证书指纹，验证网络通信安全性
  - **时间信息**: zTimeInfo - 检测系统时间、启动时间等时间相关信息
  - **端口信息**: zPortInfo - 检测系统中可疑端口的使用情况
  - **日志信息**: zLogcatInfo - 检测系统日志中的可疑记录
  - **侧信道信息**: zSideChannelInfo - 通过侧信道攻击检测系统环境异常
  - **本地网络信息**: zLocalNetworkInfo - 扫描本地网络设备
  - **任务信息**: zTaskInfo - 分析任务状态和权限
  - **内存映射**: zMapsInfo - 分析库映射和内存篡改
  - **Root文件**: zRootFileInfo - 检测Root相关文件
  - **挂载点**: zMountsInfo - 检测系统挂载点异常
  - **系统属性**: zSystemPropInfo - 验证关键系统属性
  - **链接器信息**: zLinkerInfo - 分析动态链接器状态
  - **系统设置**: zSystemSettingInfo - 检查系统设置异常
  - **TEE信息**: zTeeInfo - 检测可信执行环境状态

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

#### Root检测
- **文件检测**: 检测常见Root文件（su、mu、busybox等）
- **系统属性检测**: 检查ro.secure、ro.debuggable等关键属性
- **挂载点检测**: 检测系统挂载点异常，如overlay挂载
- **包检测**: 检测Root管理应用（SuperSU、Magisk Manager等）

#### 调试工具检测
- **Frida检测**: 检测Frida注入的线程（gmain、pool-frida等）
- **IDA检测**: 检测IDA Pro调试工具的痕迹
- **进程检测**: 分析进程状态、内存映射、任务信息
- **端口检测**: 检测调试工具常用的端口监听

#### 系统完整性检测
- **类加载器检测**: 检测LSPosed、Xposed等框架注入的类加载器
- **SSL证书检测**: 验证HTTPS连接的证书指纹，防止中间人攻击
- **时间检测**: 检测系统时间篡改、启动时间异常
- **日志检测**: 检测系统日志中的可疑记录（如Zygisk痕迹）

#### 侧信道攻击检测
- **系统调用检测**: 通过比较不同系统调用的执行时间判断环境异常
- **文件访问检测**: 检测文件访问权限和路径异常
- **性能分析**: 通过性能特征检测调试工具和Hook框架

### 2. 高性能架构

#### 线程优化
- **CPU核心绑定**: 自动绑定到性能核心，提升检测效率
- **线程优先级提升**: 动态调整线程优先级，确保关键任务优先执行
- **异步信息收集**: 多线程并行收集各类系统信息，提高检测速度
- **智能任务调度**: 基于任务类型和依赖关系进行智能调度

#### 内存管理
- **智能内存分配**: 根据检测需求动态分配内存资源
- **缓存机制**: 缓存频繁访问的系统信息，减少重复查询
- **内存泄漏防护**: 严格的资源管理，确保无内存泄漏
- **内存映射优化**: 高效的文件映射和内存访问

#### 并发安全
- **读写锁保护**: 使用std::shared_mutex保护共享数据
- **单例模式**: 确保核心组件的唯一性和线程安全
- **原子操作**: 使用原子操作保证数据一致性
- **条件变量**: 实现线程间的协调和同步

### 3. 安全通信

#### HTTPS通信
- **mbedTLS集成**: 基于mbedTLS库实现安全的HTTPS通信
- **证书固定**: 支持证书固定验证，防止中间人攻击
- **TLS 1.2+**: 强制使用TLS 1.2及以上版本，确保通信安全
- **超时控制**: 完善的超时机制，防止长时间阻塞

#### 数据保护
- **加密传输**: 所有网络通信都使用HTTPS加密
- **证书验证**: 严格的证书验证机制
- **数据完整性**: 确保传输数据的完整性和真实性

## 代码质量

### 1. 详细注释
- **Doxygen风格**: 所有函数和类都使用Doxygen风格的注释
- **功能说明**: 每个函数都有详细的功能描述和实现说明
- **参数文档**: 完整的参数类型、用途和返回值说明
- **使用示例**: 关键函数提供使用示例和注意事项
- **线程安全**: 明确标注线程安全性和并发注意事项

### 2. 代码规范
- **命名规范**: 统一的命名约定，提高代码可读性
- **模块分离**: 清晰的模块边界和职责分离
- **错误处理**: 完善的错误处理和异常管理
- **资源管理**: 严格的资源分配和释放管理

### 3. 架构设计
- **模块化**: 高度模块化的设计，便于维护和扩展
- **可扩展性**: 易于添加新的检测模块和功能
- **可测试性**: 良好的测试接口和单元测试支持
- **跨平台**: 支持不同Android版本和架构

## 使用方式

### 1. 环境准备
```bash
# 确保已安装Android SDK 34和NDK 27.2.12479018
# 确保已安装CMake 3.22.1或更高版本
# 确保已安装Java 17
# 确保已安装Gradle 8.4
```

### 2. 编译项目
```bash
# 编译所有模块
./gradlew assembleDebug

# 编译特定模块
./gradlew :app:assembleDebug
./gradlew :zcore:assembleDebug
./gradlew :zinfo:assembleDebug

# 清理项目
./gradlew clean
```



## 参考资料

- [Android Key Attestation](https://developer.android.com/training/articles/security-key-attestation)
- [Android Keystore System](https://source.android.com/docs/security/features/keystore)
- [X.509 Certificate Structure](https://tools.ietf.org/html/rfc5280)
- [ASN.1 Encoding Rules](https://www.itu.int/rec/T-REC-X.690/)
- https://android.googlesource.com/platform/ndk
- https://github.com/openjdk/jdk8u.git
- https://github.com/vvb2060/KeyAttestation.git



**注意**: 本项目仅用于安全研究和教育目的，请遵守相关法律法规，不得用于非法用途。
