#ifndef CONFIG_H
#define CONFIG_H

// ==================== 命名空间控制 ====================
// 通过这个宏来控制标准 API 和 非标准 API 的使用
// 定义 USE_NONSTD_API 时使用 nonstd 命名空间同时开启 nonstd_ 系列的宏替换
// 未定义时使用 std 命名空间

#define USE_NONSTD_API

#ifdef USE_NONSTD_API
    #include "nonstd_libc.h"
    #include "string.h"
    #include "vector.h"
    #include "map.h"

    using nonstd::string;
    using nonstd::vector;
    using nonstd::map;

#else

    // 当使用 std 命名空间时，包含标准库头文件
    #include <string>
    #include <vector>
    #include <map>
    
    // 使用 std 命名空间（但避免与系统函数冲突）
    using std::string;
    using std::vector;
    using std::map;

#endif

#endif // CONFIG_H