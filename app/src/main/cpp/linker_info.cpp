//
// Created by lxz on 2025/6/12.
//

#include "linker_info.h"
#include "zElfEditor.h"
#include "zLinker.h"
#include <setjmp.h>
#include <unwind.h>
#include <android/log.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

char* linker64_base_addr = nullptr;

struct BacktraceState {
    void **buffer;
    int max;
    int count;
};

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context *context, void *arg) {
    BacktraceState *state = (BacktraceState *)arg;
    if (state->count < state->max) {
        state->buffer[state->count++] = (void *)_Unwind_GetIP(context);
    }
    return _URC_NO_REASON;
}



// 通过堆栈获取 linker64 代码段在内存中的地址（非代码段起始地址）
// 此函数在 _init 函数中调用时，其堆栈顺序如下
// 其中 __dl__ZN6soinfo17call_constructorsEv 函数为 ld-android.so 也就是 linker64 中的一个地址
// 从这里向上查找到的第一个 elf 头就是 ld-android.so 也就是 linker64 的基地址
// get_linker64_code_addr                                                   [0]
// _init() native-lib.cpp:54                                                [1]
// __dl__ZN6soinfo17call_constructorsEv 0x0000006ffc172734                  [2]
// __dl__Z9do_dlopenPKciPK17android_dlextinfoPKv 0x0000006ffc15c324         [3]
// __dl___loader_android_dlopen_ext 0x0000006ffc157100                      [4]
// android_dlopen_ext 0x0000006fdbde1110                                    [5]
void* get_linker64_code_addr(){
    void *buffer[100];
    BacktraceState state = {buffer, 100, 0};
    _Unwind_Backtrace(unwindCallback, &state);

    for(int i = 0; i < state.count; i++){
        void* addr = state.buffer[i];
        Dl_info info;
        if (dladdr(addr, &info)) {
            // 计算函数内的偏移量
            uintptr_t offset = (uintptr_t)addr - (uintptr_t)info.dli_fbase;
            
            // 打印详细信息
            LOGE("Stack frame[%d]:", i);
            LOGE("  Address: %p", addr);
            LOGE("  Module: %s", info.dli_fname ? info.dli_fname : "??");
            LOGE("  FunName: %s", info.dli_sname ? info.dli_sname : "??");
            LOGE("  Offset: 0x%lx", offset);

        } else {
            LOGE("Stack frame[%d]: %p (No symbol info)", i, addr);
        }
    }   

    void* __dl__ZN6soinfo17call_constructorsEv = state.buffer[2];
    return __dl__ZN6soinfo17call_constructorsEv;
}

char* get_linker64_code_addr_(int max_depth = 20) {
    void **fp = nullptr;

    // 获取当前的 fp 寄存器（x29）
    asm volatile("mov %0, x29" : "=r"(fp));

    for (int i = 0; i < max_depth && fp; ++i) {
        void *ret = *(fp + 1);   // return address 是 fp[1]
        if (ret == nullptr) break;

        Dl_info info;
        if (dladdr(ret, &info)) {
            LOGE("#%02d: pc=%p -> %s (%s+0x%lx)", i, ret,
                 info.dli_fname ? info.dli_fname : "???",
                 info.dli_sname ? info.dli_sname : "???",
                 (uintptr_t)ret - (uintptr_t)info.dli_saddr);
        } else {
            LOGE("#%02d: pc=%p (no symbol)", i, ret);
        }

        void **next_fp = (void **)(*fp);  // fp[0] = previous fp
        if (next_fp <= fp) break;        // 防止死循环
        fp = next_fp;

        if (i == 2){
            return (char*)ret;
        }
    }
    return nullptr;
}

// 判断内存地址是否映射物理内存
int is_address_valid(void *addr) {
    unsigned char vec;
    if (mincore(addr, sysconf(_SC_PAGESIZE), &vec) == 0) {
        return 1;  // 地址有效
    }
    return 0;  // 地址未映射
}

//void print_so_from_addr(void* addr) {
//    Dl_info info;
//    if (dladdr(addr, &info)) {
//        LOGE("Address: %p", addr);
//        LOGE("Module : %s", info.dli_fname ? info.dli_fname : "??");
//        LOGE("Symbol : %s", info.dli_sname ? info.dli_sname : "??");
//        LOGE("Offset : 0x%lx", (uintptr_t)addr - (uintptr_t)info.dli_fbase);
//    } else {
//        LOGE("Failed to find symbol info for address %p", addr);
//    }
//}

static sigjmp_buf jump_buffer;

void signal_handler(int signo) {
    siglongjmp(jump_buffer, 1);
}

// 判断内存区域是否可读，使用 SIGSEGV 捕获非法访问
int is_readable(void *addr) {
    struct sigaction sa_old_segv, sa_old_bus, sa_new;

    sa_new.sa_handler = signal_handler;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = SA_NODEFER;

    // 注册 SIGSEGV 和 SIGBUS 处理
    sigaction(SIGSEGV, &sa_new, &sa_old_segv);
    sigaction(SIGBUS, &sa_new, &sa_old_bus);

    if (sigsetjmp(jump_buffer, 1) == 0) {
        volatile char value = *(char *)addr;  // 试探性访问
        (void)value;  // 防止编译器优化

        // 恢复原来的信号处理
        sigaction(SIGSEGV, &sa_old_segv, NULL);
        sigaction(SIGBUS, &sa_old_bus, NULL);
        return 1;  // 可读
    }

    // 发生 SIGSEGV 或 SIGBUS
    sigaction(SIGSEGV, &sa_old_segv, NULL);
    sigaction(SIGBUS, &sa_old_bus, NULL);
    return 0;  // 不可读
}

void* find_elf_base(void* func_addr) {
    uintptr_t addr = (uintptr_t)func_addr;

    // 向下对齐到 0x1000 作为起始搜索地址
    addr &= ~(0x1000 - 1); // 4KB 对齐

    for(;addr>0x1000 && addr < 0x8000000000;addr -= 0x1000){
        // 判断内存地址是否映射物理内存
        if (!is_address_valid((void *)addr)) {
            continue;
        }

         // 判断内存区域是否可读
         if(!is_readable((void*)addr)){
             continue;
         }

        // 判断是否为 ELF 头
        if(*(unsigned int*)addr != 0x464c457f){
            continue;
        }

        LOGE("find elf struct %p %s", addr, addr);

//        zElfEditor lib = zElfEditor((void*)addr);
//        LOGE("soname is %p %s", lib.so_name, lib.so_name);

        // 理论上来讲这个地址的 soname 应该为 ld-android.so，但 android9 及以下这个符号读不到
        return (void*)addr; //
    }
    LOGE("soname find error");
    return nullptr;  // 没有找到
}

// 通过堆栈获取 linker 的首地址
//void __attribute__((constructor)) init_linker(void){
extern "C" void _init(void){
    __android_log_print(6, "lxz", "hello init");

    void* linker64_code_addr = get_linker64_code_addr_();
    LOGE("linker64_code_addr %p", linker64_code_addr);

    linker64_base_addr = (char*)find_elf_base(linker64_code_addr);
    LOGE("linker64_base_addr %p %s", linker64_base_addr, linker64_base_addr);
}



char* get_linker_solist(char* linker64_base_addr){
    zElfEditor linker64_file = zElfEditor("/system/bin/linker64");
    long solist_get_head_offset = linker64_file.find_symbol("__dl__Z15solist_get_headv");
    LOGE("solist_get_head_offset %x", solist_get_head_offset);

    LOGE("linker64_base_addr %p", linker64_base_addr);
    LOGE("solist_get_head_addr %p", linker64_base_addr + solist_get_head_offset);

    typedef char*(SOLIST_GET_HEAD)();
    SOLIST_GET_HEAD* solist_get_head =  (SOLIST_GET_HEAD*)(linker64_base_addr + solist_get_head_offset);
    char* solist = solist_get_head();

    return solist;
}

std::map<std::string, std::string> get_linker_info(){
    std::map<std::string, std::string> info;
    std::vector<std::string> libpath_list = zLinker::getInstance()->get_libpath_list();
    for (int i = 0; i < libpath_list.size(); ++i) {
        if(strstr(libpath_list[i].c_str(), "lsposed")){
            info[libpath_list[i]] = "error";
        }
    }
    return info;
}

