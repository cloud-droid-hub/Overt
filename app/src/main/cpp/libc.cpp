#include <string.h>
#include <android/log.h>
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

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "lxz", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "lxz", __VA_ARGS__)

#include "config.h"
#include "syscall.h"

#ifdef USE_NONSTD_API

// 手动实现strcmp函数 - 自定义版本
extern "C" int nonstd_strcmp(const char *str1, const char *str2) {
    LOGE("nonstd_strcmp called: str1='%s', str2='%s'", str1 ? str1 : "NULL", str2 ? str2 : "NULL");
    
    // 处理NULL指针
    if (!str1 && !str2) {
        LOGE("Both strings are NULL, returning 0");
        return 0;
    }
    if (!str1) {
        LOGE("str1 is NULL, returning -1");
        return -1;
    }
    if (!str2) {
        LOGE("str2 is NULL, returning 1");
        return 1;
    }
    
    // 手动实现strcmp逻辑
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            int result = (unsigned char)str1[i] - (unsigned char)str2[i];
            LOGE("Strings differ at position %d: '%c' vs '%c', returning %d", i, str1[i], str2[i], result);
            return result;
        }
        i++;
    }
    
    // 检查字符串长度
    if (str1[i] == '\0' && str2[i] == '\0') {
        LOGE("Strings are identical, returning 0");
        return 0;
    } else if (str1[i] == '\0') {
        LOGE("str1 is shorter, returning -1");
        return -1;
    } else {
        LOGE("str2 is shorter, returning 1");
        return 1;
    }
}

// ==================== 字符串函数 ====================

extern "C" size_t nonstd_strlen(const char *str) {
    LOGE("nonstd_strlen called: str='%s'", str ? str : "NULL");
    
    if (!str) {
        LOGE("strlen: NULL pointer, returning 0");
        return 0;
    }
    
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    
    LOGE("strlen: length = %zu", len);
    return len;
}

extern "C" char* nonstd_strcpy(char *dest, const char *src) {
    LOGE("nonstd_strcpy called: dest=%p, src='%s'", dest, src ? src : "NULL");
    
    if (!dest || !src) {
        LOGE("strcpy: NULL pointer");
        return dest;
    }
    
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    
    LOGE("strcpy: copied '%s'", dest);
    return dest;
}

extern "C" char* nonstd_strcat(char *dest, const char *src) {
    LOGE("nonstd_strcat called: dest='%s', src='%s'", dest ? dest : "NULL", src ? src : "NULL");
    
    if (!dest || !src) {
        LOGE("strcat: NULL pointer");
        return dest;
    }
    
    char *d = dest;
    while (*d != '\0') d++;
    while ((*d++ = *src++) != '\0');
    
    LOGE("strcat: result = '%s'", dest);
    return dest;
}

extern "C" int nonstd_strncmp(const char *str1, const char *str2, size_t n) {
    LOGE("nonstd_strncmp called: str1='%s', str2='%s', n=%zu", 
         str1 ? str1 : "NULL", str2 ? str2 : "NULL", n);
    
    if (n == 0) {
        LOGE("strncmp: n=0, returning 0");
        return 0;
    }
    
    do {
        if (*str1 != *str2++) {
            int result = (*(unsigned char *)str1 - *(unsigned char *)(--str2));
            LOGE("strncmp: diff, returning %d", result);
            return result;
        }
        if (*str1++ == 0) break;
    } while (--n != 0);
    
    LOGE("strncmp: equal, returning 0");
    return 0;
}

// ==================== 内存函数 ====================

extern "C" void* nonstd_malloc(size_t size) {
    LOGE("nonstd_malloc called: size=%zu", size);
    
    if (size == 0) {
        LOGE("malloc: size=0, returning NULL");
        return NULL;
    }
    
    // 使用系统调用分配内存
    void *ptr = (void*)__syscall6(SYS_mmap, 0, size, 
                                 PROT_READ | PROT_WRITE, 
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (ptr == MAP_FAILED) {
        LOGE("malloc: mmap failed, returning NULL");
        return NULL;
    }
    
    LOGE("malloc: allocated %zu bytes at %p", size, ptr);
    return ptr;
}

extern "C" void nonstd_free(void *ptr) {
    LOGE("nonstd_free called: ptr=%p", ptr);
    
    if (!ptr) {
        LOGE("free: NULL pointer, ignoring");
        return;
    }
    
    // 使用系统调用释放内存
    long result = __syscall2(SYS_munmap, (long)ptr, 0);
    if (result == 0) {
        LOGE("free: successfully freed %p", ptr);
    } else {
        LOGE("free: munmap failed for %p", ptr);
    }
}

extern "C" void* nonstd_calloc(size_t nmemb, size_t size) {
    LOGE("nonstd_calloc called: nmemb=%zu, size=%zu", nmemb, size);
    
    size_t total_size = nmemb * size;
    if (total_size == 0 || total_size / nmemb != size) { // 检查溢出
        LOGE("calloc: invalid size, returning NULL");
        return NULL;
    }
    
    void *ptr = nonstd_malloc(total_size);
    if (ptr) {
        // 清零内存
        memset(ptr, 0, total_size);
        LOGE("calloc: allocated and zeroed %zu bytes at %p", total_size, ptr);
    }
    
    return ptr;
}

extern "C" void* nonstd_realloc(void *ptr, size_t size) {
    LOGE("nonstd_realloc called: ptr=%p, size=%zu", ptr, size);
    
    if (!ptr) {
        LOGE("realloc: NULL pointer, calling malloc");
        return nonstd_malloc(size);
    }
    
    if (size == 0) {
        LOGE("realloc: size=0, calling free and returning NULL");
        nonstd_free(ptr);
        return NULL;
    }
    
    // 简单实现：分配新内存，复制数据，释放旧内存
    void *new_ptr = nonstd_malloc(size);
    if (new_ptr) {
        // 这里应该获取原内存块的大小，简化处理
        nonstd_memcpy(new_ptr, ptr, size);
        nonstd_free(ptr);
        LOGE("realloc: reallocated to %zu bytes at %p", size, new_ptr);
    }
    
    return new_ptr;
}

// ==================== 文件操作函数 ====================

extern "C" int nonstd_open(const char *pathname, int flags, ...) {
    LOGE("nonstd_open called: pathname='%s', flags=0x%x", pathname ? pathname : "NULL", flags);
    
    if (!pathname) {
        LOGE("open: NULL pathname");
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
    LOGE("open: returned fd=%d", fd);
    return fd;
}

extern "C" int nonstd_close(int fd) {
    LOGE("nonstd_close called: fd=%d", fd);
    
    int result = (int)__syscall1(SYS_close, fd);
    LOGE("close: result=%d", result);
    return result;
}

extern "C" ssize_t nonstd_read(int fd, void *buf, size_t count) {
    LOGE("nonstd_read called: fd=%d, buf=%p, count=%zu", fd, buf, count);
    
    if (!buf) {
        LOGE("read: NULL buffer");
        return -1;
    }
    
    ssize_t result = __syscall3(SYS_read, fd, (long)buf, count);
    LOGE("read: result=%zd", result);
    return result;
}

extern "C" ssize_t nonstd_write(int fd, const void *buf, size_t count) {
    LOGE("nonstd_write called: fd=%d, buf=%p, count=%zu", fd, buf, count);
    
    if (!buf) {
        LOGE("write: NULL buffer");
        return -1;
    }
    
    ssize_t result = __syscall3(SYS_write, fd, (long)buf, count);
    LOGE("write: result=%zd", result);
    return result;
}

// ==================== 网络函数 ====================

extern "C" int nonstd_socket(int domain, int type, int protocol) {
    LOGE("nonstd_socket called: domain=%d, type=%d, protocol=%d", domain, type, protocol);
    
    int sock = (int)__syscall3(SYS_socket, domain, type, protocol);
    LOGE("socket: returned sock=%d", sock);
    return sock;
}

extern "C" int nonstd_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    LOGE("nonstd_connect called: sockfd=%d, addr=%p, addrlen=%u", sockfd, addr, addrlen);
    
    int result = (int)__syscall3(SYS_connect, sockfd, (long)addr, addrlen);
    LOGE("connect: result=%d", result);
    return result;
}

extern "C" int nonstd_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    LOGE("nonstd_bind called: sockfd=%d, addr=%p, addrlen=%u", sockfd, addr, addrlen);
    
    int result = (int)__syscall3(SYS_bind, sockfd, (long)addr, addrlen);
    LOGE("bind: result=%d", result);
    return result;
}

extern "C" int nonstd_listen(int sockfd, int backlog) {
    LOGE("nonstd_listen called: sockfd=%d, backlog=%d", sockfd, backlog);
    
    int result = (int)__syscall2(SYS_listen, sockfd, backlog);
    LOGE("listen: result=%d", result);
    return result;
}

extern "C" int nonstd_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    LOGE("nonstd_accept called: sockfd=%d, addr=%p, addrlen=%p", sockfd, addr, addrlen);
    
    int result = (int)__syscall3(SYS_accept, sockfd, (long)addr, (long)addrlen);
    LOGE("accept: result=%d", result);
    return result;
}

// ==================== 时间函数 ====================

extern "C" time_t nonstd_time(time_t *tloc) {
    LOGE("nonstd_time called: tloc=%p", tloc);
    
    time_t result = __syscall1(SYS_time, (long)tloc);
    LOGE("time: result=%ld", result);
    return result;
}

extern "C" int nonstd_gettimeofday(struct timeval *tv, struct timezone *tz) {
    LOGE("nonstd_gettimeofday called: tv=%p, tz=%p", tv, tz);
    
    int result = (int)__syscall2(SYS_gettimeofday, (long)tv, (long)tz);
    LOGE("gettimeofday: result=%d", result);
    return result;
}

// ==================== 进程函数 ====================

extern "C" pid_t nonstd_getpid(void) {
    LOGE("nonstd_getpid called");
    
    pid_t result = (pid_t)__syscall0(SYS_getpid);
    LOGE("getpid: result=%d", result);
    return result;
}

extern "C" pid_t nonstd_getppid(void) {
    LOGE("nonstd_getppid called");
    
    // getppid系统调用号
    pid_t result = (pid_t)__syscall0(173); // SYS_getppid
    LOGE("getppid: result=%d", result);
    return result;
}

// ==================== 信号函数 ====================

extern "C" int nonstd_kill(pid_t pid, int sig) {
    LOGE("nonstd_kill called: pid=%d, sig=%d", pid, sig);
    
    int result = (int)__syscall2(SYS_kill, pid, sig);
    LOGE("kill: result=%d", result);
    return result;
}

// ==================== 其他常用函数 ====================

extern "C" int nonstd_atoi(const char *nptr) {
    LOGE("nonstd_atoi called: nptr='%s'", nptr ? nptr : "NULL");
    
    if (!nptr) {
        LOGE("atoi: NULL pointer, returning 0");
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
    LOGE("atoi: result=%d", result);
    return result;
}

extern "C" long nonstd_atol(const char *nptr) {
    LOGE("nonstd_atol called: nptr='%s'", nptr ? nptr : "NULL");
    
    if (!nptr) {
        LOGE("atol: NULL pointer, returning 0");
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
    LOGE("atol: result=%ld", result);
    return result;
}

// ==================== 扩展字符串函数 ====================

extern "C" const char* nonstd_strrchr(const char *str, int character) {
    LOGE("nonstd_strrchr called: str='%s', character='%c'", str ? str : "NULL", character);
    
    const char *ptr = nullptr;
    while (*str != '\0') {
        if (*str == (char) character) {
            ptr = str;
        }
        str++;
    }
    
    LOGE("strrchr: result=%p", (void*)ptr);
    return (char *) ptr;
}

extern "C" char* nonstd_strncpy(char *dst, const char *src, size_t n) {
    LOGE("nonstd_strncpy called: dst=%p, src='%s', n=%zu", dst, src ? src : "NULL", n);
    
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
    
    LOGE("strncpy: result='%s'", dst);
    return (dst);
}

extern "C" size_t nonstd_strlcpy(char *dst, const char *src, size_t siz) {
    LOGE("nonstd_strlcpy called: dst=%p, src='%s', siz=%zu", dst, src ? src : "NULL", siz);
    
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
    LOGE("strlcpy: result=%zu", result);
    return result;
}

extern "C" const char* nonstd_strstr(const char *s, const char *find) {
    LOGE("nonstd_strstr called: s='%s', find='%s'", s ? s : "NULL", find ? find : "NULL");
    
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
    
    LOGE("strstr: result='%s'", s);
    return ((char *) s);
}

extern "C" void* nonstd_memset(void *dst, int val, size_t count) {
    LOGE("nonstd_memset called: dst=%p, val=%d, count=%zu", dst, val, count);
    
    char *ptr = (char*)dst;
    while (count--)
        *ptr++ = val;
    
    LOGE("memset: completed");
    return dst;
}

extern "C" void* nonstd_memcpy(void *dst, const void *src, size_t len) {
    LOGE("nonstd_memcpy called: dst=%p, src=%p, len=%zu", dst, src, len);
    
    const char* s = (const char*)src;
    char *d = (char*)dst;
    while (len--)
        *d++ = *s++;
    
    LOGE("memcpy: completed");
    return dst;
}

extern "C" const char* nonstd_strchr(const char *p, int ch) {
    LOGE("nonstd_strchr called: p='%s', ch='%c'", p ? p : "NULL", ch);
    
    for (;; ++p) {
        if (*p == static_cast<char>(ch)) {
            LOGE("strchr: found at %p", (void*)p);
            return const_cast<char *>(p);
        }
        if (*p == '\0') {
            LOGE("strchr: not found");
            return nullptr;
        }
    }
}

// ==================== 扩展文件操作函数 ====================

extern "C" int nonstd_fstat(int __fd, struct stat* __buf) {
    LOGE("nonstd_fstat called: fd=%d, buf=%p", __fd, __buf);
    
    int result = __syscall2(SYS_fstat, __fd, (long)__buf);
    LOGE("fstat: result=%d", result);
    return result;
}

extern "C" off_t nonstd_lseek(int __fd, off_t __offset, int __whence) {
    LOGE("nonstd_lseek called: fd=%d, offset=%ld, whence=%d", __fd, __offset, __whence);
    
    off_t result = __syscall3(SYS_lseek, __fd, __offset, __whence);
    LOGE("lseek: result=%ld", result);
    return result;
}

extern "C" ssize_t nonstd_readlinkat(int __dir_fd, const char* __path, char* __buf, size_t __buf_size) {
    LOGE("nonstd_readlinkat called: dir_fd=%d, path='%s', buf=%p, buf_size=%zu", 
         __dir_fd, __path ? __path : "NULL", __buf, __buf_size);
    
    ssize_t result = __syscall4(SYS_readlinkat, __dir_fd, (long)__path, (long)__buf, (long)__buf_size);
    LOGE("readlinkat: result=%zd", result);
    return result;
}

// ==================== 扩展系统函数 ====================

extern "C" int nonstd_nanosleep(const struct timespec* __request, struct timespec* __remainder) {
    LOGE("nonstd_nanosleep called: request=%p, remainder=%p", __request, __remainder);
    
    int result = (int)__syscall2(SYS_nanosleep, (long)__request, (long)__remainder);
    LOGE("nanosleep: result=%d", result);
    return result;
}

extern "C" int nonstd_mprotect(void* __addr, size_t __size, int __prot) {
    LOGE("nonstd_mprotect called: addr=%p, size=%zu, prot=%d", __addr, __size, __prot);
    
    int result = (int)__syscall3(SYS_mprotect, (long)__addr, (long)__size, (long)__prot);
    LOGE("mprotect: result=%d", result);
    return result;
}

extern "C" int nonstd_inotify_init1(int flags) {
    LOGE("nonstd_inotify_init1 called: flags=%d", flags);
    
    int result = __syscall1(SYS_inotify_init1, flags);
    LOGE("inotify_init1: result=%d", result);
    return result;
}

extern "C" int nonstd_inotify_add_watch(int __fd, const char *__path, uint32_t __mask) {
    LOGE("nonstd_inotify_add_watch called: fd=%d, path='%s', mask=%u", __fd, __path ? __path : "NULL", __mask);
    
    int result = __syscall3(SYS_inotify_add_watch, __fd, (long)__path, (long)__mask);
    LOGE("inotify_add_watch: result=%d", result);
    return result;
}

extern "C" int nonstd_inotify_rm_watch(int __fd, uint32_t __watch_descriptor) {
    LOGE("nonstd_inotify_rm_watch called: fd=%d, watch_descriptor=%u", __fd, __watch_descriptor);
    
    int result = __syscall2(SYS_inotify_rm_watch, __fd, (long)__watch_descriptor);
    LOGE("inotify_rm_watch: result=%d", result);
    return result;
}

extern "C" int nonstd_tgkill(int __tgid, int __tid, int __signal) {
    LOGE("nonstd_tgkill called: tgid=%d, tid=%d, signal=%d", __tgid, __tid, __signal);
    
    int result = (int)__syscall3(SYS_tgkill, __tgid, __tid, __signal);
    LOGE("tgkill: result=%d", result);
    return result;
}

extern "C" void nonstd_exit(int __status) {
    LOGE("nonstd_exit called: status=%d", __status);
    
    __syscall1(SYS_exit, __status);
    // 这行代码永远不会执行
}

#endif // USE_NONSTD_API 