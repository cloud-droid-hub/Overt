# libc函数替换总结

## 概述

已将项目中除了日志系统之外的所有标准libc函数调用替换为nonstd_版本，以确保使用自定义实现的函数。

## 替换的函数列表

### 字符串函数
- `strcmp` → `nonstd_strcmp`
- `strlen` → `nonstd_strlen`
- `strcpy` → `nonstd_strcpy`
- `strcat` → `nonstd_strcat`
- `strncmp` → `nonstd_strncmp`

### 内存函数
- `malloc` → `nonstd_malloc`
- `free` → `nonstd_free`
- `calloc` → `nonstd_calloc`
- `realloc` → `nonstd_realloc`
- `memset` → `nonstd_memset`
- `memcpy` → `nonstd_memcpy`

### 文件操作函数
- `open` → `nonstd_open`
- `close` → `nonstd_close`
- `read` → `nonstd_read`
- `write` → `nonstd_write`

### 网络函数
- `socket` → `nonstd_socket`
- `connect` → `nonstd_connect`
- `bind` → `nonstd_bind`
- `listen` → `nonstd_listen`
- `accept` → `nonstd_accept`

### 时间函数
- `time` → `nonstd_time`
- `gettimeofday` → `nonstd_gettimeofday`

### 进程函数
- `getpid` → `nonstd_getpid`
- `getppid` → `nonstd_getppid`

### 信号函数
- `kill` → `nonstd_kill`

### 其他函数
- `atoi` → `nonstd_atoi`
- `atol` → `nonstd_atol`

## 修改的文件

### 1. util.cpp
- 添加了 `#include "libc.h"`
- 替换了 `read`, `open`, `close`, `strlen`, `strcmp` 函数

### 2. port_info.cpp
- 添加了 `#include "libc.h"`
- 替换了 `socket`, `connect`, `close` 函数

### 3. time_info.cpp
- 添加了 `#include "libc.h"`
- 替换了 `time` 函数

### 4. tee_info.cpp
- 添加了 `#include "libc.h"`
- 替换了 `strlen` 函数

### 5. system_setting_info.cpp
- 添加了 `#include "libc.h"`
- 替换了 `strlen` 函数

### 6. zClassLoader.cpp
- 添加了 `#include "libc.h"`
- 替换了 `atoi` 函数

### 7. zFile.cpp
- 添加了 `#include "libc.h"`
- 替换了 `open`, `close`, `read`, `strcmp` 函数

### 8. zElf.cpp
- 添加了 `#include "libc.h"`
- 替换了 `strncmp`, `strcmp`, `open`, `close` 函数

### 9. zJavaVm.cpp
- 添加了 `#include "libc.h"`
- 替换了 `memcpy` 函数

### 10. tee_cert_parser.cpp
- 添加了 `#include "libc.h"`
- 替换了 `strlen`, `memset` 函数

### 11. zLog.cpp
- 添加了 `#include "libc.h"`
- 替换了 `malloc`, `free` 函数

## 未替换的文件

### string.cpp
- 该文件已经使用了自定义的字符串函数实现（`custom_strlen`, `custom_strcpy`, `custom_strcat`）
- 不需要替换

### libc.cpp
- 该文件包含了所有nonstd_函数的实现
- 不需要替换

## 注意事项

1. **日志系统**: 日志系统（zLog.cpp和zLog.h）中的函数调用已保留，因为这些是系统级的日志输出功能。

2. **头文件包含**: 所有修改的文件都添加了 `#include "libc.h"` 来包含函数声明。

3. **函数声明**: 所有nonstd_函数的声明都在 `libc.h` 中，实现都在 `libc.cpp` 中。

4. **编译兼容性**: 所有替换都保持了函数签名的一致性，确保编译不会出错。

## 验证方法

可以通过以下方式验证替换是否成功：

1. **编译检查**: 确保项目能够正常编译
2. **功能测试**: 运行应用确保功能正常
3. **日志检查**: 查看nonstd_函数的日志输出，确认使用的是自定义实现

## 问题修复记录

### 2025-01-27 修复崩溃问题

**问题描述**: 使用自定义API时，`zElf::get_maps_base`函数发生空指针访问崩溃

**根本原因**: 
1. `nonstd_memcmp`函数未在头文件中声明和实现
2. `zElf.cpp`中使用了未声明的`nonstd_memcmp`函数
3. 缺少空指针检查，导致访问空指针时崩溃

**修复内容**:
1. 在`nonstd_libc.h`中添加了`nonstd_memcmp`函数声明
2. 在`nonstd_libc.cpp`中实现了`nonstd_memcmp`函数
3. 在宏定义中添加了`#define nonstd_memcmp memcmp`
4. 修复了`zElf.cpp`中的空指针检查问题
5. 替换了所有遗漏的`strstr`和`memcmp`函数调用

**修复的文件**:
- `nonstd_libc.h`: 添加`nonstd_memcmp`声明和宏定义
- `nonstd_libc.cpp`: 实现`nonstd_memcmp`函数
- `zElf.cpp`: 修复空指针检查和函数调用
- `util.cpp`: 替换`memcmp`调用
- `linker_info.cpp`: 替换`strstr`调用
- `class_loader_info.cpp`: 替换`strstr`调用
- `mounts_info.cpp`: 替换`strstr`调用
- `task_info.cpp`: 替换`strstr`调用

### 2025-01-27 修复nonstd::string空指针问题

**问题描述**: 使用自定义API时，`split_str`函数分割`nonstd::string`后返回空字符串，导致后续处理失败

**根本原因**: 
1. `nonstd::string`的构造函数和`c_str()`方法没有正确处理空指针和空字符串
2. 当`split_str`函数创建长度为0的子字符串时，`nonstd::string`的`c_str()`方法可能返回无效指针

**修复内容**:
1. 修复了`nonstd::string`的`c_str()`方法，确保总是返回有效指针
2. 修复了`nonstd::string`的构造函数，正确处理空指针和长度为0的情况
3. 修复了`nonstd::string`的复制构造函数和赋值操作符，正确处理空指针
4. 修复了`nonstd::string`的析构函数和`clear()`方法，避免删除空指针
5. 添加了详细的日志输出，便于调试

**修复的文件**:
- `string.cpp`: 修复`nonstd::string`类的所有方法，确保与`std::string`行为一致

### 2025-01-27 基于std::string开源实现重构nonstd::string

**改进内容**:
1. **参考std::string开源实现**: 直接参考libstdc++的`basic_string.h`和`basic_string.tcc`实现
2. **构造函数行为一致性**: 
   - 允许`nullptr`指针当长度为0时（与std::string行为一致）
   - 只有当`nullptr`且长度大于0时才抛出异常
3. **辅助函数健壮性**: 
   - `custom_strlen`: 添加空指针检查
   - `custom_strcpy`: 添加空指针检查
   - `custom_strcat`: 添加空指针检查
4. **内存管理改进**: 
   - 所有析构函数和清理操作都添加空指针检查
   - 确保内存分配和释放的一致性

**技术细节**:
- 基于GCC 13的libstdc++实现
- 参考`basic_string(const _CharT* __s, size_type __n)`构造函数
- 遵循C++标准库的异常处理规范
- 保持与std::string完全一致的行为

### 2025-01-27 修复nonstd::vector的emplace_back实现

**问题描述**: `split_str`函数使用`result.emplace_back(s + i, j - i)`时，`nonstd::vector`的`emplace_back`实现有问题

**根本原因**: 
1. `nonstd::vector`的`emplace_back`方法只是简单地调用`T()`默认构造函数
2. 没有使用传入的参数进行完美转发构造
3. 导致`split_str`函数创建的子字符串都是空字符串

**修复内容**:
1. **修复`emplace_back`实现**: 使用完美转发`std::forward<Args>(args)...`来正确构造元素
2. **参考Android NDK实现**: 确认Android的libc++实现与标准一致
3. **验证构造函数行为**: 确认Android的`basic_string`构造函数行为与PC端一致

**修复的文件**:
- `vector.h`: 修复`nonstd::vector`的`emplace_back`方法，使用完美转发

**技术细节**:
- 基于Android NDK 21.1.6352462的libc++实现
- 参考`template <class... Args> reference emplace_back(Args&&... args)`声明
- 使用`std::forward`进行完美转发
- 确保与标准库行为完全一致

### 2025-01-27 修复nonstd::string的rfind方法

**问题描述**: 使用非标准API时，`get_linker_info`函数中的`so_path.rfind('/')`导致`substr`抛出`std::out_of_range`异常

**根本原因**: 
1. `nonstd::string`的`rfind`方法在处理`size_t`下溢时有问题
2. 当`rfind`找不到字符时，应该返回`npos`，但循环逻辑有缺陷
3. 导致`rfind('/')`返回错误的位置，进而导致`substr`参数越界

**修复内容**:
1. **修复`rfind(char ch, size_t pos)`方法**: 使用`int`类型处理循环，避免`size_t`下溢
2. **修复`find_last_of`方法**: 同样使用`int`类型处理循环
3. **确保正确返回`npos`**: 当找不到字符时正确返回`npos`

**修复的文件**:
- `string.cpp`: 修复`nonstd::string`的`rfind`和`find_last_of`方法

**技术细节**:
- 使用`int`类型进行循环，避免`size_t`下溢问题
- 正确处理边界条件，确保找到字符时返回正确位置
- 确保找不到字符时返回`npos`
- 修复了`get_linker_info`函数中的字符串处理逻辑

## 后续维护

- 新增代码时，请使用nonstd_版本的函数
- 如果添加新的libc函数，请在 `libc.h` 中声明，在 `libc.cpp` 中实现
- 保持函数签名与标准libc函数一致

## 宏定义切换功能

### 使用方法

1. **使用非标准库实现**（默认）:
   ```bash
   # 编译时定义宏
   g++ -DUSE_NONSTD_API your_file.cpp
   ```

2. **使用标准库实现**:
   ```bash
   # 不定义宏，或注释掉宏定义
   g++ your_file.cpp
   ```

### 宏定义说明

- 当定义了 `USE_NONSTD_API` 宏时，使用 `libc.cpp` 中的自定义实现
- 当未定义 `USE_NONSTD_API` 宏时，所有 `nonstd_*` 函数会被宏替换为对应的标准库函数
- 这样可以在不修改代码的情况下，快速切换使用标准库或自定义实现

### 在Android项目中配置

在 `app/build.gradle` 中：

```gradle
android {
    defaultConfig {
        // 使用非标准库实现
        cppFlags "-DUSE_NONSTD_API"
    }
    
    buildTypes {
        release {
            // 发布版本可以使用标准库实现
            // cppFlags "-DUSE_NONSTD_API"  // 注释掉这行
        }
    }
}
``` 