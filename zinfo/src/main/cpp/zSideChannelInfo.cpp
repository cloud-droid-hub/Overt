//
// Created by liuxi on 2025/10/29.
//

#include "zSideChannelInfo.h"
#include "syscall.h"
#include "zLog.h"

#define AT_FDCWD        -100

/**
 * 系统调用faccessat的封装函数
 * 用于检测文件访问权限
 * @param dirfd 目录文件描述符
 * @param pathname 文件路径
 * @param mode 访问模式
 * @param flags 标志位
 * @return 系统调用返回值
 */
static inline int sys_faccessat(int dirfd, const char *pathname, int mode, int flags)
{
    int ret = (int)syscall(SYS_faccessat, dirfd, pathname, mode, flags);
    return ret;          // -1 时 errno 已设置
}

/**
 * 系统调用fchownat的封装函数
 * 用于修改文件所有者
 * @param dirfd 目录文件描述符
 * @param pathname 文件路径
 * @param owner 所有者ID
 * @param group 组ID
 * @param flags 标志位
 * @return 系统调用返回值
 */
static inline int sys_fchownat(int dirfd, const char *pathname,
                               unsigned int owner, unsigned int group, int flags)
{
    int ret = (int)syscall(SYS_fchownat, dirfd, pathname, owner, group, flags);
    return ret;
}

/**
 * 获取当前时间（纳秒）
 * 使用CLOCK_BOOTTIME时钟获取系统启动后的时间
 * @return 当前时间戳（纳秒）
 */
static inline uint64_t now_ns(void)
{
    struct timespec ts = {};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}


/**
 * 获取侧信道信息的主函数
 * 通过侧信道攻击检测系统环境异常
 * 通过比较不同系统调用的执行时间来判断是否存在调试工具或Hook框架
 * @return 包含检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_side_channel_info(){
    map<string, map<string, string>> info;
    int times1[7000] = {0};  // 存储faccessat系统调用的执行时间
    int times2[7000] = {0};  // 存储fchownat系统调用的执行时间

    // 执行7000次faccessat系统调用，记录每次的执行时间
    for(int i = 0; i < 7000; i++){
        uint64_t start_time = now_ns();
        __syscall4(SYS_faccessat, AT_FDCWD, (long)"/data/local/tmp/foo", R_OK, 0);
        uint64_t end_time = now_ns();
        times1[i] = (int)(end_time - start_time);
    }

    // 执行7000次fchownat系统调用，记录每次的执行时间
    for(int i = 0; i < 7000; i++){
        uint64_t start_time = now_ns();
        __syscall5(SYS_fchownat, AT_FDCWD, (long)"/data/local/tmp/foo", 1000, 1000, 0);
        uint64_t end_time = now_ns();
        times2[i] = (int)(end_time - start_time);
    }

    // 一般来说 faccessat 的执行速度是要比 fchownat 快的，如果 faccessat 出现大量慢于 fchownat 的情况，那么说明环境有异常
    int error_count = 0;
    for(int i = 0; i < 7000; i++){
        if(times1[i] > times2[i]){
            error_count++;
        }
    }

    LOGE("error_count: %d", error_count);

    // 如果异常次数超过5000次，则认为环境存在异常
    if(error_count > 5000){
        info["side_channel"]["risk"] = "error";
        info["side_channel"]["explain"] = "faccessat is slower than fchownat";
    }

    return info;
}
