#ifndef SYSCALL_H
#define SYSCALL_H

#include "zConfig.h"

// 模块配置开关 - 可以通过修改这个宏来控制是否使用自定义系统调用实现
#define ZSYSCALL_ENABLE_NONSTD_API 1

// 当全局配置宏启用时，全局宏配置覆盖模块宏配置
#if ZCONFIG_ENABLE
#undef ZSYSCALL_ENABLE_NONSTD_API
#define ZSYSCALL_ENABLE_NONSTD_API ZCONFIG_ENABLE_NONSTD_API
#endif


#if ZSYSCALL_ENABLE_NONSTD_API

#define nonstd_syscall syscall

#include <bits/glibc-syscalls.h>
#include <linux/fcntl.h>

// 在定义 syscall 宏之前，先包含 unistd.h 并取消可能存在的宏定义
#ifdef syscall
#undef syscall
#endif
#include <unistd.h>

// ARM64 系统调用宏
#define __asm_syscall(...) __asm__ __volatile__("svc #0" : "=r"(x0) : __VA_ARGS__ : "memory", "cc")

#define SYS_fstat           80
#define SYS_access          21
#define SYS_readlink        89
#define SYS_newfstatat      79
#define SYS_fcntl64         221
#define SYS_gettid          178
#define SYS_faccessat       48
#define SYS_fchownat        54

// 系统调用内联函数
__attribute__((always_inline))
static inline long nonstd_syscall_1(long n) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0");
    __asm_syscall("r"(x8));
    return x0;
}

__attribute__((always_inline))
static inline long nonstd_syscall_2(long n, long a) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    __asm_syscall("r"(x8), "0"(x0));
    return x0;
}

__attribute__((always_inline))
static inline long nonstd_syscall_3(long n, long a, long b) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    __asm_syscall("r"(x8), "0"(x0), "r"(x1));
    return x0;
}

__attribute__((always_inline))
static inline long nonstd_syscall_4(long n, long a, long b, long c) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2));
    return x0;
}

__attribute__((always_inline))
static inline long nonstd_syscall_5(long n, long a, long b, long c, long d) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3));
    return x0;
}

__attribute__((always_inline))
static inline long nonstd_syscall_6(long n, long a, long b, long c, long d, long e) {
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
static long nonstd_syscall_7(long n, long a, long b, long c, long d, long e, long f) {
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

// 辅助宏：将参数转换为 long 类型
#define __to_int(x) ((int)(x))
#define __to_long(x) ((long)(x))

// 辅助宏：将不同数量的参数转换为 long 类型
#define __nonstd_syscall_convert_1(a1) __to_long(a1)
#define __nonstd_syscall_convert_2(a1, a2) __to_long(a1), __to_long(a2)
#define __nonstd_syscall_convert_3(a1, a2, a3) __to_long(a1), __to_long(a2), __to_long(a3)
#define __nonstd_syscall_convert_4(a1, a2, a3, a4) __to_long(a1), __to_long(a2), __to_long(a3), __to_long(a4)
#define __nonstd_syscall_convert_5(a1, a2, a3, a4, a5) __to_long(a1), __to_long(a2), __to_long(a3), __to_long(a4), __to_long(a5)
#define __nonstd_syscall_convert_6(a1, a2, a3, a4, a5, a6) __to_long(a1), __to_long(a2), __to_long(a3), __to_long(a4), __to_long(a5), __to_long(a6)
#define __nonstd_syscall_convert_7(a1, a2, a3, a4, a5, a6, a7) __to_long(a1), __to_long(a2), __to_long(a3), __to_long(a4), __to_long(a5), __to_long(a6), __to_long(a7)

// 辅助宏：检测参数数量
// 使用宏技巧来检测 __VA_ARGS__ 中的参数数量
#define __nonstd_syscall_arg_count(...) __nonstd_syscall_arg_count_impl(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0)
#define __nonstd_syscall_arg_count_impl(_1, _2, _3, _4, _5, _6, _7, N, ...) N

// 根据参数数量选择对应的系统调用
#define __nonstd_syscall_dispatch(count, ...) __nonstd_syscall_dispatch_impl(count, __VA_ARGS__)
#define __nonstd_syscall_dispatch_impl(count, ...) ___nonstd_syscall_dispatch_impl_##count(__VA_ARGS__)

// 为每个参数数量定义转换和调用宏
#define ___nonstd_syscall_dispatch_impl_1(...) nonstd_syscall_1(__nonstd_syscall_convert_1(__VA_ARGS__))
#define ___nonstd_syscall_dispatch_impl_2(...) nonstd_syscall_2(__nonstd_syscall_convert_2(__VA_ARGS__))
#define ___nonstd_syscall_dispatch_impl_3(...) nonstd_syscall_3(__nonstd_syscall_convert_3(__VA_ARGS__))
#define ___nonstd_syscall_dispatch_impl_4(...) nonstd_syscall_4(__nonstd_syscall_convert_4(__VA_ARGS__))
#define ___nonstd_syscall_dispatch_impl_5(...) nonstd_syscall_5(__nonstd_syscall_convert_5(__VA_ARGS__))
#define ___nonstd_syscall_dispatch_impl_6(...) nonstd_syscall_6(__nonstd_syscall_convert_6(__VA_ARGS__))
#define ___nonstd_syscall_dispatch_impl_7(...) nonstd_syscall_7(__nonstd_syscall_convert_7(__VA_ARGS__))

// 主宏：根据参数数量自动选择对应的系统调用
// 注意：参数数量包括系统调用号本身，所以 1 个参数 = syscall, 2 个参数 = syscall, 等等
#define syscall(...) __nonstd_syscall_dispatch(__nonstd_syscall_arg_count(__VA_ARGS__), __VA_ARGS__)




#else
    // 使用标准系统调用实现
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <bits/glibc-syscalls.h>
    
    // 标准 syscall() 是可变参数函数，直接使用即可
    // 定义 nonstd_syscall 宏直接映射到标准 syscall()
    #define nonstd_syscall(...) syscall(__VA_ARGS__)
#endif

#endif // SYSCALL_H


















