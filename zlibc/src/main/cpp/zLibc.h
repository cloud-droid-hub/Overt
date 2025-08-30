
#ifndef Z_LIBC_H
#define Z_LIBC_H

#include "zConfig.h"

// 模块配置开关 - 可以通过修改这个宏来控制日志输出
#define ZLIBC_ENABLE_NONSTD_API 1

// 当全局配置宏启用时，全局宏配置覆盖模块宏配置
#if ZCONFIG_ENABLE
#undef ZLIBC_ENABLE_NONSTD_API
#define ZLIBC_ENABLE_NONSTD_API ZCONFIG_ENABLE_NONSTD_API
#endif

#if ZLIBC_ENABLE_NONSTD_API

#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define zlibc_strcmp strcmp
#define zlibc_strlen strlen
#define zlibc_strcpy strcpy
#define zlibc_strcat strcat
#define zlibc_strncmp strncmp
#define zlibc_strrchr strrchr
#define zlibc_strncpy strncpy
#define zlibc_strlcpy strlcpy
#define zlibc_strstr strstr
#define zlibc_strchr strchr

// ==================== 内存函数宏定义 ====================
#include <malloc.h>
// libc 的 malloc 是有缓存优化的，所以慎重启用这个四个内存分配相关的函数，会严重消耗性能
//#define zlibc_malloc malloc
//#define zlibc_free free
//#define zlibc_calloc calloc
//#define zlibc_realloc realloc

#define zlibc_memset memset
#define zlibc_memcpy memcpy
#define zlibc_memcmp memcmp

// ==================== 文件操作函数宏定义 ====================
#define zlibc_open open
#define zlibc_close close
#define zlibc_read read
#define zlibc_write write
#define zlibc_fstat fstat
#define zlibc_lseek lseek
#define zlibc_readlinkat readlinkat

// ==================== 网络函数宏定义 ====================
#define zlibc_socket socket
#define zlibc_connect connect
#define zlibc_fcntl fcntl
#define zlibc_bind bind
#define zlibc_listen listen
#define zlibc_accept accept

// ==================== 时间函数宏定义 ====================
#define zlibc_time time
#define zlibc_gettimeofday gettimeofday

// ==================== 进程函数宏定义 ====================
#define zlibc_getpid getpid
#define zlibc_getppid getppid

// ==================== 信号函数宏定义 ====================
#define zlibc_kill kill

// ==================== 其他常用函数宏定义 ====================
#define zlibc_atoi atoi
#define zlibc_atol atol

//// ==================== 扩展系统函数宏定义 ====================
#define zlibc_nanosleep nanosleep
#define zlibc_mprotect mprotect
#define zlibc_inotify_init1 inotify_init1
#define zlibc_inotify_add_watch inotify_add_watch
#define zlibc_inotify_rm_watch inotify_rm_watch
#define zlibc_tgkill tgkill
#define zlibc_exit exit
#define zlibc_readlink readlink
#define zlibc_localtime localtime
#define zlibc_stat stat
#define zlibc_access access

#define zlibc_memmove memmove
#define zlibc_strncat strncat
#define zlibc_strcoll strcoll
#define zlibc_strxfrm strxfrm
#define zlibc_memchr memchr
#define zlibc_strcspn strcspn
#define zlibc_strpbrk strpbrk
#define zlibc_strspn strspn
#define zlibc_strtok strtok
#define zlibc_strerror strerror
#define zlibc_popen popen
#define zlibc_execve execve

#else
    #include <string.h>
    #include <malloc.h>
    #include <time.h>
    #include <stdlib.h>
    #include <sys/socket.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <stdio.h>
#endif



#include <sys/types.h>

extern "C" {
// ==================== 字符串函数 ====================
    int zlibc_strcmp(const char *str1, const char *str2);
    size_t zlibc_strlen(const char *str);
    char *zlibc_strcpy(char *dest, const char *src);
    char *zlibc_strcat(char *dest, const char *src);
    int zlibc_strncmp(const char *str1, const char *str2, size_t n);
    char *zlibc_strrchr(const char *str, int character);
    char *zlibc_strncpy(char *dst, const char *src, size_t n);
    size_t zlibc_strlcpy(char *dst, const char *src, size_t siz);
    char *zlibc_strstr(const char *s, const char *find);
    char *zlibc_strchr(const char *p, int ch);

    // ==================== 内存函数 ====================
    void *zlibc_malloc(size_t size);
    void zlibc_free(void *ptr);
    void *zlibc_calloc(size_t nmemb, size_t size);
    void *zlibc_realloc(void *ptr, size_t size);
    void *zlibc_memset(void *dst, int val, size_t count);
    void *zlibc_memcpy(void *dst, const void *src, size_t len);
    int zlibc_memcmp(const void *s1, const void *s2, size_t n);

    // ==================== 文件操作函数 ====================

    int zlibc_open(const char* pathname, int flags, ...);
    int zlibc_close(int fd);
    ssize_t zlibc_read(int fd, void *buf, size_t count);
    ssize_t zlibc_write(int fd, const void *buf, size_t count);
    int zlibc_fstat(int __fd, struct stat *__buf);
    off_t zlibc_lseek(int __fd, off_t __offset, int __whence);
    ssize_t zlibc_readlinkat(int __dir_fd, const char *__path, char *__buf, size_t __buf_size);
    int zlibc_access(const char *pathname, int mode);
    int zlibc_stat(const char *pathname, struct stat *buf);

    // ==================== 网络函数 ====================
    int zlibc_socket(int domain, int type, int protocol);
    int zlibc_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    int zlibc_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    int zlibc_listen(int sockfd, int backlog);
    int zlibc_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

    // ==================== 时间函数 ====================
    time_t zlibc_time(time_t *tloc);
    int zlibc_gettimeofday(struct timeval *tv, struct timezone *tz);

    // ==================== 进程函数 ====================
    pid_t zlibc_getpid(void);
    pid_t zlibc_getppid(void);

    // ==================== 信号函数 ====================
    int zlibc_kill(pid_t pid, int sig);

    // ==================== 其他常用函数 ====================
    int zlibc_atoi(const char *nptr);
    long zlibc_atol(const char *nptr);

    // ==================== 扩展系统函数 ====================
    int zlibc_nanosleep(const struct timespec *__request, struct timespec *__remainder);
    int zlibc_mprotect(void *__addr, size_t __size, int __prot);
    int zlibc_inotify_init1(int flags);
    int zlibc_inotify_add_watch(int __fd, const char *__path, uint32_t __mask);
    int zlibc_inotify_rm_watch(int __fd, uint32_t __watch_descriptor);
    int zlibc_tgkill(int __tgid, int __tid, int __signal);
    void zlibc_exit(int __status);

    ssize_t zlibc_readlink(const char *pathname, char *buf, size_t bufsiz);
    struct tm *zlibc_localtime(const time_t *timep);
    int zlibc_stat(const char *__path, struct stat *__buf);
    int zlibc_access(const char *__path, int __mode);

    void *zlibc_memmove(void *dest, const void *src, size_t n);
    char *zlibc_strncat(char *dest, const char *src, size_t n);
    int zlibc_strcoll(const char *s1, const char *s2);
    size_t zlibc_strxfrm(char *dest, const char *src, size_t n);
    void *zlibc_memchr(const void *s, int c, size_t n);
    size_t zlibc_strcspn(const char *s, const char *reject);
    char *zlibc_strpbrk(const char *s, const char *accept);
    size_t zlibc_strspn(const char *s, const char *accept);
    char *zlibc_strtok(char *str, const char *delim);
    char *zlibc_strerror(int errnum);

    FILE* zlibc_popen(const char* cmd, const char* mode);
    int zlibc_execve(const char* __file, char* const* __argv, char* const* __envp);
}

#endif //Z_LIBC_H