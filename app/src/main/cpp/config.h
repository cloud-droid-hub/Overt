#ifndef CONFIG_H
#define CONFIG_H

// ==================== 命名空间控制 ====================
// 通过这个宏来控制标准 API 和 非标准 API 的使用
// 定义 USE_NONSTD_API 时使用 nonstd 命名空间同时开启 nonstd_ 系列的宏替换
// 未定义时使用 std 命名空间


/*
 * 这里简单记录下原理：
 * 链接器在遇到符号冲突时，通常会按照以下优先级选择：
 * 当前编译单元中的符号（最高优先级）
 * 静态库中的符号（中等优先级）
 * 动态库中的符号（最低优先级）
 *
 * nonstd_open 被宏替换为 open，成为当前编译单元中的 open 函数
 * 系统的 open 函数在动态库中
 * 链接器优先选择了当前编译单元中的 open（自定义实现）
 *
 * 简单一点其实根本不需要进行宏替换，只要自定义的函数名称及签名和系统库一致，
 * 那么只需要通过一个宏来控制是否导入当前代码中就行了
 * */

/*
 * 优点：这样做的优点很明显，可以无缝切换自定义的 api 极大的减少来自 hook 系统库的攻击面
 * 配合系统调用以及混淆加固之后可以说可以通杀小白
 *
 * 缺点：缺点就是代码写起来有点麻烦，各种系统头文件不能无脑的引入
 * 稍不注意就是各种链接错误，很多时候要考虑最小引入的头文件
 * */

//#define USE_NONSTD_API

#ifdef USE_NONSTD_API
#include <asm-generic/fcntl.h>
#else
#include <fcntl.h>
#endif

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