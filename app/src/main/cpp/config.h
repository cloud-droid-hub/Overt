#ifndef CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>

// ==================== 命名空间控制 ====================
// 通过这个宏来控制使用哪个命名空间
// 定义 USE_NONSTD_API 时使用 nonstd 命名空间
// 未定义时使用 std 命名空间

#define USE_NONSTD_API

#ifdef USE_NONSTD_API

    #include "string.h"
    #include "vector.h"
    #include "map.h"

    extern "C" {
        // 字符串函数
        int nonstd_strcmp(const char *str1, const char *str2);
        size_t nonstd_strlen(const char *str);
        char* nonstd_strcpy(char *dest, const char *src);
        char* nonstd_strcat(char *dest, const char *src);
        int nonstd_strncmp(const char *str1, const char *str2, size_t n);
        const char* nonstd_strrchr(const char *str, int character);
        char* nonstd_strncpy(char *dst, const char *src, size_t n);
        size_t nonstd_strlcpy(char *dst, const char *src, size_t siz);
        const char* nonstd_strstr(const char *s, const char *find);
        const char* nonstd_strchr(const char *p, int ch);

        void* nonstd_memset(void *dst, int val, size_t count);
        void* nonstd_memcpy(void *dst, const void *src, size_t len);
        
        // 内存管理函数
        void* nonstd_malloc(size_t size);
        void nonstd_free(void *ptr);
        void* nonstd_calloc(size_t nmemb, size_t size);
        void* nonstd_realloc(void *ptr, size_t size);
        
        // 文件操作函数
        int nonstd_open(const char *pathname, int flags, ...);
        int nonstd_close(int fd);
        ssize_t nonstd_read(int fd, void *buf, size_t count);
        ssize_t nonstd_write(int fd, const void *buf, size_t count);
        int nonstd_fstat(int __fd, struct stat* __buf);
        off_t nonstd_lseek(int __fd, off_t __offset, int __whence);
        ssize_t nonstd_readlinkat(int __dir_fd, const char* __path, char* __buf, size_t __buf_size);
        
        // 网络函数
        int nonstd_socket(int domain, int type, int protocol);
        int nonstd_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
        int nonstd_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
        int nonstd_listen(int sockfd, int backlog);
        int nonstd_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
        
        // 时间函数
        time_t nonstd_time(time_t *tloc);
        int nonstd_gettimeofday(struct timeval *tv, struct timezone *tz);
        int nonstd_nanosleep(const struct timespec* __request, struct timespec* __remainder);
        
        // 进程函数
        pid_t nonstd_getpid(void);
        pid_t nonstd_getppid(void);
        
        // 信号函数
        int nonstd_kill(pid_t pid, int sig);
        int nonstd_tgkill(int __tgid, int __tid, int __signal);
        
        // 扩展系统函数
        int nonstd_mprotect(void* __addr, size_t __size, int __prot);
        int nonstd_inotify_init1(int flags);
        int nonstd_inotify_add_watch(int __fd, const char *__path, uint32_t __mask);
        int nonstd_inotify_rm_watch(int __fd, uint32_t __watch_descriptor);
        
        // 其他函数
        int nonstd_atoi(const char *nptr);
        long nonstd_atol(const char *nptr);
    }

    using namespace nonstd;

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
    using std::make_pair;
    using ::memcpy;
    using ::memset;
    using ::strcmp;
    using ::strlen;
    using ::strcpy;
    using ::strcat;
    using ::malloc;
    using ::free;
    using ::calloc;
    using ::realloc;
#endif

#endif // CONFIG_H