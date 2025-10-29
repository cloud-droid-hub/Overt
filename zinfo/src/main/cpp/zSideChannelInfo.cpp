//
// Created by liuxi on 2025/10/29.
//

#include "zSideChannelInfo.h"
#include "syscall.h"
#include "zLog.h"

#define AT_FDCWD        -100

static inline int sys_faccessat(int dirfd, const char *pathname, int mode, int flags)
{
    int ret = (int)syscall(SYS_faccessat, dirfd, pathname, mode, flags);
    return ret;          // -1 时 errno 已设置
}

static inline int sys_fchownat(int dirfd, const char *pathname,
                               unsigned int owner, unsigned int group, int flags)
{
    int ret = (int)syscall(SYS_fchownat, dirfd, pathname, owner, group, flags);
    return ret;
}

static inline uint64_t now_ns(void)
{
    struct timespec ts = {};
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}


map<string, map<string, string>> get_side_channel_info(){
    map<string, map<string, string>> info;
    int times1[7000] = {0};
    int times2[7000] = {0};

    for(int i = 0; i < 7000; i++){
        uint64_t start_time = now_ns();
        __syscall4(SYS_faccessat, AT_FDCWD, (long)"/data/local/tmp/foo", R_OK, 0);
        uint64_t end_time = now_ns();
        times1[i] = (int)(end_time - start_time);
    }

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

    if(error_count > 5000){
        info["side_channel"]["risk"] = "error";
        info["side_channel"]["explain"] = "faccessat is slower than fchownat";
    }

    return info;
}
