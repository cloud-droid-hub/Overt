//
// Created by liuxi on 2025/10/29.
//

#include <sched.h>
#include <cerrno>
#include "zSideChannelInfo.h"
#include "zLog.h"
#include "zStdUtil.h"
#include "zLibc.h"
#include <sys/syscall.h>


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
 * 获取侧信道信息(查杀分离 — 采集端)
 * 通过侧信道时序比较不同系统调用的执行时间，采集异常计数(原始数据)，
 * 不做风险判定；阈值判断(7000/5000)由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{side_channel -> {value: "异常次数"}}
 * 技术参考：https://bbs.kanxue.com/thread-288928.htm
 */
map<string, map<string, string>> get_side_channel_info(){
    map<string, map<string, string>> info;

    // 绑定到高性能核心以稳定测量（根据实际测试情况来看，这一步非常有必要）
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask); // 绑定到 CPU0，一般来讲，CPU0 都是大核

    pid_t tid = syscall(SYS_gettid); // 当前线程的 TID
    int result = sched_setaffinity(tid, sizeof(mask), &mask);

    if (result != 0) {
        int err = errno;
        LOGE("sched_setaffinity failed, errno=%d (%s)", err, strerror(err));
    }
    LOGI("Thread %d successfully bound to CPU0", tid);


    uint64_t times1[10000] = {0};  // 存储faccessat系统调用的执行时间
    uint64_t times2[10000] = {0};  // 存储fchownat系统调用的执行时间

    // 执行10000次faccessat系统调用，记录每次的执行时间
    for(int i = 0; i < 10000; i++){
        uint64_t start_time = raw_ns();
        syscall(SYS_faccessat, 0xFFFFFFFFLL, 0LL, 0xFFFFFFFFLL, 0LL);
        uint64_t end_time = raw_ns();
        times1[i] = end_time - start_time;
    }

    // 执行10000次fchownat系统调用，记录每次的执行时间
    for(int i = 0; i < 10000; i++){
        uint64_t start_time = raw_ns();
        syscall(SYS_fchownat, -1, 0LL, 0, 0, -1);
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

    // 全量上报异常计数(原始数据)，阈值判定由 zengine 负责
    info["side_channel"]["value"] = to_string(error_count);

    // —— KernelSU/APatch prctl 探测(机制级 root 检测) ——
    // KernelSU/APatch 是内核级 root,无文件痕迹;但它们通过私有 prctl option 0xDEADBEEF
    // 与用户态通信。普通内核不认识该 option → 返回 -EINVAL 且 out 参数不变;
    // 打了 KSU/APatch 补丁的内核会响应(写版本号 / 回魔数 / 返回 0)。
    // 只上报原始探测结果,判定(命中即风险)由 zengine 负责。
    // 这个检测点是从 https://github.com/WsttXm/RiskEngine 抄过来的，我的手机没有检测到，可能和版本有关
    {
        volatile int ksu_version = 0;
        volatile int ksu_reply = 0;
        long ksu_ret = syscall(SYS_prctl, 0xDEADBEEF, 2,
                               reinterpret_cast<unsigned long>(&ksu_version), 0,
                               reinterpret_cast<unsigned long>(&ksu_reply));
        bool ksu_present = (ksu_version != 0) || (ksu_reply == 0x5A5A5A5A) || (ksu_ret == 0);

        if (ksu_present) {
            string ksu_token = (ksu_version > 0)
                    ? string_format("v:%d", ksu_version)
                    : "present";
            info["ksu_prctl"]["value"] = ksu_token;
            LOGE("KernelSU prctl detected: %s", ksu_token.c_str());
        } else {
            info["ksu_prctl"]["value"] = "0";
            LOGI("KernelSU prctl not present (ret=%ld)", ksu_ret);
        }
    }

    return info;
}
