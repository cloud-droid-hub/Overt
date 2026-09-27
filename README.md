# Overt - Android设备安全检测工具

## 项目概述

Overt是一个专业的Android设备安全检测工具，通过多维度收集和分析系统信息来检测设备是否被Root、被调试工具注入或存在其他安全风险。项目采用模块化架构设计，将不同功能分散到独立的模块中，便于维护和扩展。所有代码都经过详细注释，确保代码的可读性和可维护性。

## 核心特性

- **多维度安全检测**: 涵盖Root检测、调试工具检测、系统完整性检测等
- **模块化设计**: 清晰的模块分离，便于维护和功能扩展
- **安全通信**: 基于mbedTLS的HTTPS通信，支持证书固定验证
- **非标准API支持**: 可切换使用标准API或非标准API，绕过libc层直接进行系统调用

## 项目架构

项目采用多模块架构，包含app（主应用）、zengine（分析引擎）、zcore（核心功能库）、zinfo（信息收集）、zconfig（配置管理）、zlog（日志）、zstd（标准库工具）、zlibc（系统调用封装）等模块。采用分层依赖架构，从配置管理到任务调度共分6个依赖等级。

## 查杀分离架构

本项目采用「查杀分离」设计，将**信息采集**与**风险判定**彻底解耦：

```
[C++ 侧 —— 采集，到此为止]
  zinfo 采集器 (只收集全量原始数据，输出 {项目: {value: 原始值}}，无任何判定)
  → zManager 调度 (每 10s 轮询 19 类检测任务)
  → notice_java(title)  ← 向 zengine 上传原始数据的唯一上传点，此后零 native 调用

[Java 侧 —— 分析，由 zengine 引擎负责]
  NativeUpdateBus.onCardInfoUpdated(title, rawJson)
  → MainActivity 后台线程调 MainApplication.analyze(title, rawJson)   [zengine 分析引擎]
  → 19 个 Analyzer 按内置黑名单/阈值/规则判定 → {项目: {risk: safe|warn|error, explain: ...}}
  → 回主线程 InfoCardContainer 渲染成风险卡片
```

各模块职责：
- **zinfo**（C++）：只负责**采集原始数据**（文件/属性/进程/Linker/TEE/端口/传感器原始字段等），全量上报，不做 risk 判定；黑白名单等判定数据不在采集端内置（PackageInfo 的探测包名列表仅作为探测范围声明）。
- **zengine**（纯 Java 分析引擎）：内置全部**风险判定逻辑**——黑名单、阈值、期望值、特征串。定位为**【模拟服务端】**：采集端(native)上传"客户端观察到的数据"，zengine 以服务端视角(不被 hook 的信任锚)做判定。入口 `MainApplication.analyze(category, rawJson)`，返回 `{risk, explain}` 结果 JSON。通过 `app/build.gradle` 的 `sourceSets` 源码共享编入 app，也作为独立模块以硬编码数据做单测。允许使用 `android.util.Log`（运行日志）；网络/Context 等能力通过可注入 Provider 解耦（如 `SslFingerprintFetcher`、`ContextProvider`）。
- **app**（Java UI + C++ 调度）：`zManager` 只编排采集；`MainActivity` 只做显示——收到原始数据后，**在后台线程调用 zengine 分析**（analyzer 可能发网络请求，主线程会抛 NetworkOnMainThreadException），结果回主线程渲染。
- **zcore/zconfig/zlog/zstd/zlibc**：核心库、配置开关、日志、标准库替代、libc 封装。

19 类检测任务均已分离：`risk_file_info` / `class_loader_info` / `class_info` / `side_channel_info` / `finger_info` / `linker_info` / `proc_info` / `tee_info` / `package_info` / `system_setting_info` / `system_prop_info` / `signature_info` / `port_info` / `time_info` / `ssl_info` / `local_network_info` / `selinux_info` / `isoloated_process_info` / `sensor_info`，每一类的判定逻辑都在 `zengine/src/main/java/com/example/zengine/analyzer/` 下对应 Analyzer 中。

注：`package_info` 的探测包名列表（C++ `probe_package_map`）是采集范围的天然声明（JNI 必须知道探测哪些包）；风险判定（黑名单命中即 error、白名单缺失即 warn）的名单完全内置在 Java 侧 `PackageInfoAnalyzer`。

## 核心原理

> 统一说明：以下各检测器的「采集」在 `zinfo` 侧 C++ 完成并全量上报原始数据；「判定」（黑名单/阈值/期望值）在 `zengine` 侧 Java Analyzer 中完成。

#### Root检测原理

1. **风险文件检测**（采集端 zRiskFileInfo.cpp,统一探测 root 特征 + 模拟器特征文件）:
   - 遍历 root 特征文件（`/system/bin/su`、`/system/xbin/su` 等）+ 模拟器特征文件（qemu/goldfish/vbox/redroid 等）
   - 全部路径统一探测存在状态，上报 `{路径 -> {value: "1"/"0"}}`，不掺类别/判定
   - 由 zengine RiskFileAnalyzer 内置名单判定：root 特征文件存在 → error；模拟器特征文件存在 → error
   - Root 管理应用（SuperSU、Magisk Manager 等）安装检测见「包信息检测原理」

2. **系统属性检测**（见「系统属性检测原理」）:
   - ro.secure/ro.debuggable 等关键属性的期望值校验由 zengine 判定

3. **挂载点/挂载命名空间检测**（见「进程信息检测原理」）:
   - /proc/mounts 异常挂载点、overlay 挂载、mount ns 对比均由 zengine 判定

#### 类加载器检测原理

1. **类加载器遍历**:
   - 通过JVM接口获取所有已加载的类加载器
   - 将类加载器转换为字符串表示

2. **特征匹配**:
   - 检测`LspModuleClassLoader`等LSPosed框架的类加载器
   - 检测`InMemoryDexClassLoader`等内存动态加载的类加载器
   - 这些类加载器通常由Hook框架注入

3. **类名检测**:
   - 遍历所有已加载的类名
   - 匹配黑名单类名（如`lsposed`、`XposedBridge`等）
   - 发现可疑类名则判定为存在Hook框架

#### 侧信道检测原理

1. **系统调用时序分析**:
   - 绑定线程到特定CPU核心，确保测量稳定性
   - 使用ARM64虚拟计数器（`cntvct_el0`寄存器）获取高精度时间戳
   - 执行大量系统调用（如`faccessat`、`fchownat`）并记录执行时间

2. **异常检测**:
   - 正常情况下，`faccessat`的执行速度应该快于`fchownat`
   - 如果`faccessat`大量慢于`fchownat`，说明存在Hook框架拦截
   - Hook框架会增加系统调用的执行时间，导致时序异常
   - 采集端只上报异常计数，阈值判定（7000/5000）由 zengine 负责

3. **KernelSU/APatch prctl 探测**（机制级内核root检测）:
   - KernelSU/APatch 是内核级 root，无文件痕迹，但通过私有 prctl `0xDEADBEEF` 与用户态通信
   - 普通内核不认识该 option → 返回错误且 out 参数不变
   - 打了 KSU/APatch 补丁的内核会响应（写版本号 / 回魔数 `0x5A5A5A5A` / 返回 0）
   - 探测到内核响应即判定存在内核 root，由 zengine 判定为 risk

#### 时间检测原理

1. **本地时间获取**: 使用标准时间函数获取系统当前时间
2. **启动时间检测**:
   - 通过系统调用获取系统启动时间
   - 分析`/proc/mounts`中文件系统的最早创建时间
   - 启动时间过短可能表示设备刚重启或时间被篡改

3. **远程时间验证**:
   - 通过HTTPS请求获取远程服务器时间
   - 使用证书固定验证确保通信安全
   - 比较本地时间和远程时间，差异过大则判定为时间被篡改

#### 内存映射检测原理

1. **maps文件解析**:
   - 解析`/proc/self/maps`文件，获取进程内存映射信息
   - 分析关键系统库（如`libart.so`、`libc.so`）的映射情况

2. **映射数量检测**:
   - 正常情况下，系统库应该有固定数量的内存段（通常为4个）
   - 映射数量异常可能表示库被篡改或Hook

3. **权限检测**:
   - 检查内存段的权限标志（如`r--p`、`r-xp`、`rw-p`）
   - 权限异常可能表示内存被修改或Hook框架注入

4. **内容检测**:
   - 读取关键文件（如`base.odex`）的二进制内容
   - 搜索可疑字符串（如`--inline-max-code-units=0`）
   - 发现可疑内容则判定为存在Hook框架

#### 进程信息检测原理

1. **内存映射分析**（maps）:
   - 遍历 `/proc/self/maps`，上报关键系统库（libart/libc/libinput）的映射段数/权限序列
   - 检测 base.odex 加载状态与内容特征（`--inline-max-code-units=0`）
   - 段数/权限异常由 zengine 判定（正常为 4 段、权限序列 r--p/r-xp/r--p/rw-p）

2. **挂载点检测**（mounts）:
   - 读取 `/proc/self/mounts` 全量上报
   - 检测异常挂载名（dex2oat/APatch/shamiko/模块）与 /system overlay 由 zengine 判定

3. **任务信息检测**（task）:
   - 分析 `/proc/self/task` 每个线程的 stat，上报原始行
   - 检测可疑线程名（`gmain`/`pool-frida` 等 Frida 特征）由 zengine 判定

4. **进程属性检测**（attr_prev）:
   - 读取 `/proc/self/attr/prev`，`zygote` 特征（可能为 Magisk 痕迹）由 zengine 判定

5. **网络连接检测**（net_tcp）:
   - 读取 `/proc/self/net/tcp`，Frida/IDA 端口特征（:69A2/:69A3/:5D8A）由 zengine 判定

6. **挂载命名空间对比**（mount ns）:
   - 上报自己的与 init 进程（PID 1）的 `ns/mnt` ID 与 mountinfo 全文（原始数据）
   - zengine 对比：两者 mnt namespace 不同 → 隔离了挂载（被动手脚）→ risk
   - init 中关键挂载点（/system /vendor /product /system_ext /odm /data/adb /sbin /debug_ramdisk /apex）
     在自己的 mountinfo 中缺失 → mount 隐藏证据 → risk

#### SSL证书检测原理

1. **证书指纹采集**（采集端 zSslInfo.cpp）:
   - 对目标 URL（百度、腾讯新闻 ip2city）发起 HTTPS 请求（mbedTLS），获取服务器证书
   - 上报观察到的证书 SHA256 指纹 + 请求错误，均为原始数据
   - 不内置任何期望指纹（期望值不硬编码在采集端）

2. **期望指纹动态获取**（zengine SslFingerprintFetcher）:
   - 期望指纹不由采集端硬编码，而是 zengine 按需重新请求目标 URL，现场解析叶子证书 SHA256 指纹
   - 持久化缓存为 `date:cert` 键值对（app 私有目录文件），当天命中则直接复用，不重复请求

3. **指纹对比判定**（zengine SslInfoAnalyzer）:
   - 观察指纹 vs 当天期望指纹，不一致 → error（MITM/证书被篡改）
   - 期望指纹获取失败 → 无法比对，该项不武断判定
   - 注意：期望值与观察值同源（同设备），MITM 检测依赖"服务端视角"的信任锚语义

4. **地理位置判定**:
   - 腾讯新闻 ip2city 响应解析出国家+省+市（采集端只解析，不判定）
   - zengine 判定：为空 → error；不以"中国"开头 → error；否则 safe

#### 设备指纹检测原理

1. **Android ID获取**:
   - 通过JNI调用Android Settings.Secure API获取android_id
   - Android ID是设备唯一标识符，用于设备识别

2. **存储指纹生成**:
   - 使用`statfs64`系统调用获取存储文件系统信息
   - 提取文件系统类型、块大小、块数量、文件数量、文件系统ID等特征
   - 拼接生成唯一的存储指纹字符串

3. **DRM ID获取**:
   - 使用Android MediaDrm API获取Widevine DRM设备唯一ID
   - 将32字节的DRM ID压缩为16个十六进制字符
   - DRM ID为空可能表示设备DRM功能异常或被禁用

#### 链接器信息检测原理

1. **共享库路径遍历**:
   - 通过动态链接器获取所有已加载共享库的路径列表
   - 遍历所有库路径，检测可疑库文件

2. **黑名单库检测**:
   - 检测LSPosed相关库（包含"lsposed"字符串）
   - 检测Frida相关库（包含"frida"字符串）
   - 这些库的存在通常表明Hook框架已注入

3. **CRC校验和检测**:
   - 对关键系统库（如`libc.so`、`libart.so`、`libinput.so`）进行CRC校验
   - 比较运行时库的CRC值与预期值
   - CRC不匹配表示库文件被篡改或Hook

#### TEE信息检测原理

1. **KeyStore认证证书链获取**（采集端 zTeeInfo.cpp）:
   - 通过Android KeyStore API生成密钥对，设置认证挑战（Attestation Challenge "tee_check"）
   - 获取完整的X.509认证证书链（叶子→根，不只取叶子）
   - 整条链 base64 编码后作为 `tee_cert_chain` 原始数据上报，C++ 侧不做任何解析/判定

2. **验签（zengine TeeAttestationVerifier）**:
   - 链自洽验签：逐级验证叶子由中间签发、链顶自签（伪造证书/篡改链在此被拦下）
   - 根公钥增强：链顶公钥 ∈ {Google / AOSP 根} 视为强信任；厂商/模拟器根不误报（链自洽为硬门槛）
   - 采集端每次随机生成 challenge（getrandom 内核真随机 → zRandom），zengine 校验证书内的 challenge 与上报值一致（防重放/防简单替换）

3. **字段解析（zengine Asn1Attestation，抄自 KeyAttestation）**:
   - 用 BouncyCastle 解析叶子证书的 TEE Attestation 扩展（OID 1.3.6.1.4.1.11129.2.1.17）
   - AuthorizationList 以 ASN1TaggedObject 存储，`getTagNo()` 取 tag（RootOfTrust/OS_VERSION 等）
   - 提取 RootOfTrust：device_locked、verified_boot_state、verified_boot_key

4. **安全状态判定（zengine TeeInfoAnalyzer）**:
   - 验签失败 / challenge 不匹配 → tee_verify error
   - 解析失败 / 无 RootOfTrust → tee_statue error
   - device_locked 未锁定 → error
   - verified_boot_state 非已验签(VERIFIED=0) → error

#### 包信息检测原理

1. **探测包名列表**（采集端 zPackageInfo.cpp）:
   - 采集端维护一个**探测包名列表**（probe_package_map，只声明"探测哪些包"，无黑白语义）
   - 对列表内每个包，用 4 种方式探测安装状态并全量上报：
     - Context（PackageManager.getApplicationInfo / getLaunchIntentForPackage）
     - 路径（/data/data、/data/user/0、/data/user_de/0、外部存储）
     - 路径漏洞（/sdcard/android/data 等）
     - Shell 越权
   - 上报原始安装方式编码（0/pms/file/path_hole/shell_hole），不做判定

2. **黑白名单判定**（zengine PackageInfoAnalyzer）:
   - 黑名单名单（119 个 root/调试/VPN 包）与白名单（微信/支付宝）**完全内置在 Java 侧**
   - 黑名单包已安装（任意方式）→ error
   - 白名单包未安装 → warn

#### 系统设置检测原理

1. **充电状态检测**:
   - 通过注册BatteryManager广播接收器获取电池状态
   - 检测设备是否正在充电或已充满
   - 充电状态异常可能表示设备环境异常

2. **安装器名称检测**:
   - 通过PackageManager获取应用的安装器包名
   - 检测是否通过可疑安装器安装（如Root管理应用）
   - 异常安装器可能表示应用被恶意修改

#### 系统属性检测原理

1. **属性遍历**（采集端 zSystemPropInfo.cpp）:
   - 使用`__system_property_foreach`遍历所有系统属性，全量上报属性名、值、序列号版本(原始数据)
   - 采集端不做任何判定

2. **关键属性验证**（zengine SystemPropAnalyzer）:
   - 检查`ro.secure`应为"1"、`ro.debuggable`应为"0"、`ro.build.tags`应为"release-keys"等
   - 值不在期望列表 → error

3. **序列号分析**:
   - 关键 `ro.*` 属性 serial_version != 0（被动态修改）→ error

4. **Community ROM / 模拟器 / 云手机属性**:
   - lineage/cm/mokee/rr/pixelexperience/modversion 属性存在 → warn（custom rom）
   - ro.kernel.qemu / ro.boot.qemu / ro.hardware.virtual / cloudphone 家族 / redroid → warn（模拟器/云手机）

5. **分区 fingerprint 一致性**:
   - 主 `ro.build.fingerprint` 与各分区指纹（system/vendor/odm/product/system_ext/bootimage）不一致
   - 不一致 → warn（厂商 ROM 分区指纹差异是正常现象，不判 error）

#### 签名信息检测原理

1. **APK路径获取**:
   - 通过`dladdr`获取当前库文件路径
   - 使用正则表达式提取应用特定目录路径
   - 构建`base.apk`的完整路径

2. **ZIP文件解析**:
   - 使用minizip库解析APK文件（APK本质是ZIP格式）
   - 遍历ZIP文件中的条目，查找META-INF目录下的.RSA签名文件

3. **签名提取和验证**:
   - 提取.RSA文件的二进制内容
   - 计算签名文件的SHA256哈希值
   - 验证签名文件是否存在和有效

#### 端口信息检测原理

1. **端口连接检测**:
   - 创建TCP socket并尝试连接到指定端口
   - 使用非阻塞模式，设置200ms超时
   - 连接成功则判定为端口被占用

2. **可疑端口检测**（zengine PortInfoAnalyzer 判定）:
   - 检测调试工具常用端口（Frida 27042/27043/27047、IDA 23946）
   - 采集端上报端口占用状态(原始)，zengine 按端口→工具名映射判定
   - 端口被占用 → error（调试工具或Root框架正在运行）

3. **错误处理**:
   - 处理连接失败、超时等异常情况
   - 使用`select`系统调用等待连接结果
   - 检查socket错误状态确定连接是否成功

#### 本地网络信息检测原理

1. **UDP广播机制**:
   - 启动UDP广播发送器，向端口7476发送"overt"消息
   - 启动UDP广播监听器，监听端口7476接收响应
   - 启动本地IP监控，获取本机网络信息

2. **设备发现**（zengine LocalNetworkAnalyzer 判定）:
   - 接收UDP广播响应，记录发送方的IP地址(全量上报原始数据)
   - 检测到同网其他Overt设备 → warn
   - 用于检测同一网络中的其他Overt设备

#### SELinux检测原理

1. **进程 SELinux context 探测**（采集端 zSelinuxInfo.cpp）:
   - 读取 `/proc/self/attr/current`，上报进程的 SELinux 上下文（原始数据）
   - 正常 app 域为 `untrusted_app`；若上下文含 `magisk`/`su:`/`:su` 域标记，说明进程被 root 框架提权
   - 由 zengine SelinuxInfoAnalyzer 判定为 risk

2. **Zygisk 痕迹检测**（logcat 审计日志）:
   - 遍历指定PID范围（1500-2000），grep 出含 `avc` + `u:r:su:s0` 的日志行全量上报
   - `u:r:su:s0` 是 Zygisk 等 root 框架的 SELinux 审计痕迹，由 zengine 判定

#### 隔离进程信息检测原理

1. **Binder通信机制**:
   - 通过共享内存和Binder IPC与隔离进程通信
   - 发送"get_isoloated_process_info"消息请求进程信息

2. **进程信息获取**:
   - 隔离进程执行进程信息检测（调用`get_proc_info`）
   - 将检测结果序列化为JSON格式返回
   - 主进程解析JSON响应，获取隔离进程的检测结果

3. **跨进程检测优势**:
   - 隔离进程运行在独立进程空间，可能检测到主进程无法检测的异常
   - 通过Binder通信实现跨进程信息共享

#### 传感器信息检测原理

1. **原始传感器数据采集**（采集端 zSensorInfo.cpp）:
   - 使用 `zSensorManager` 获取设备所有传感器，逐个上报原始字段：
     `name, type, minDelay, maxDelay, fifoMax, fifoReserved, isWakeUp`
   - 采集端不做任何统计/聚合/评分（查杀分离最彻底：只报原始数据）

2. **统计与风险评分**（zengine SensorInfoAnalyzer）:
   - 从原始字段自行统计：传感器总数、FIFO 为 0 的数量、wake-up 传感器数
   - 判定规则（迁移自原 C++ 逻辑）:
     - 传感器总数 < 20 → +30 分
     - 所有传感器 FIFO 为 0 → +30 分
     - wake-up 传感器数 < 2 → +20 分
     - 组合加分（wakeup不足且FIFO全空）→ +20 分，封顶 100

3. **风险等级判定**:
   - 评分 > 60 → error；评分 > 0 → warn
   - 传感器 name 含 `goldfish`（qemu 模拟器传感器实现）→ 独立判定 emulator 风险

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
- **NDK版本**: 27.1.12297006
- **CMake版本**: 3.22.1

#### 第三方库
- **mbedTLS**: 用于加密和安全通信

## 使用方式

### 1. 环境准备
```bash
# 确保已安装Android SDK 34和NDK 27.1.12297006
# 确保已安装CMake 3.22.1或更高版本
# 确保已安装Java 17
# 确保已安装Gradle 8.4
```

项目已在顶层 Gradle 配置中显式固定 `NDK 27.1.12297006`，不再依赖 `local.properties` 中的 `ndk.dir` 推断版本。

### 2. 编译项目
```bash
# 编译所有模块
./gradlew assembleDebug

# 编译特定模块
./gradlew :app:assembleDebug
./gradlew :zcore:assembleDebug
./gradlew :zinfo:assembleDebug
./gradlew :zengine:assembleDebug

# 运行 zengine 分析引擎单测(硬编码数据，无需真机)
./gradlew :zengine:testDebugUnitTest

# 清理项目
./gradlew clean
```

### 3. 配置非标准API

在`zconfig/src/main/cpp/zConfig.h`中修改`ZCONFIG_ENABLE_NONSTD_API`宏：

- **启用非标准API**: `#define ZCONFIG_ENABLE_NONSTD_API 1`
- **使用标准API**: `#define ZCONFIG_ENABLE_NONSTD_API 0`

修改后重新编译项目即可生效。

## 参考资料

- [Android Key Attestation](https://developer.android.com/training/articles/security-key-attestation)
- [Android Keystore System](https://source.android.com/docs/security/features/keystore)
- [X.509 Certificate Structure](https://tools.ietf.org/html/rfc5280)
- [ASN.1 Encoding Rules](https://www.itu.int/rec/T-REC-X.690/)
- https://android.googlesource.com/platform/ndk
- https://github.com/openjdk/jdk8u.git
- https://github.com/vvb2060/KeyAttestation.git

**注意**: 本项目仅用于安全研究和教育目的，请遵守相关法律法规，不得用于非法用途。
