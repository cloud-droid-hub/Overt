#ifndef Z_LIBC_H
#define Z_LIBC_H

#include "zConfig.h"

// 模块配置开关 - 可以通过修改这个宏来控制日志输出
#define ENABLE_NONSTD_API 1

// 当全局配置宏启用时，全局宏配置覆盖模块宏配置
#if ZCONFIG_ENABLE
#undef ENABLE_NONSTD_API
#define ENABLE_NONSTD_API ZCONFIG_ENABLE_NONSTD_API
#endif

// ==================== 系统调用相关 ====================
// 模块配置开关 - 可以通过修改这个宏来控制是否使用自定义系统调用实现
#define ZSYSCALL_ENABLE_NONSTD_API 1

// 当全局配置宏启用时，全局宏配置覆盖模块宏配置
#if ZCONFIG_ENABLE
#undef ZSYSCALL_ENABLE_NONSTD_API
#define ZSYSCALL_ENABLE_NONSTD_API ZCONFIG_ENABLE_NONSTD_API
#endif


#include <sys/syscall.h>
#include <sys/types.h>

// ==================== 系统调用相关结束 ====================

#if ENABLE_NONSTD_API

    #include <sys/socket.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <malloc.h>

#else
    #include <sys/socket.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <malloc.h>

    #include <string.h>
    #include <time.h>
    #include <stdlib.h>
    #include <fcntl.h>
    #include <stdio.h>

#endif



extern "C" {

    // ==================== 系统调用函数 ====================
    long syscall(long __number, ...);

    // ==================== 字符串函数 ====================
    int strcmp(const char *str1, const char *str2);
    size_t strlen(const char *str);
    char *strcpy(char *dest, const char *src);
    char *strcat(char *dest, const char *src);
    int strncmp(const char *str1, const char *str2, size_t n);
    char *strrchr(const char *str, int character);
    char *strncpy(char *dst, const char *src, size_t n);
    size_t strlcpy(char *dst, const char *src, size_t siz);
    char *strstr(const char *s, const char *find);
    char *strchr(const char *p, int ch);

    void *calloc(size_t nmemb, size_t size);
    void *realloc(void *ptr, size_t size);
    void *memset(void *dst, int val, size_t count);
    void *memcpy(void *dst, const void *src, size_t len);
    int memcmp(const void *s1, const void *s2, size_t n);

    // ==================== 文件操作函数 ====================
    int open(const char* pathname, int flags, ...);
    int close(int fd);
    ssize_t read(int fd, void *buf, size_t count);
    ssize_t write(int fd, const void *buf, size_t count);
    int fstat(int __fd, struct stat *__buf);
    off_t lseek(int __fd, off_t __offset, int __whence);
    ssize_t readlinkat(int __dir_fd, const char *__path, char *__buf, size_t __buf_size);
    int access(const char *pathname, int mode);
    int stat(const char *pathname, struct stat *buf);

    // ==================== 网络函数 ====================
    int socket(int domain, int type, int protocol);
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    int listen(int sockfd, int backlog);
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

    // ==================== 时间函数 ====================
    time_t time(time_t *tloc);
    int gettimeofday(struct timeval *tv, struct timezone *tz);

    // ==================== 进程函数 ====================
    pid_t getpid(void);
    pid_t getppid(void);

    // ==================== 信号函数 ====================
    int kill(pid_t pid, int sig);

    // ==================== 其他常用函数 ====================
    int atoi(const char *nptr);
    long atol(const char *nptr);

    // ==================== 扩展系统函数 ====================
    int nanosleep(const struct timespec *__request, struct timespec *__remainder);
    int mprotect(void *__addr, size_t __size, int __prot);
    int inotify_init1(int flags);
    int inotify_add_watch(int __fd, const char *__path, uint32_t __mask);
    int inotify_rm_watch(int __fd, uint32_t __watch_descriptor);
    int tgkill(int __tgid, int __tid, int __signal);
    void exit(int __status);

    ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
    struct tm *localtime(const time_t *timep);
    int stat(const char *__path, struct stat *__buf);
    int access(const char *__path, int __mode);

    void *memmove(void *dest, const void *src, size_t n);
    char *strncat(char *dest, const char *src, size_t n);
    int strcoll(const char *s1, const char *s2);
    size_t strxfrm(char *dest, const char *src, size_t n);
    void *memchr(const void *s, int c, size_t n);
    size_t strcspn(const char *s, const char *reject);
    char *strpbrk(const char *s, const char *accept);
    size_t strspn(const char *s, const char *accept);
    char *strtok(char *str, const char *delim);
    char *strerror(int errnum);

    FILE* popen(const char* cmd, const char* mode);
    int execve(const char* __file, char* const* __argv, char* const* __envp);
}

#endif //Z_LIBC_H