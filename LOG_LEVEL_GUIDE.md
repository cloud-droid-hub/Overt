# Android 日志级别使用指南

## 日志级别定义

| 级别 | 方法 | 适用场景 | 输出策略 |
|------|------|----------|----------|
| VERBOSE | Log.v() | 高频调试信息（如循环内部状态、事件追踪） | 开发阶段开启，生产环境关闭 |
| DEBUG | Log.d() | 调试关键路径（如函数参数、数据转换结果） | 开发阶段开启，生产环境关闭 |
| INFO | Log.i() | 系统核心事件（如应用启动、用户关键操作） | 生产环境保留，控制频率 |
| WARN | Log.w() | 潜在风险（如非预期参数但可恢复、资源接近阈值） | 生产环境保留，建议绑定监控告警 |
| ERROR | Log.e() | 可恢复错误（如网络超时、文件读取失败） | 生产环境保留，记录完整上下文 |

## 具体使用场景

### VERBOSE (Log.v)
- 循环内部的详细状态
- 高频事件的追踪
- 详细的函数调用链
- 数据结构的详细内容

```cpp
// 正确示例
LOGV("[TEE] Processing byte %d of %d", i, total_bytes);
LOGV("[TEE] Function call: parse_certificate() -> validate_signature()");
```

### DEBUG (Log.d)
- 函数入口和出口
- 关键参数值
- 数据转换结果
- 解析过程的中间状态

```cpp
// 正确示例
LOGD("[TEE] Starting certificate parsing, size: %d bytes", cert_size);
LOGD("[TEE] Found extension OID: %s", oid_string);
LOGD("[TEE] Parsing at offset %d, remaining %d bytes", offset, remaining);
```

### INFO (Log.i)
- 应用启动/停止
- 用户关键操作
- 重要业务事件
- 系统状态变化

```cpp
// 正确示例
LOGI("[TEE] TEE attestation check started");
LOGI("[TEE] Certificate validation completed successfully");
LOGI("[TEE] Security level: %d", security_level);
```

### WARN (Log.w)
- 非预期但可恢复的情况
- 资源使用接近阈值
- 降级处理
- 潜在的安全风险

```cpp
// 正确示例
LOGW("[TEE] RootOfTrust not valid, but continuing with basic checks");
LOGW("[TEE] Certificate parsing took longer than expected: %d ms", duration);
LOGW("[TEE] Using fallback validation method");
```

### ERROR (Log.e)
- 真正的错误情况
- 无法恢复的异常
- 关键功能失败
- 需要立即关注的问题

```cpp
// 正确示例
LOGE("[TEE] Failed to parse certificate: invalid format");
LOGE("[TEE] JNI environment is null");
LOGE("[TEE] Memory allocation failed for certificate data");
```

## TEE模块日志修复

### 问题分析
在TEE模块中发现大量不正确的日志级别使用：
- 正常的调试信息使用了`LOGE`
- 解析过程的中间状态被标记为错误
- 这会导致生产环境中产生大量"错误"日志

### 修复原则
1. **解析过程的中间状态** → `LOGD`
2. **正常的调试信息** → `LOGD`
3. **可恢复的异常情况** → `LOGW`
4. **真正的错误** → `LOGE`

### 已修复的示例
```cpp
// 修复前
LOGE("[Native-TEE] Searching for extension OID: %s", oid_str);
LOGE("[Native-TEE] Found attestation extension OID!");
LOGE("[Native-TEE] Extension OID as string: %s", oid_str_debug);

// 修复后
LOGD("[Native-TEE] Searching for extension OID: %s", oid_str);
LOGD("[Native-TEE] Found attestation extension OID!");
LOGD("[Native-TEE] Extension OID as string: %s", oid_str_debug);
```

## 最佳实践

### 1. 日志级别选择
- **开发阶段**: 使用DEBUG和VERBOSE级别进行详细调试
- **测试阶段**: 使用INFO级别验证关键流程
- **生产环境**: 主要使用WARN和ERROR级别，少量INFO级别

### 2. 日志内容
- 包含足够的上下文信息
- 使用有意义的标签（如`[TEE]`、`[JNI]`）
- 避免记录敏感信息

### 3. 性能考虑
- 避免在循环中使用高频日志
- 使用条件日志减少不必要的字符串格式化
- 生产环境中关闭DEBUG和VERBOSE级别

### 4. 监控告警
- WARN级别日志建议绑定监控告警
- ERROR级别日志必须记录完整上下文
- 定期分析日志模式，优化日志策略

## 配置建议

### 开发环境
```cpp
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
```

### 生产环境
```cpp
#define CURRENT_LOG_LEVEL LOG_LEVEL_WARN
```

### 调试特定模块
```cpp
#ifdef DEBUG_TEE_MODULE
    #define TEE_LOG_LEVEL LOG_LEVEL_DEBUG
#else
    #define TEE_LOG_LEVEL LOG_LEVEL_WARN
#endif
``` 