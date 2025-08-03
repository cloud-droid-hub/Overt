
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <bits/glibc-syscalls.h>
#include <dirent.h>
#include <asm-generic/unistd.h>

#include "zLog.h"
#include "syscall.h"
#include <string.h>
#include "nonstd_libc.h"

#ifdef USE_NONSTD_API


#define PAGE_ALIGN(x) (((x) + 0xFFF) & ~0xFFF)

// 手动实现strcmp函数 - 自定义版本
int nonstd_strcmp(const char *str1, const char *str2) {
    LOGV("nonstd_strcmp called: str1='%s', str2='%s'", str1 ? str1 : "NULL", str2 ? str2 : "NULL");
    
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

size_t nonstd_strlen(const char *str) {
    LOGV("nonstd_strlen called: str='%s'", str ? str : "NULL");
    
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

char* nonstd_strcpy(char *dest, const char *src) {
    LOGV("nonstd_strcpy called: dest=%p, src='%s'", dest, src ? src : "NULL");
    
    if (!dest || !src) {
        LOGV("strcpy: NULL pointer");
        return dest;
    }
    
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    
    LOGV("strcpy: copied '%s'", dest);
    return dest;
}

char* nonstd_strcat(char *dest, const char *src) {
    LOGV("nonstd_strcat called: dest='%s', src='%s'", dest ? dest : "NULL", src ? src : "NULL");
    
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

int nonstd_strncmp(const char *str1, const char *str2, size_t n) {
    LOGV("nonstd_strncmp called: str1='%s', str2='%s', n=%zu", 
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

void* nonstd_malloc(size_t size) {
    LOGV("nonstd_malloc is called");
    if (size == 0) return NULL;

    size_t total_size = PAGE_ALIGN(sizeof(MemHeader) + size);

    void* p = (void*)syscall(__NR_mmap,
                             NULL,
                             total_size,
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS,
                             -1,
                             0);

    if (p == MAP_FAILED) {
        LOGV("nonstd_malloc: mmap failed, errno=%d", errno);
        return NULL;
    }

    MemHeader* header = (MemHeader*)p;
    header->size = total_size;

    LOGV("nonstd_malloc: allocated %zu bytes (user size=%zu) at %p", total_size, size, header->data);
    return header->data;
}

void nonstd_free(void* ptr) {
    if (!ptr) return;

    MemHeader* header = (MemHeader*)((char*)ptr - offsetof(MemHeader, data));
    if ((uintptr_t)header % 4096 != 0) {
        LOGV("nonstd_free: invalid pointer alignment %p", ptr);
        return;
    }

    syscall(__NR_munmap, (void*)header, header->size);
    LOGV("nonstd_free: unmapped %zu bytes at %p", header->size, ptr);
}

void* nonstd_calloc(size_t nmemb, size_t size) {
    LOGV("nonstd_calloc called: nmemb=%zu, size=%zu", nmemb, size);

    // 溢出检查
    if (nmemb == 0 || size == 0 || nmemb > SIZE_MAX / size) {
        LOGV("nonstd_calloc: invalid allocation size, returning NULL");
        return NULL;
    }

    size_t total_size = nmemb * size;
    void* ptr = nonstd_malloc(total_size);
    if (ptr) {
        // 可自定义清零，也可用如下低级写法替代 memset
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < total_size; ++i) {
            p[i] = 0;
        }
        LOGV("nonstd_calloc: allocated and zeroed %zu bytes at %p", total_size, ptr);
    }

    return ptr;
}

void* nonstd_realloc(void* ptr, size_t size) {
    LOGV("nonstd_realloc called: ptr=%p, size=%zu", ptr, size);

    if (!ptr) {
        return nonstd_malloc(size);  // 相当于 malloc
    }

    if (size == 0) {
        nonstd_free(ptr);            // 相当于 free
        return NULL;
    }

    // 获取旧块大小
    MemHeader* old_header = (MemHeader*)((char*)ptr - offsetof(MemHeader, data));
    size_t old_size = old_header->size - sizeof(MemHeader);  // 原用户可用大小

    void* new_ptr = nonstd_malloc(size);
    if (!new_ptr) return NULL;

    // 复制较小的那一部分
    size_t copy_size = (size < old_size) ? size : old_size;
    const uint8_t* src = (const uint8_t*)ptr;
    uint8_t* dst = (uint8_t*)new_ptr;
    for (size_t i = 0; i < copy_size; ++i) {
        dst[i] = src[i];
    }

    nonstd_free(ptr);
    LOGV("nonstd_realloc: copied %zu bytes to new block %p", copy_size, new_ptr);
    return new_ptr;
}



// ==================== 文件操作函数 ====================

int nonstd_open(const char* pathname, int flags, ...) {
    LOGE("nonstd_open called: pathname='%s', flags=0x%x", pathname ? pathname : "NULL", flags);
    
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
    int fd = (int)__syscall4(SYS_openat, AT_FDCWD, (long)pathname, flags, mode);
    LOGV("open: returned fd=%d", fd);
    return fd;
}

int nonstd_close(int fd) {
    LOGV("nonstd_close called: fd=%d", fd);
    
    int result = (int)__syscall1(SYS_close, fd);
    LOGV("close: result=%d", result);
    return result;
}

ssize_t nonstd_read(int fd, void *buf, size_t count) {
    LOGV("nonstd_read called: fd=%d, buf=%p, count=%zu", fd, buf, count);

    if (!buf) {
        LOGV("read: NULL buffer");
        return -1;
    }

    ssize_t result = __syscall3(SYS_read, fd, (long)buf, count);
    LOGV("read: result=%zd", result);
    return result;
}

ssize_t nonstd_write(int fd, const void *buf, size_t count) {
    LOGV("nonstd_write called: fd=%d, buf=%p, count=%zu", fd, buf, count);
    
    if (!buf) {
        LOGV("write: NULL buffer");
        return -1;
    }
    
    ssize_t result = __syscall3(SYS_write, fd, (long)buf, count);
    LOGV("write: result=%zd", result);
    return result;
}

// ==================== 网络函数 ====================
//
//int nonstd_socket(int domain, int type, int protocol) {
//    LOGV("nonstd_socket called: domain=%d, type=%d, protocol=%d", domain, type, protocol);
//
//    int sock = (int)__syscall3(SYS_socket, domain, type, protocol);
//    LOGV("socket: returned sock=%d", sock);
//    return sock;
//}
//
//int nonstd_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
//    LOGV("nonstd_connect called: sockfd=%d, addr=%p, addrlen=%u", sockfd, addr, addrlen);
//
//    int result = (int)__syscall3(SYS_connect, sockfd, (long)addr, addrlen);
//    LOGV("connect: result=%d", result);
//    return result;
//}

int nonstd_socket(int domain, int type, int protocol) {
    LOGV("nonstd_socket called: domain=%d, type=%d, protocol=%d", domain, type, protocol);

    // socket syscall 参数顺序：domain, type, protocol
    long sock = __syscall3(SYS_socket, (long)domain, (long)type, (long)protocol);

    if (sock < 0) {
        LOGV("socket: syscall failed, errno=%d", -sock);
        return -1;
    }

    LOGV("socket: created sockfd=%ld", sock);
    return (int)sock;
}

int nonstd_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    LOGV("nonstd_connect called: sockfd=%d, addr=%p, addrlen=%u", sockfd, addr, addrlen);

    // connect 参数顺序：sockfd, addr, addrlen
    long result = __syscall3(SYS_connect, (long)sockfd, (long)addr, (long)addrlen);

    if (result < 0) {
        LOGV("connect: syscall failed, errno=%ld", -result);
        return -1;
    }

    LOGV("connect: success");
    return 0;
}



int nonstd_fcntl(int fd, int cmd, ...) {
    LOGV("nonstd_fcntl called: fd=%d, cmd=%d", fd, cmd);

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

    long result = __syscall3(SYS_fcntl64, (long)fd, (long)cmd, arg);

    if (result < 0) {
        LOGV("fcntl: syscall failed, errno=%ld", -result);
        errno = -result;
        return -1;
    }

    LOGV("fcntl: success, result=%ld", result);
    return (int)result;
}

int nonstd_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    LOGV("nonstd_bind called: sockfd=%d, addr=%p, addrlen=%u", sockfd, addr, addrlen);
    
    int result = (int)__syscall3(SYS_bind, sockfd, (long)addr, addrlen);
    LOGV("bind: result=%d", result);
    return result;
}

int nonstd_listen(int sockfd, int backlog) {
    LOGV("nonstd_listen called: sockfd=%d, backlog=%d", sockfd, backlog);
    
    int result = (int)__syscall2(SYS_listen, sockfd, backlog);
    LOGV("listen: result=%d", result);
    return result;
}

int nonstd_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    LOGV("nonstd_accept called: sockfd=%d, addr=%p, addrlen=%p", sockfd, addr, addrlen);
    
    int result = (int)__syscall3(SYS_accept, sockfd, (long)addr, (long)addrlen);
    LOGV("accept: result=%d", result);
    return result;
}

// ==================== 时间函数 ====================

time_t nonstd_time(time_t *tloc) {
    LOGV("nonstd_time called: tloc=%p", tloc);
    
    // 在 ARM64 上，time 系统调用已被废弃，使用 clock_gettime 替代
    struct timespec ts;

    int result = __syscall2(SYS_clock_gettime, CLOCK_REALTIME, (long)&ts);

    
    if (result != 0) {
        return -1;
    }
    
    time_t time_result = (time_t)ts.tv_sec;
    if (tloc) {
        *tloc = time_result;
    }

    return time_result;
}

int nonstd_gettimeofday(struct timeval *tv, struct timezone *tz) {
    LOGV("nonstd_gettimeofday called: tv=%p, tz=%p", tv, tz);
    
    int result = (int)__syscall2(SYS_gettimeofday, (long)tv, (long)tz);
    LOGV("gettimeofday: result=%d", result);
    return result;
}

// ==================== 进程函数 ====================

pid_t nonstd_getpid(void) {
    LOGV("nonstd_getpid called");
    
    pid_t result = (pid_t)__syscall0(SYS_getpid);
    LOGV("getpid: result=%d", result);
    return result;
}

pid_t nonstd_getppid(void) {
    LOGV("nonstd_getppid called");
    
    // getppid系统调用号
    pid_t result = (pid_t)__syscall0(173); // SYS_getppid
    LOGV("getppid: result=%d", result);
    return result;
}

// ==================== 信号函数 ====================

int nonstd_kill(pid_t pid, int sig) {
    LOGV("nonstd_kill called: pid=%d, sig=%d", pid, sig);
    
    int result = (int)__syscall2(SYS_kill, pid, sig);
    LOGV("kill: result=%d", result);
    return result;
}

// ==================== 其他常用函数 ====================

int nonstd_atoi(const char *nptr) {
    LOGV("nonstd_atoi called: nptr='%s'", nptr ? nptr : "NULL");
    
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
    LOGV("atoi: result=%d", result);
    return result;
}

long nonstd_atol(const char *nptr) {
    LOGV("nonstd_atol called: nptr='%s'", nptr ? nptr : "NULL");
    
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

char* nonstd_strrchr(const char *str, int character) {
    LOGV("nonstd_strrchr called: str='%s', character='%c'", str ? str : "NULL", character);
    
    const char *ptr = nullptr;
    while (*str != '\0') {
        if (*str == (char) character) {
            ptr = str;
        }
        str++;
    }
    
    LOGV("strrchr: result=%p", (void*)ptr);
    return (char *) ptr;
}

char* nonstd_strncpy(char *dst, const char *src, size_t n) {
    LOGV("nonstd_strncpy called: dst=%p, src='%s', n=%zu", dst, src ? src : "NULL", n);
    
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

size_t nonstd_strlcpy(char *dst, const char *src, size_t siz) {
    LOGV("nonstd_strlcpy called: dst=%p, src='%s', siz=%zu", dst, src ? src : "NULL", siz);
    
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

char* nonstd_strstr(const char *s, const char *find) {
    LOGV("nonstd_strstr called: s='%s', find='%s'", s ? s : "NULL", find ? find : "NULL");
    
    char c, sc;
    size_t len;
    if ((c = *find++) != '\0') {
        len = nonstd_strlen(find);
        do {
            do {
                if ((sc = *s++) == '\0')
                    return (nullptr);
            } while (sc != c);
        } while (nonstd_strncmp(s, find, len) != 0);
        s--;
    }
    
    LOGV("strstr: result='%s'", s);
    return ((char *) s);
}

void* nonstd_memset(void *dst, int val, size_t count) {
    LOGV("nonstd_memset called: dst=%p, val=%d, count=%zu", dst, val, count);
    
    char *ptr = (char*)dst;
    while (count--)
        *ptr++ = val;
    
    LOGV("memset: completed");
    return dst;
}

void* nonstd_memcpy(void *dst, const void *src, size_t len) {
    LOGV("nonstd_memcpy called: dst=%p, src=%p, len=%zu", dst, src, len);
    
    const char* s = (const char*)src;
    char *d = (char*)dst;
    while (len--)
        *d++ = *s++;
    
    LOGV("memcpy: completed");
    return dst;
}

int nonstd_memcmp(const void *s1, const void *s2, size_t n) {
    LOGV("nonstd_memcmp called: s1=%p, s2=%p, n=%zu", s1, s2, n);
    
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

char* nonstd_strchr(const char *p, int ch) {
    LOGV("nonstd_strchr called: p='%s', ch='%c'", p ? p : "NULL", ch);
    
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

int nonstd_fstat(int __fd, struct stat* __buf) {
    LOGV("nonstd_fstat called: fd=%d, buf=%p", __fd, __buf);
    
    int result = __syscall2(SYS_fstat, __fd, (long)__buf);
    LOGV("fstat: result=%d", result);
    return result;
}

off_t nonstd_lseek(int __fd, off_t __offset, int __whence) {
    LOGV("nonstd_lseek called: fd=%d, offset=%ld, whence=%d", __fd, __offset, __whence);

    off_t result = __syscall3(SYS_lseek, __fd, __offset, __whence);
    LOGV("lseek: result=%ld", result);
    return result;
}

ssize_t nonstd_readlinkat(int __dir_fd, const char* __path, char* __buf, size_t __buf_size) {
    LOGV("nonstd_readlinkat called: dir_fd=%d, path='%s', buf=%p, buf_size=%zu",
         __dir_fd, __path ? __path : "NULL", __buf, __buf_size);

    ssize_t result = __syscall4(SYS_readlinkat, __dir_fd, (long)__path, (long)__buf, (long)__buf_size);
    LOGV("readlinkat: result=%zd", result);
    return result;
}

// ==================== 扩展系统函数 ====================

int nonstd_nanosleep(const struct timespec* __request, struct timespec* __remainder) {
    LOGV("nonstd_nanosleep called: request=%p, remainder=%p", __request, __remainder);

    int result = (int)__syscall2(SYS_nanosleep, (long)__request, (long)__remainder);
    LOGV("nanosleep: result=%d", result);
    return result;
}

int nonstd_mprotect(void* __addr, size_t __size, int __prot) {
    LOGV("nonstd_mprotect called: addr=%p, size=%zu, prot=%d", __addr, __size, __prot);
    
    int result = (int)__syscall3(SYS_mprotect, (long)__addr, (long)__size, (long)__prot);
    LOGV("mprotect: result=%d", result);
    return result;
}

int nonstd_inotify_init1(int flags) {
    LOGV("nonstd_inotify_init1 called: flags=%d", flags);
    
    int result = __syscall1(SYS_inotify_init1, flags);
    LOGV("inotify_init1: result=%d", result);
    return result;
}

int nonstd_inotify_add_watch(int __fd, const char *__path, uint32_t __mask) {
    LOGV("nonstd_inotify_add_watch called: fd=%d, path='%s', mask=%u", __fd, __path ? __path : "NULL", __mask);
    
    int result = __syscall3(SYS_inotify_add_watch, __fd, (long)__path, (long)__mask);
    LOGV("inotify_add_watch: result=%d", result);
    return result;
}

int nonstd_inotify_rm_watch(int __fd, uint32_t __watch_descriptor) {
    LOGV("nonstd_inotify_rm_watch called: fd=%d, watch_descriptor=%u", __fd, __watch_descriptor);
    
    int result = __syscall2(SYS_inotify_rm_watch, __fd, (long)__watch_descriptor);
    LOGV("inotify_rm_watch: result=%d", result);
    return result;
}

int nonstd_tgkill(int __tgid, int __tid, int __signal) {
    LOGV("nonstd_tgkill called: tgid=%d, tid=%d, signal=%d", __tgid, __tid, __signal);
    
    int result = (int)__syscall3(SYS_tgkill, __tgid, __tid, __signal);
    LOGV("tgkill: result=%d", result);
    return result;
}

void nonstd_exit(int __status) {
    LOGV("nonstd_exit called: status=%d", __status);
    
    __syscall1(SYS_exit, __status);
    // 这行代码永远不会执行
}



// ==================== 目录操作函数 ====================



// ==================== 时间相关 ====================





ssize_t nonstd_readlink(const char *pathname, char *buf, size_t bufsiz) {
    LOGV("nonstd_readlink called: pathname='%s', buf=%p, bufsiz=%zu", pathname ? pathname : "NULL", buf, bufsiz);
    if (!pathname || !buf) {
        LOGV("readlink: NULL arg");
        return -1;
    }
    ssize_t result = (ssize_t)__syscall3(SYS_readlink, (long)pathname, (long)buf, (long)bufsiz);
    LOGV("readlink: result=%zd", result);
    return result;
}

struct tm* nonstd_localtime(const time_t* timep) {
    LOGV("nonstd_localtime called: timep=%p", timep);
    if (!timep) return nullptr;
    static struct tm result;
    time_t t = *timep;
    t += 8 * 3600;
    gmtime_r(&t, &result);
    return &result;
}

int nonstd_stat(const char* __path, struct stat* __buf) {
    LOGV("nonstd_stat called: path='%s', buf=%p", __path ? __path : "NULL", __buf);
    if (!__path || !__buf) {
        LOGV("stat: NULL arg");
        return -1;
    }
    int result = (int)__syscall4(SYS_newfstatat, AT_FDCWD, (long)__path, (long)__buf, 0);
    LOGV("stat: result=%d", result);
    return result;
}


int nonstd_access(const char* __path, int __mode) {
    LOGV("nonstd_access called: path='%s', mode=0x%x", __path ? __path : "NULL", __mode);

    if (!__path) {
        LOGV("access: NULL path");
        errno = EFAULT;  // 错误地址
        return -1;
    }

    long result = __syscall2(SYS_access, (uintptr_t)__path, (long)__mode);

    if (result < 0) {
        errno = -result;
        LOGV("access: failed, errno=%d", errno);
        return -1;
    }

    LOGV("access: success");
    return 0;
}

#endif // USE_NONSTD_API

