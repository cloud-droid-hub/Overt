#ifndef SYSCALL_H
#define SYSCALL_H

#include <stddef.h>
#include <stdint.h>

// ARM64 系统调用宏
#define __asm_syscall(...) \
    __asm__ __volatile__("svc #0" \
        : "=r"(x0) : __VA_ARGS__ : "memory", "cc")

// 系统调用号定义 (Linux ARM64) - 根据 Chromium 官方文档
#define SYS_read        63
#define SYS_write       64
#define SYS_openat      56
#define SYS_close       57
#define SYS_mmap        222
#define SYS_munmap      215
#define SYS_brk         214
#define SYS_exit        93
#define SYS_getpid      172
#define SYS_getuid      174
#define SYS_getgid      176
#define SYS_fork        220
#define SYS_execve      221
#define SYS_wait4       260
#define SYS_kill        129
#define SYS_signal      134
#define SYS_socket      198
#define SYS_connect     203
#define SYS_bind        200
#define SYS_listen      201
#define SYS_accept      202
#define SYS_sendto      206
#define SYS_recvfrom    207
#define SYS_setsockopt  208
#define SYS_getsockopt  209
#define SYS_shutdown    210
#define SYS_poll        7
#define SYS_select      23
#define SYS_epoll_create1 20
#define SYS_epoll_ctl   21
#define SYS_epoll_wait  22
#define SYS_clock_gettime 113
#define SYS_gettimeofday 169
#define SYS_time        201
#define SYS_nanosleep   101
#define SYS_usleep      137
#define SYS_sleep       142
#define SYS_alarm       37
#define SYS_setitimer   103
#define SYS_getitimer   102
#define SYS_sigaction   134
#define SYS_sigprocmask 135
#define SYS_sigpending  136
#define SYS_sigsuspend  133
#define SYS_sigaltstack 132
#define SYS_rt_sigaction 134
#define SYS_rt_sigprocmask 135
#define SYS_rt_sigpending 136
#define SYS_rt_sigsuspend 133
#define SYS_rt_sigreturn 139
#define SYS_rt_sigqueueinfo 138
#define SYS_rt_sigtimedwait 137

// 扩展系统调用号
#define SYS_fstat        80
#define SYS_lseek        62
#define SYS_readlinkat   78
#define SYS_mprotect     226
#define SYS_inotify_init1 26
#define SYS_inotify_add_watch 27
#define SYS_inotify_rm_watch 28
#define SYS_tgkill       131

// 系统调用内联函数
__attribute__((always_inline))
static inline long __syscall0(long n) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0");
    __asm_syscall("r"(x8));
    return x0;
}

__attribute__((always_inline))
static inline long __syscall1(long n, long a) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    __asm_syscall("r"(x8), "0"(x0));
    return x0;
}

__attribute__((always_inline))
static inline long __syscall2(long n, long a, long b) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    __asm_syscall("r"(x8), "0"(x0), "r"(x1));
    return x0;
}

__attribute__((always_inline))
static inline long __syscall3(long n, long a, long b, long c) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2));
    return x0;
}

__attribute__((always_inline))
static inline long __syscall4(long n, long a, long b, long c, long d) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3));
    return x0;
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
    return x0;
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
    return x0;
}

#endif // SYSCALL_H 