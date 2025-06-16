#ifndef DETECTFRIDA_MYLIBC_H
#define DETECTFRIDA_MYLIBC_H

#include <stdlib.h>

namespace nonstd {

    int mprotect(void *__addr, size_t __size, int __prot);

    void *malloc(size_t __byte_count);

//int  my_openat(int __dir_fd, const void* __path, int __flags, int __mode );

    int open(const void *__path, int __flags);

    int fstat(int __fd, struct stat *__buf);

    ssize_t read(int __fd, void *__buf, size_t __count);

    void *memcpy(void *dst0, const void *src0, size_t length);

    char *strrchr(const char *str, int character);

    char *strncpy(char *dst, const char *src, size_t n);

    size_t strlcpy(char *dst, const char *src, size_t siz);

    size_t strlen(const char *s);

    int strncmp(const char *s1, const char *s2, size_t n);

    char *strstr(const char *s, const char *find);

    void *memset(void *dst, int c, size_t n);

    int strcmp(const char *s1, const char *s2);

    int atoi(const char *s);

    char *strchr(const char *p, int ch);

    int close(int __fd);


#define __asm_syscall(...) do { \
    __asm__ __volatile__ ( "svc 0" \
    : "=r"(x0) : __VA_ARGS__ : "memory", "cc"); \
    return x0; \
    } while (0)

    __attribute__((always_inline))
    static inline long __syscall0(long n) {
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0");
        __asm_syscall("r"(x8));
    }

    __attribute__((always_inline))
    static inline long __syscall1(long n, long a) {
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a;
        __asm_syscall("r"(x8), "0"(x0));
    }

    __attribute__((always_inline))
    static inline long __syscall2(long n, long a, long b) {
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a;
        register long x1 __asm__("x1") = b;
        __asm_syscall("r"(x8), "0"(x0), "r"(x1));
    }

    __attribute__((always_inline))
    static inline long __syscall3(long n, long a, long b, long c) {
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a;
        register long x1 __asm__("x1") = b;
        register long x2 __asm__("x2") = c;
        __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2));
    }

    __attribute__((always_inline))
    static inline long __syscall4(long n, long a, long b, long c, long d) {
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a;
        register long x1 __asm__("x1") = b;
        register long x2 __asm__("x2") = c;
        register long x3 __asm__("x3") = d;
        __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3));
    }

    __attribute__((always_inline))
    static inline long __syscall5(long n, long a, long b, long c, long d, long e) {
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a;
        register long x1 __asm__("x1") = b;
        register long x2 __asm__("x2") = c;
        register long x3 __asm__("x3") = d;
        register long x4 __asm__("x4") = e;
        __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4));
    }

    __attribute__((always_inline))
    static inline long __syscall6(long n, long a, long b, long c, long d, long e, long f) {
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a;
        register long x1 __asm__("x1") = b;
        register long x2 __asm__("x2") = c;
        register long x3 __asm__("x3") = d;
        register long x4 __asm__("x4") = e;
        register long x5 __asm__("x5") = f;
        __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5));
    }
}

#endif //DETECTFRIDA_MYLIBC_H
