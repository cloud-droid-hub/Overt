#include <linux/fcntl.h>
#include <asm/unistd.h>
#include "libc.h"

namespace nonstd {
    char *strrchr(const char *str, int character) {
        const char *ptr = nullptr;
        while (*str != '\0') {
            if (*str == (char) character) {
                ptr = str;
            }
            str++;
        }
        return (char *) ptr;
    }
    char *strncpy(char *dst, const char *src, size_t n) {
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
        return (dst);
    }

    size_t strlcpy(char *dst, const char *src, size_t siz) {
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
        return (s - src - 1);    /* count does not include NUL */
    }

    size_t strlen(const char *s) {
        size_t len = 0;
        while (*s++) len++;
        return len;
    }

    int strncmp(const char *s1, const char *s2, size_t n) {
        if (n == 0)
            return (0);
        do {
            if (*s1 != *s2++)
                return (*(unsigned char *) s1 - *(unsigned char *) --s2);
            if (*s1++ == 0)
                break;
        } while (--n != 0);
        return (0);
    }

    char *strstr(const char *s, const char *find) {
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
        return ((char *) s);
    }

    void *memset(void *dst, int val, size_t count) {
        char *ptr = (char *) dst;
        while (count--)
            *ptr++ = val;
        return dst;
    }

    int strcmp(const char *s1, const char *s2) {
        while (*s1 == *s2++)
            if (*s1++ == 0)
                return (0);
        return (*(unsigned char *) s1 - *(unsigned char *) --s2);
    }

/** Returns true if `ch` is in `[ \f\n\r\t\v]`. */
    static __inline int isspace(int __ch) {
        return __ch == ' ' || (__ch >= '\t' && __ch <= '\r');
    }

/** Returns true if `ch` is in `[0-9]`. */
    static __inline int isdigit(int __ch) {
        return (__ch >= '0' && __ch <= '9');
    }

    int atoi(const char *s) {
        int n = 0, neg = 0;
        while (isspace(*s)) s++;
        switch (*s) {
            case '-':
                neg = 1;
            case '+':
                s++;
        }
        while (isdigit(*s))
            n = 10 * n - (*s++ - '0');
        return neg ? n : -n;
    }

    void *memcpy(void *dst, const void *src, size_t len) {
        const char *s = (const char *) src;
        char *d = (char *) dst;
        while (len--)
            *d++ = *s++;
        return dst;
    }

    char *__strchr_chk(const char *p, int ch, size_t s_len) {
        for (;; ++p, s_len--) {
            if (*p == static_cast<char>(ch)) {
                return const_cast<char *>(p);
            }
            if (*p == '\0') {
                return nullptr;
            }
        }
    }

    char *strchr(const char *p, int ch) {
        return __strchr_chk(p, ch, ((size_t) -1));
    }

    __attribute__((always_inline))
    static inline int open(int __dir_fd, const void *__path, int __flags, int __mode) {
        return (int) nonstd::__syscall4(__NR_openat, __dir_fd, (long) __path, __flags, __mode);
    }

    int open(const void *__path, int __flags) {
        return open(AT_FDCWD, __path, __flags, 0);
    }

    int fstat(int __fd, struct stat *__buf) {
        return nonstd::__syscall2(__NR_fstat, __fd, (long) __buf);
    }


    ssize_t read(int __fd, void *__buf, size_t __count) {
        return nonstd::__syscall3(__NR_read, __fd, (long) __buf, (long) __count);
    }

    __attribute__((always_inline))
    static inline off_t lseek(int __fd, off_t __offset, int __whence) {
        return nonstd::__syscall3(__NR_lseek, __fd, __offset, __whence);
    }

    __attribute__((always_inline))
    int close(int __fd) {
        return (int) nonstd::__syscall1(__NR_close, __fd);
    }

    __attribute__((always_inline))
    static inline int nanosleep(const struct timespec *__request, struct timespec *__remainder) {
        return (int) nonstd::__syscall2(__NR_nanosleep, (long) __request, (long) __remainder);
    }

    __attribute__((always_inline))
    static inline ssize_t readlinkat(int __dir_fd, const char *__path, char *__buf, size_t __buf_size) {
        return nonstd::__syscall4(__NR_readlinkat, __dir_fd, (long) __path, (long) __buf, (long) __buf_size);
    }


    __attribute__((always_inline))
    int inotify_init1(int flags) {
        return nonstd::__syscall1(__NR_inotify_init1, flags);
    }

    __attribute__((always_inline))
    int inotify_add_watch(int __fd, const char *__path, uint32_t __mask) {
        return nonstd::__syscall3(__NR_inotify_add_watch, __fd, (long) __path, (long) __mask);
    }

    __attribute__((always_inline))
    int inotify_rm_watch(int __fd, uint32_t __watch_descriptor) {
        return nonstd::__syscall2(__NR_inotify_rm_watch, __fd, (long) __watch_descriptor);
    }

//Not Used
    __attribute__((always_inline))
    static inline int tgkill(int __tgid, int __tid, int __signal) {
        return (int) nonstd::__syscall3(__NR_tgkill, __tgid, __tid, __signal);
    }

//Not Used
    __attribute__((always_inline))
    static inline void exit(int __status) {
        nonstd::__syscall1(__NR_exit, __status);
    }

//int mprotect(void* __addr, size_t __size, int __prot);
    int mprotect(void *__addr, size_t __size, int __prot) {
        return (int) nonstd::__syscall3(__NR_mprotect, (long) __addr, (long) __size, (long) __prot);
    }

//void* malloc(size_t __byte_count);
    void *malloc(size_t __byte_count) {
        void *current_break = (void *) nonstd::__syscall1(__NR_brk, (long) 0);     // 获取当前堆的结束地址
        void *new_break = (void *) ((char *) current_break + __byte_count); // 计算新的堆结束地址
        if ((int) nonstd::__syscall1(__NR_brk, (long) new_break) == -1) {         // 设置新的堆结束地址
            return NULL;                                                // 错误处理
        }
        return new_break;
    }
}