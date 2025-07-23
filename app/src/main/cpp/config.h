#ifndef CONFIG_H
#define CONFIG_H

// ==================== 命名空间控制 ====================
// 通过这个宏来控制标准 API 和 非标准 API 的使用
// 定义 USE_NONSTD_API 时使用 nonstd 命名空间同时开启 nonstd_ 系列的宏替换
// 未定义时使用 std 命名空间
#define USE_NONSTD_API

#ifdef USE_NONSTD_API

    #include "string.h"
    #include "vector.h"
    #include "map.h"

    using nonstd::string;
    using nonstd::vector;
    using nonstd::map;

    #include "nonstd_libc.h"

    #define strcmp nonstd_strcmp
    #define strlen nonstd_strlen
    #define strcpy nonstd_strcpy
    #define strcat nonstd_strcat
    #define strncmp nonstd_strncmp
    #define strrchr nonstd_strrchr
    #define strncpy nonstd_strncpy
    #define strlcpy nonstd_strlcpy
    #define strstr nonstd_strstr
    #define strchr nonstd_strchr
    #define memset nonstd_memset
    #define memcpy nonstd_memcpy
    #define malloc nonstd_malloc
    #define free nonstd_free
    #define calloc nonstd_calloc
    #define realloc nonstd_realloc
    #define open nonstd_open
    #define close nonstd_close
    #define read nonstd_read
    #define write nonstd_write
    #define fstat nonstd_fstat
    #define lseek nonstd_lseek
    #define readlinkat nonstd_readlinkat
    #define socket nonstd_socket
    #define connect nonstd_connect
    #define bind nonstd_bind
    #define listen nonstd_listen
    #define accept nonstd_accept
    #define time nonstd_time
    #define gettimeofday nonstd_gettimeofday
    #define nanosleep nonstd_nanosleep
    #define getpid nonstd_getpid
    #define getppid nonstd_getppid
    #define kill nonstd_kill
    #define tgkill nonstd_tgkill
    #define mprotect nonstd_mprotect
    #define inotify_init1 nonstd_inotify_init1
    #define inotify_add_watch nonstd_inotify_add_watch
    #define inotify_rm_watch nonstd_inotify_rm_watch
    #define atoi nonstd_atoi
    #define atol nonstd_atol

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