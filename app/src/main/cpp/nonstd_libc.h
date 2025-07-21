//
// Created by lxz on 2025/7/21.
//

#ifndef OVERT_NONSTD_LIBC_H
#define OVERT_NONSTD_LIBC_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>


// ==================== 字符串函数 ====================
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

// ==================== 内存函数 ====================
void* nonstd_malloc(size_t size);
void nonstd_free(void *ptr);
void* nonstd_calloc(size_t nmemb, size_t size);
void* nonstd_realloc(void *ptr, size_t size);
void* nonstd_memset(void *dst, int val, size_t count);
void* nonstd_memcpy(void *dst, const void *src, size_t len);
int nonstd_memcmp(const void *s1, const void *s2, size_t n);

// ==================== 文件操作函数 ====================
int nonstd_open(const char *pathname, int flags, ...);
int nonstd_close(int fd);
ssize_t nonstd_read(int fd, void *buf, size_t count);
ssize_t nonstd_write(int fd, const void *buf, size_t count);
int nonstd_fstat(int __fd, struct stat* __buf);
off_t nonstd_lseek(int __fd, off_t __offset, int __whence);
ssize_t nonstd_readlinkat(int __dir_fd, const char* __path, char* __buf, size_t __buf_size);

// ==================== 网络函数 ====================
int nonstd_socket(int domain, int type, int protocol);
int nonstd_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int nonstd_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int nonstd_listen(int sockfd, int backlog);
int nonstd_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

// ==================== 时间函数 ====================
time_t nonstd_time(time_t *tloc);
int nonstd_gettimeofday(struct timeval *tv, struct timezone *tz);

// ==================== 进程函数 ====================
pid_t nonstd_getpid(void);
pid_t nonstd_getppid(void);

// ==================== 信号函数 ====================
int nonstd_kill(pid_t pid, int sig);

// ==================== 其他常用函数 ====================
int nonstd_atoi(const char *nptr);
long nonstd_atol(const char *nptr);

// ==================== 扩展系统函数 ====================
int nonstd_nanosleep(const struct timespec* __request, struct timespec* __remainder);
int nonstd_mprotect(void* __addr, size_t __size, int __prot);
int nonstd_inotify_init1(int flags);
int nonstd_inotify_add_watch(int __fd, const char *__path, uint32_t __mask);
int nonstd_inotify_rm_watch(int __fd, uint32_t __watch_descriptor);
int nonstd_tgkill(int __tgid, int __tid, int __signal);
void nonstd_exit(int __status);

#ifdef USE_NONSTD_API

// 使用非标准库实现

#else // USE_NONSTD_API

// 宏定义，用于在不使用非标准库的情况下，使用标准库的函数

// ==================== 字符串函数宏定义 ====================
#define nonstd_strcmp strcmp
#define nonstd_strlen strlen
#define nonstd_strcpy strcpy
#define nonstd_strcat strcat
#define nonstd_strncmp strncmp
#define nonstd_strrchr strrchr
#define nonstd_strncpy strncpy
#define nonstd_strlcpy strlcpy
#define nonstd_strstr strstr
#define nonstd_strchr strchr

// ==================== 内存函数宏定义 ====================
#define nonstd_malloc malloc
#define nonstd_free free
#define nonstd_calloc calloc
#define nonstd_realloc realloc
#define nonstd_memset memset
#define nonstd_memcpy memcpy
#define nonstd_memcmp memcmp

// ==================== 文件操作函数宏定义 ====================
#define nonstd_open open
#define nonstd_close close
#define nonstd_read read
#define nonstd_write write
#define nonstd_fstat fstat
#define nonstd_lseek lseek
#define nonstd_readlinkat readlinkat

// ==================== 网络函数宏定义 ====================
#define nonstd_socket socket
#define nonstd_connect connect
#define nonstd_bind bind
#define nonstd_listen listen
#define nonstd_accept accept

// ==================== 时间函数宏定义 ====================
#define nonstd_time time
#define nonstd_gettimeofday gettimeofday

// ==================== 进程函数宏定义 ====================
#define nonstd_getpid getpid
#define nonstd_getppid getppid

// ==================== 信号函数宏定义 ====================
#define nonstd_kill kill

// ==================== 其他常用函数宏定义 ====================
#define nonstd_atoi atoi
#define nonstd_atol atol

// ==================== 扩展系统函数宏定义 ====================
#define nonstd_nanosleep nanosleep
#define nonstd_mprotect mprotect
#define nonstd_inotify_init1 inotify_init1
#define nonstd_inotify_add_watch inotify_add_watch
#define nonstd_inotify_rm_watch inotify_rm_watch
#define nonstd_tgkill tgkill
#define nonstd_exit exit

#endif // USE_NONSTD_API

#endif //OVERT_NONSTD_LIBC_H
