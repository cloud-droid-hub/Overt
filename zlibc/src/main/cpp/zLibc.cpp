// 下面这些头文件只包含宏定义，没有函数定义
//#include <asm-generic/unistd.h>
#include <linux/fcntl.h>
#include <errno.h>
#include <linux/time.h>
#include <linux/mman.h>
#include <stdarg.h>

#include "zLog.h"
#include "zLibc.h"

#define MAP_FAILED (void*)(-1)
#define PAGE_ALIGN(x) (((x) + 0xFFF) & ~0xFFF)

#if ENABLE_NONSTD_API

// ==================== 系统调用函数 ====================

// 内联汇编实现的 syscall 函数
// ARM64 调用约定：x0-x7 用于参数传递，x8 用于系统调用号
long syscall(long __number, ...) {
    long result;
    long arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8;
    
    va_list args;
    va_start(args, __number);
    
    // 读取可变参数（最多 8 个：x0-x7）
    arg1 = va_arg(args, long);
    arg2 = va_arg(args, long);
    arg3 = va_arg(args, long);
    arg4 = va_arg(args, long);
    arg5 = va_arg(args, long);
    arg6 = va_arg(args, long);
    arg7 = va_arg(args, long);
    arg8 = va_arg(args, long);
    
    va_end(args);
    
    // 内联汇编：执行系统调用
    __asm__ __volatile__(
        "mov x8, %1\n\t"        // x8 = syscall number
        "mov x0, %2\n\t"        // x0 = arg1
        "mov x1, %3\n\t"        // x1 = arg2
        "mov x2, %4\n\t"        // x2 = arg3
        "mov x3, %5\n\t"        // x3 = arg4
        "mov x4, %6\n\t"        // x4 = arg5
        "mov x5, %7\n\t"        // x5 = arg6
        "mov x6, %8\n\t"        // x6 = arg7
        "mov x7, %9\n\t"        // x7 = arg8
        "svc #0\n\t"            // 进入内核
        "mov %0, x0"            // result = x0
        : "=r" (result)
        : "r" (__number), "r" (arg1), "r" (arg2), "r" (arg3), "r" (arg4), "r" (arg5), "r" (arg6), "r" (arg7), "r" (arg8)
        : "x0", "x8", "memory", "cc"
    );
    
    // 检查错误返回 (-4095 ~ -1)
    // Linux 系统调用错误返回范围是 -4095 到 -1
    if (result > -4095L && result < 0) {
        errno = -result;
        return -1;
    }
    
    return result;
}

// 手动实现strcmp函数 - 自定义版本
int strcmp(const char *str1, const char *str2) {
    LOGV("strcmp called: str1='%s', str2='%s'", str1 ? str1 : "NULL", str2 ? str2 : "NULL");

    // 处理NULL指针
    if (!str1 && !str2) {
        LOGV("Both strings are NULL, returning 0");
        return 0;
    }
    if (!str1) {
        LOGV("str1 is NULL, returning -1");
        return -1;
    }
    if (!str2) {
        LOGV("str2 is NULL, returning 1");
        return 1;
    }

    // 手动实现strcmp逻辑
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            int result = (unsigned char)str1[i] - (unsigned char)str2[i];
            LOGV("Strings differ at position %d: '%c' vs '%c', returning %d", i, str1[i], str2[i], result);
            return result;
        }
        i++;
    }

    // 检查字符串长度
    if (str1[i] == '\0' && str2[i] == '\0') {
        LOGV("Strings are identical, returning 0");
        return 0;
    } else if (str1[i] == '\0') {
        LOGV("str1 is shorter, returning -1");
        return -1;
    } else {
        LOGV("str2 is shorter, returning 1");
        return 1;
    }
}

// ==================== 字符串函数 ====================

size_t strlen(const char *str) {
    LOGV("strlen called: str='%s'", str ? str : "NULL");

    if (!str) {
        LOGV("strlen: NULL pointer, returning 0");
        return 0;
    }

    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }

    LOGV("strlen: length = %zu", len);
    return len;
}

char* strcpy(char *dest, const char *src) {
    LOGV("strcpy called: dest=%p, src='%s'", dest, src ? src : "NULL");

    if (!dest || !src) {
        LOGV("strcpy: NULL pointer");
        return dest;
    }

    char *d = dest;
    while ((*d++ = *src++) != '\0');

    LOGV("strcpy: copied '%s'", dest);
    return dest;
}

char* strcat(char *dest, const char *src) {
    LOGV("strcat called: dest='%s', src='%s'", dest ? dest : "NULL", src ? src : "NULL");

    if (!dest || !src) {
        LOGV("strcat: NULL pointer");
        return dest;
    }

    char *d = dest;
    while (*d != '\0') d++;
    while ((*d++ = *src++) != '\0');

    LOGV("strcat: result = '%s'", dest);
    return dest;
}

int strncmp(const char *str1, const char *str2, size_t n) {
    LOGV("strncmp called: str1='%s', str2='%s', n=%zu",
         str1 ? str1 : "NULL", str2 ? str2 : "NULL", n);

    if (n == 0) {
        LOGV("strncmp: n=0, returning 0");
        return 0;
    }

    do {
        if (*str1 != *str2++) {
            int result = (*(unsigned char *)str1 - *(unsigned char *)(--str2));
            LOGV("strncmp: diff, returning %d", result);
            return result;
        }
        if (*str1++ == 0) break;
    } while (--n != 0);

    LOGV("strncmp: equal, returning 0");
    return 0;
}

// ==================== 内存函数 ====================

typedef struct {
    size_t size;
    uint8_t data[];  // C99 flexible array member
} MemHeader;

void* calloc(size_t nmemb, size_t size) {
    LOGV("calloc called: nmemb=%zu, size=%zu", nmemb, size);

    // 溢出检查
    if (nmemb == 0 || size == 0 || nmemb > SIZE_MAX / size) {
        LOGV("calloc: invalid allocation size, returning NULL");
        return NULL;
    }

    size_t total_size = nmemb * size;
    void* ptr = malloc(total_size);
    if (ptr) {
        // 可自定义清零，也可用如下低级写法替代 memset
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < total_size; ++i) {
            p[i] = 0;
        }
        LOGV("calloc: allocated and zeroed %zu bytes at %p", total_size, ptr);
    }

    return ptr;
}

void* realloc(void* ptr, size_t size) {
    LOGV("realloc called: ptr=%p, size=%zu", ptr, size);

    if (!ptr) {
        return malloc(size);  // 相当于 malloc
    }

    if (size == 0) {
        free(ptr);            // 相当于 free
        return NULL;
    }

    // 获取旧块大小
    MemHeader* old_header = (MemHeader*)((char*)ptr - offsetof(MemHeader, data));
    size_t old_size = old_header->size - sizeof(MemHeader);  // 原用户可用大小

    void* new_ptr = malloc(size);
    if (!new_ptr) return NULL;

    // 复制较小的那一部分
    size_t copy_size = (size < old_size) ? size : old_size;
    const uint8_t* src = (const uint8_t*)ptr;
    uint8_t* dst = (uint8_t*)new_ptr;
    for (size_t i = 0; i < copy_size; ++i) {
        dst[i] = src[i];
    }

    free(ptr);
    LOGV("realloc: copied %zu bytes to new block %p", copy_size, new_ptr);
    return new_ptr;
}

// ==================== 文件操作函数 ====================

int open(const char* pathname, int flags, ...) {
    LOGD("open called: pathname='%s', flags=0x%x", pathname ? pathname : "NULL", flags);

    if (!pathname) {
        LOGV("open: NULL pathname");
        return -1;
    }

    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    // 使用 openat 系统调用，AT_FDCWD 表示相对于当前工作目录
    int fd = (int)syscall(SYS_openat, AT_FDCWD, (long)pathname, flags, mode);
    LOGV("open: returned fd=%d", fd);
    return fd;
}

int close(int fd) {
    LOGV("close called: fd=%d", fd);

    int result = (int)syscall(SYS_close, fd);
    LOGV("close: result=%d", result);
    return result;
}

ssize_t read(int fd, void *buf, size_t count) {
    LOGV("read called: fd=%d, buf=%p, count=%zu", fd, buf, count);

    if (!buf) {
        LOGV("read: NULL buffer");
        return -1;
    }

    ssize_t result = syscall(SYS_read, fd, (long)buf, count);
    LOGV("read: result=%zd", result);
    return result;
}

ssize_t write(int fd, const void *buf, size_t count) {
    LOGV("write called: fd=%d, buf=%p, count=%zu", fd, buf, count);

    if (!buf) {
        LOGV("write: NULL buffer");
        return -1;
    }

    ssize_t result = syscall(SYS_write, fd, (long)buf, count);
    LOGV("write: result=%zd", result);
    return result;
}

// ==================== 网络函数 ====================

int socket(int domain, int type, int protocol) {
    LOGV("socket called: domain=%d, type=%d, protocol=%d", domain, type, protocol);

    // socket syscall 参数顺序：domain, type, protocol
    long sock = syscall(SYS_socket, (long)domain, (long)type, (long)protocol);

    if (sock < 0) {
        LOGV("socket: syscall failed, errno=%d", -sock);
        return -1;
    }

    LOGV("socket: created sockfd=%ld", sock);
    return (int)sock;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    LOGV("connect called: sockfd=%d, addr=%p, addrlen=%u", sockfd, addr, addrlen);

    // connect 参数顺序：sockfd, addr, addrlen
    long result = syscall(SYS_connect, (long)sockfd, (long)addr, (long)addrlen);

    if (result < 0) {
        LOGV("connect: syscall failed, errno=%ld", -result);
        return -1;
    }

    LOGV("connect: success");
    return 0;
}



int fcntl(int fd, int cmd, ...) {
    LOGV("fcntl called: fd=%d, cmd=%d", fd, cmd);

    va_list args;
    va_start(args, cmd);

    long arg = 0;
    // 某些 fcntl 命令需要第三个参数
    switch (cmd) {
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETFL:
        case F_SETLK:
        case F_SETLKW:
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        case F_GETLEASE:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
        case F_ADD_SEALS:
        case F_GET_RW_HINT:
        case F_SET_RW_HINT:
            arg = va_arg(args, long); // 取第三个参数
            break;
        default:
            arg = 0; // 默认无参数
            break;
    }

    va_end(args);

    long result = syscall(SYS_fcntl, (long)fd, (long)cmd, arg);

    if (result < 0) {
        LOGV("fcntl: syscall failed, errno=%ld", -result);
        errno = -result;
        return -1;
    }

    LOGV("fcntl: success, result=%ld", result);
    return (int)result;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    LOGV("bind called: sockfd=%d, addr=%p, addrlen=%u", sockfd, addr, addrlen);

    int result = (int)syscall(SYS_bind, sockfd, (long)addr, addrlen);
    LOGV("bind: result=%d", result);
    return result;
}

int listen(int sockfd, int backlog) {
    LOGV("listen called: sockfd=%d, backlog=%d", sockfd, backlog);

    int result = (int)syscall(SYS_listen, sockfd, backlog);
    LOGV("listen: result=%d", result);
    return result;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    LOGV("accept called: sockfd=%d, addr=%p, addrlen=%p", sockfd, addr, addrlen);

    int result = (int)syscall(SYS_accept, sockfd, (long)addr, (long)addrlen);
    LOGV("accept: result=%d", result);
    return result;
}

// ==================== 时间函数 ====================

time_t time(time_t *tloc) {
    LOGV("time called: tloc=%p", tloc);

    // 在 ARM64 上，time 系统调用已被废弃，使用 clock_gettime 替代
    struct timespec ts;

    int result = syscall(SYS_clock_gettime, CLOCK_REALTIME, (long)&ts);


    if (result != 0) {
        return -1;
    }

    time_t time_result = (time_t)ts.tv_sec;
    if (tloc) {
        *tloc = time_result;
    }

    return time_result;
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    LOGV("gettimeofday called: tv=%p, tz=%p", tv, tz);

    int result = (int)syscall(SYS_gettimeofday, (long)tv, (long)tz);
    LOGV("gettimeofday: result=%d", result);
    return result;
}

// ==================== 进程函数 ====================

pid_t getpid(void) {
    LOGV("getpid called");

    pid_t result = (pid_t)syscall(SYS_getpid);
    LOGV("getpid: result=%d", result);
    return result;
}

pid_t getppid(void) {
    LOGV("getppid called");

    // getppid系统调用号
    pid_t result = (pid_t)syscall(173); // SYS_getppid
    LOGV("getppid: result=%d", result);
    return result;
}

// ==================== 信号函数 ====================

int kill(pid_t pid, int sig) {
    LOGV("kill called: pid=%d, sig=%d", pid, sig);

    int result = (int)syscall(SYS_kill, pid, sig);
    LOGV("kill: result=%d", result);
    return result;
}

// ==================== 其他常用函数 ====================

int atoi(const char *nptr) {
    LOGD("atoi called: nptr='%s'", nptr ? nptr : "NULL");

    if (!nptr) {
        LOGV("atoi: NULL pointer, returning 0");
        return 0;
    }

    int result = 0;
    int sign = 1;
    int i = 0;

    // 跳过空白字符
    while (nptr[i] == ' ' || nptr[i] == '\t' || nptr[i] == '\n' ||
           nptr[i] == '\r' || nptr[i] == '\f' || nptr[i] == '\v') {
        i++;
    }

    // 处理符号
    if (nptr[i] == '+' || nptr[i] == '-') {
        sign = (nptr[i] == '-') ? -1 : 1;
        i++;
    }

    // 转换数字
    while (nptr[i] >= '0' && nptr[i] <= '9') {
        result = result * 10 + (nptr[i] - '0');
        i++;
    }

    result *= sign;
    LOGD("atoi: result=%d", result);
    return result;
}

long atol(const char *nptr) {
    LOGV("atol called: nptr='%s'", nptr ? nptr : "NULL");

    if (!nptr) {
        LOGV("atol: NULL pointer, returning 0");
        return 0;
    }

    long result = 0;
    long sign = 1;
    int i = 0;

    // 跳过空白字符
    while (nptr[i] == ' ' || nptr[i] == '\t' || nptr[i] == '\n' ||
           nptr[i] == '\r' || nptr[i] == '\f' || nptr[i] == '\v') {
        i++;
    }

    // 处理符号
    if (nptr[i] == '+' || nptr[i] == '-') {
        sign = (nptr[i] == '-') ? -1 : 1;
        i++;
    }

    // 转换数字
    while (nptr[i] >= '0' && nptr[i] <= '9') {
        result = result * 10 + (nptr[i] - '0');
        i++;
    }

    result *= sign;
    LOGV("atol: result=%ld", result);
    return result;
}

// ==================== 扩展字符串函数 ====================

char* strrchr(const char *str, int character) {
    LOGV("strrchr called: str='%s', character='%c'", str ? str : "NULL", character);

    const char *ptr = nullptr;
    while (*str != '\0') {
        if (*str == (char) character) {
            ptr = str;
        }
        str++;
    }

    LOGV("strrchr: result=%p", (void*)ptr);
    return (char*)ptr;
}

char* strncpy(char *dst, const char *src, size_t n) {
    LOGV("strncpy called: dst=%p, src='%s', n=%zu", dst, src ? src : "NULL", n);

    if (n != 0) {
        char *d = dst;
        const char *s = src;

        do {
            if ((*d++ = *s++) == 0) {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                    *d++ = 0;
                break;
            }
        } while (--n != 0);
    }

    LOGV("strncpy: result='%s'", dst);
    return (dst);
}

size_t strlcpy(char *dst, const char *src, size_t siz) {
    LOGV("strlcpy called: dst=%p, src='%s', siz=%zu", dst, src ? src : "NULL", siz);

    char *d = dst;
    const char *s = src;
    size_t n = siz;
    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }
    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';        /* NUL-terminate dst */
        while (*s++);
    }

    size_t result = (s - src - 1);    /* count does not include NUL */
    LOGV("strlcpy: result=%zu", result);
    return result;
}

char* strstr(const char *s, const char *find) {
    LOGV("strstr called: s='%s', find='%s'", s ? s : "NULL", find ? find : "NULL");

    char c, sc;
    size_t len;
    if ((c = *find++) != '\0') {
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == '\0')
                    return (nullptr);
            } while (sc != c);
        } while (strncmp(s, find, len) != 0);
        s--;
    }

    LOGV("strstr: result='%s'", s);
    return ((char *) s);
}

void* memset(void *dst, int val, size_t count) {
    LOGV("memset called: dst=%p, val=%d, count=%zu", dst, val, count);

    char *ptr = (char*)dst;
    while (count--)
        *ptr++ = val;

    LOGV("memset: completed");
    return dst;
}

void* memcpy(void *dst, const void *src, size_t len) {
    LOGV("memcpy called: dst=%p, src=%p, len=%zu", dst, src, len);

    const char* s = (const char*)src;
    char *d = (char*)dst;
    while (len--)
        *d++ = *s++;

    LOGV("memcpy: completed");
    return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    LOGV("memcmp called: s1=%p, s2=%p, n=%zu", s1, s2, n);

    if (!s1 && !s2) {
        LOGV("memcmp: both pointers are NULL, returning 0");
        return 0;
    }
    if (!s1) {
        LOGV("memcmp: s1 is NULL, returning -1");
        return -1;
    }
    if (!s2) {
        LOGV("memcmp: s2 is NULL, returning 1");
        return 1;
    }

    const unsigned char *p1 = static_cast<const unsigned char*>(s1);
    const unsigned char *p2 = static_cast<const unsigned char*>(s2);

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            int result = static_cast<int>(p1[i]) - static_cast<int>(p2[i]);
            LOGV("memcmp: difference at position %zu: %u vs %u, returning %d", i, p1[i], p2[i], result);
            return result;
        }
    }

    LOGV("memcmp: identical, returning 0");
    return 0;
}

char* strchr(const char *p, int ch) {
    LOGV("strchr called: p='%s', ch='%c'", p ? p : "NULL", ch);

    for (;; ++p) {
        if (*p == static_cast<char>(ch)) {
            LOGV("strchr: found at %p", (void*)p);
            return const_cast<char *>(p);
        }
        if (*p == '\0') {
            LOGV("strchr: not found");
            return nullptr;
        }
    }
    return nullptr;
}

// ==================== 扩展文件操作函数 ====================
#include <bits/glibc-syscalls.h>

int fstat(int __fd, struct stat* __buf) {
    LOGV("fstat called: fd=%d, buf=%p", __fd, __buf);

    int result = syscall(SYS_fstat, __fd, (long)__buf);
    LOGV("fstat: result=%d", result);
    return result;
}

off_t lseek(int __fd, off_t __offset, int __whence) {
    LOGV("lseek called: fd=%d, offset=%ld, whence=%d", __fd, __offset, __whence);

    off_t result = syscall(SYS_lseek, __fd, __offset, __whence);
    LOGV("lseek: result=%ld", result);
    return result;
}

ssize_t readlinkat(int __dir_fd, const char* __path, char* __buf, size_t __buf_size) {
    LOGV("readlinkat called: dir_fd=%d, path='%s', buf=%p, buf_size=%zu",
         __dir_fd, __path ? __path : "NULL", __buf, __buf_size);

    ssize_t result = syscall(SYS_readlinkat, __dir_fd, (long)__path, (long)__buf, (long)__buf_size);
    LOGV("readlinkat: result=%zd", result);
    return result;
}

// ==================== 扩展系统函数 ====================

int nanosleep(const struct timespec* __request, struct timespec* __remainder) {
    LOGV("nanosleep called: request=%p, remainder=%p", __request, __remainder);

    int result = (int)syscall(SYS_nanosleep, (long)__request, (long)__remainder);
    LOGV("nanosleep: result=%d", result);
    return result;
}

int mprotect(void* __addr, size_t __size, int __prot) {
    LOGV("mprotect called: addr=%p, size=%zu, prot=%d", __addr, __size, __prot);

    int result = (int)syscall(SYS_mprotect, (long)__addr, (long)__size, (long)__prot);
    LOGV("mprotect: result=%d", result);
    return result;
}

int inotify_init1(int flags) {
    LOGV("inotify_init1 called: flags=%d", flags);

    int result = syscall(SYS_inotify_init1, flags);
    LOGV("inotify_init1: result=%d", result);
    return result;
}

int inotify_add_watch(int __fd, const char *__path, uint32_t __mask) {
    LOGV("inotify_add_watch called: fd=%d, path='%s', mask=%u", __fd, __path ? __path : "NULL", __mask);

    int result = syscall(SYS_inotify_add_watch, __fd, (long)__path, (long)__mask);
    LOGV("inotify_add_watch: result=%d", result);
    return result;
}

int inotify_rm_watch(int __fd, uint32_t __watch_descriptor) {
    LOGV("inotify_rm_watch called: fd=%d, watch_descriptor=%u", __fd, __watch_descriptor);

    int result = syscall(SYS_inotify_rm_watch, __fd, (long)__watch_descriptor);
    LOGV("inotify_rm_watch: result=%d", result);
    return result;
}

int tgkill(int __tgid, int __tid, int __signal) {
    LOGV("tgkill called: tgid=%d, tid=%d, signal=%d", __tgid, __tid, __signal);

    int result = (int)syscall(SYS_tgkill, __tgid, __tid, __signal);
    LOGV("tgkill: result=%d", result);
    return result;
}

void exit(int __status) {
    LOGV("exit called: status=%d", __status);

    syscall(SYS_exit, __status);
    // 这行代码永远不会执行
    return;
}


// ==================== 时间相关 ====================


ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    LOGV("readlink called: pathname='%s', buf=%p, bufsiz=%zu", pathname ? pathname : "NULL", buf, bufsiz);
    if (!pathname || !buf) {
        LOGV("readlink: NULL arg");
        return -1;
    }
    ssize_t result = (ssize_t)syscall(SYS_readlinkat, AT_FDCWD, (long)pathname, (long)buf, (long)bufsiz);
    LOGV("readlink: result=%zd", result);
    return result;
}

int stat(const char* __path, struct stat* __buf) {
    LOGV("stat called: path='%s', buf=%p", __path ? __path : "NULL", __buf);
    if (!__path || !__buf) {
        LOGV("stat: NULL arg");
        return -1;
    }
    int result = (int)syscall(SYS_newfstatat, AT_FDCWD, (long)__path, (long)__buf, 0);
    LOGV("stat: result=%d", result);
    return result;
}

int access(const char* __path, int __mode) {
    LOGV("access called: path='%s', mode=0x%x", __path ? __path : "NULL", __mode);

    if (!__path) {
        LOGV("access: NULL path");
        errno = EFAULT;  // 错误地址
        return -1;
    }

    long result = syscall(SYS_faccessat, AT_FDCWD, (uintptr_t)__path, (long)__mode, 0);

    if (result < 0) {
        errno = -result;
        LOGV("access: failed, errno=%d", errno);
        return -1;
    }

    LOGV("access: success");
    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    auto *d = static_cast<unsigned char *>(dest);
    const auto *s = static_cast<const unsigned char *>(src);

    if (d == s || n == 0) return dest;

    if (d < s) {
        for (size_t i = 0; i < n; ++i) d[i] = s[i];
    } else {
        for (size_t i = n; i != 0; --i) d[i - 1] = s[i - 1];
    }
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) ++d;
    while (n-- && *src) *d++ = *src++;
    *d = '\0';
    return dest;
}

int strcoll(const char *s1, const char *s2) {
    return strcmp(s1, s2);  // 或自己实现非 locale 版本
}

size_t strxfrm(char *dest, const char *src, size_t n) {
    size_t len = strlen(src);
    if (n > 0) {
        size_t copy_len = (len < n - 1) ? len : n - 1;
        memcpy(dest, src, copy_len);
        dest[copy_len] = '\0';
    }
    return len;
}
void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = static_cast<const unsigned char *>(s);
    for (size_t i = 0; i < n; ++i) {
        if (p[i] == static_cast<unsigned char>(c)) {
            return const_cast<unsigned char *>(&p[i]);
        }
    }
    return nullptr;
}
size_t strcspn(const char *s, const char *reject) {
    size_t i = 0;
    while (s[i]) {
        for (size_t j = 0; reject[j]; ++j) {
            if (s[i] == reject[j]) return i;
        }
        ++i;
    }
    return i;
}
char *strpbrk(const char *s, const char *accept) {
    for (; *s; ++s) {
        for (const char *a = accept; *a; ++a) {
            if (*s == *a) return const_cast<char *>(s);
        }
    }
    return nullptr;
}
size_t strspn(const char *s, const char *accept) {
    size_t i = 0;
    for (; s[i]; ++i) {
        bool found = false;
        for (const char *a = accept; *a; ++a) {
            if (s[i] == *a) {
                found = true;
                break;
            }
        }
        if (!found) return i;
    }
    return i;
}

char *strtok(char *str, const char *delim) {
    static char *next = nullptr;
    if (!str) str = next;
    if (!str) return nullptr;

    // 跳过分隔符
    str += strspn(str, delim);
    if (!*str) {
        next = nullptr;
        return nullptr;
    }

    char *end = str + strcspn(str, delim);
    if (*end) {
        *end = '\0';
        next = end + 1;
    } else {
        next = nullptr;
    }
    return str;
}

char *strerror(int errnum) {
    static const char *errors[] = {
            "Success",              // 0
            "Operation not permitted",
            "No such file or directory",
            "No such process",
            "Interrupted system call",
            "I/O error",
            "No such device or address",
            "Argument list too long",
            "Exec format error",
            "Bad file descriptor"
            // 可根据需要扩展
    };
    static const size_t count = sizeof(errors) / sizeof(errors[0]);
    if (errnum >= 0 && static_cast<size_t>(errnum) < count) {
        return (char*)errors[errnum];
    }
    return "Unknown error";
}

int execve(const char* __file, char* const* __argv, char* const* __envp) {
    register long x8 __asm__("x8") = SYS_execve;
    register long x0 __asm__("x0") = (long)__file;
    register long x1 __asm__("x1") = (long)__argv;
    register long x2 __asm__("x2") = (long)__envp;

    __asm__ __volatile__(
            "svc #0\n\t"
            : "=r"(x0)
            : "r"(x8), "0"(x0), "r"(x1), "r"(x2)
            : "memory", "cc"
            );

    // 错误处理
    if (x0 > -4095) {
        errno = -x0;
        return -1;
    }

    return x0;
}

static FILE* __popen_fail(int fds[2]) {
    close(fds[0]);
    close(fds[1]);
    return nullptr;
}

FILE* popen(const char* cmd, const char* mode) {
    LOGI("popen called: cmd=%s, mode=%s", cmd , mode);
    // Was the request for a socketpair or just a pipe?
    int fds[2];
    bool bidirectional = false;
    if (strchr(mode, '+') != nullptr) {
        if (socketpair(AF_LOCAL, SOCK_CLOEXEC | SOCK_STREAM, 0, fds) == -1) return nullptr;
        bidirectional = true;
        mode = "r+";
    } else {
        if (pipe2(fds, O_CLOEXEC) == -1) return nullptr;
        mode = strrchr(mode, 'r') ? "r" : "w";
    }

    // If the parent wants to read, the child's fd needs to be stdout.
    int parent, child, desired_child_fd;
    if (*mode == 'r') {
        parent = 0;
        child = 1;
        desired_child_fd = STDOUT_FILENO;
    } else {
        parent = 1;
        child = 0;
        desired_child_fd = STDIN_FILENO;
    }

    // Ensure that the child fd isn't the desired child fd.
    if (fds[child] == desired_child_fd) {
        int new_fd = fcntl(fds[child], F_DUPFD_CLOEXEC, 0);
        if (new_fd == -1) return __popen_fail(fds);
        close(fds[child]);
        fds[child] = new_fd;
    }

    pid_t pid = vfork();
    if (pid == -1) return __popen_fail(fds);

    if (pid == 0) {
        close(fds[parent]);
        // dup2 so that the child fd isn't closed on exec.
        if (dup2(fds[child], desired_child_fd) == -1) _exit(127);
        close(fds[child]);
        if (bidirectional) dup2(STDOUT_FILENO, STDIN_FILENO);
        // 自实现了传递命令的关键函数，其它的函数自实现有些困难
        execve("/system/bin/sh", (char* const[]){"sh", "-c", (char*)cmd, nullptr}, nullptr);
        _exit(127);
    }

    FILE* fp = fdopen(fds[parent], mode);
    if (fp == nullptr) return __popen_fail(fds);

    close(fds[child]);

    return fp;
}


#endif



