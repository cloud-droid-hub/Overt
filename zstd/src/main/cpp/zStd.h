//
// Created by lxz on 2025/8/6.
//

#ifndef OVERT_ZSTD_H
#define OVERT_ZSTD_H

#include "zConfig.h"

// 模块配置开关 - 可以通过修改这个宏来控制日志输出
#define ZSTD_ENABLE_NONSTD_API 1

// 当全局配置宏启用时，全局宏配置覆盖模块宏配置
#if ZCONFIG_ENABLE
#undef ZSTD_ENABLE_NONSTD_API
#define ZSTD_ENABLE_NONSTD_API ZCONFIG_ENABLE_NONSTD_API
#endif

#if ZSTD_ENABLE_NONSTD_API

    #include "zString.h"
    #include "vector.h"
    #include "map.h"

    using nonstd::string;
    using nonstd::vector;
    using nonstd::map;
    using nonstd::pair;
#else

// 当使用 std 命名空间时，包含标准库头文件

#include <string>
#include <vector>
#include <map>


// 使用 std 命名空间（但避免与系统函数冲突）
using std::string;
using std::vector;
using std::map;
using std::pair;


#endif



#endif //OVERT_ZSTD_H
