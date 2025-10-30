//
// Created by liuxi on 2025/10/29.
//

#include "zSideChannelInfo.h"
#include "syscall.h"
#include "zLog.h"
#include "zStdUtil.h"

/**
 * 获取当前时间（纳秒）
 * 使用 cntvct_el0 寄存器获取系统启动后的时间
 * @return 当前时间戳（纳秒）
 */
static inline uint64_t raw_ns(void)
{
    uint64_t ns;
    asm volatile(
            "isb sy\n\t"            // 指令同步屏障（前后都不会被重排）
            "mrs %0, cntvct_el0\n\t"// 读虚拟计数器 → 纳秒
            "isb sy\n\t"            // 再次屏障，确保准确性
            : "=r"(ns));

    return ns;
}


/**
 * 获取侧信道信息的主函数
 * 通过侧信道攻击检测系统环境异常
 * 通过比较不同系统调用的执行时间来判断是否存在调试工具或Hook框架
 * @return 包含检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_side_channel_info(){
    map<string, map<string, string>> info;
    uint64_t times1[10000] = {0};  // 存储faccessat系统调用的执行时间
    uint64_t times2[10000] = {0};  // 存储fchownat系统调用的执行时间

    // 执行10000次faccessat系统调用，记录每次的执行时间
    for(int i = 0; i < 10000; i++){
        uint64_t start_time = raw_ns();
        __syscall4(SYS_faccessat, 0xFFFFFFFFLL, 0LL, 0xFFFFFFFFLL, 0LL);
        uint64_t end_time = raw_ns();
        times1[i] = end_time - start_time;
    }

    // 执行10000次fchownat系统调用，记录每次的执行时间
    for(int i = 0; i < 10000; i++){
        uint64_t start_time = raw_ns();
        __syscall5(SYS_fchownat, -1, 0LL, 0, 0, -1);
        uint64_t end_time = raw_ns();
        times2[i] = end_time - start_time;
    }

    // 一般来说 faccessat 的执行速度是要比 fchownat 快的，如果 faccessat 出现大量慢于 fchownat 的情况，那么说明环境有异常
    int error_count = 0;
    for(int i = 0; i < 10000; i++){
        if(times1[i] > times2[i]){
            error_count++;
        }
    }

    LOGE("error_count: %d", error_count);

    string explain = string_format("faccessat is slower than fchownat: %d", error_count);

    // 如果异常次数超过7000次，则认为环境存在异常
    if(error_count > 7000){
        info["side_channel"]["risk"] = "error";
        info["side_channel"]["explain"] = explain;
    }else{
        info["side_channel"]["risk"] = "safe";
        info["side_channel"]["explain"] = explain;
    }

    return info;
}
