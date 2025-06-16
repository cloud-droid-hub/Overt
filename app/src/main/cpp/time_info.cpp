//
// Created by lxz on 2025/6/12.
//

#include "time_info.h"
#include <android/log.h>
#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)
#include <unistd.h>
#include <limits.h> // PATH_MAX
#include <cstring>
#include <sys/types.h>
#include <optional>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <sys/syscall.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>


std::optional<struct statx> get_file_statx(std::string path) {
    struct statx stx = {};
    int res = syscall(291, AT_FDCWD, path.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BASIC_STATS, &stx);
//    int res = statx(AT_FDCWD, path.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BASIC_STATS, &stx);
    if (res == 0) {
        return stx;
    } else {
        return std::nullopt;
    }
}

std::optional<struct stat> get_file_stat(std::string path) {
    struct stat st;
    if (stat(path.c_str(), &st) == -1) {
        return std::nullopt;
    }
    return st;
}


std::string format_timestamp(long timestamp) {
    if (timestamp <= 0) {
        return "Invalid timestamp";
    }

    // 转换为本地时间
    time_t time = (time_t)timestamp;
    struct tm* timeinfo = localtime(&time);
    if (timeinfo == nullptr) {
        return "Failed to convert time";
    }

    // 格式化时间
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 输出调试信息
    LOGE("Timestamp conversion: input=%ld, year=%d, month=%d, day=%d, hour=%d, min=%d, sec=%d",
         timestamp, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
         timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    return std::string(buffer);
}

long get_earliest_time() {

    long startup_time = 0;

    std::string allResults;
    std::ifstream mounts("/proc/mounts");
    if (!mounts.is_open()) {
        return startup_time;
    }
    std::string line;
    while (std::getline(mounts, line)) {

        std::istringstream iss(line);
        std::string device, mountpoint, fstype, options;
        if (!(iss >> device >> mountpoint >> fstype >> options)) {
            continue; // 格式异常，跳过
        }
        // 只处理真实路径挂载点，排除某些虚拟fs如 proc, sysfs, tmpfs
        if (mountpoint.empty()) continue;
        if (fstype == "proc" || fstype == "sysfs" || fstype == "tmpfs" || fstype == "devtmpfs" || fstype == "devpts" || fstype == "cgroup") continue;

        // 魔改rom中这个路径有问题
        if(strncmp(mountpoint.c_str(), "/storage/emulated", strlen("/storage/emulated")) == 0) continue;

        // 这个目录下可能会有 com.google.android.gms 导致有问题
        if(strncmp(mountpoint.c_str(), "/data/user_de", strlen("/data/user_de")) == 0) continue;

        // 这个目录恢复出厂设置不会改变
        if(strncmp(mountpoint.c_str(), "/product", strlen("/product")) == 0) continue;

        // 这个目录恢复出厂设置不会改变
        if(strcmp(mountpoint.c_str(), "/") == 0) continue;

        std::optional<struct stat> statOpt = get_file_stat(mountpoint.c_str());

        struct stat stx = *statOpt;

        long modify_time = (long)stx.st_mtim.tv_sec;
        startup_time = (startup_time == 0 && modify_time > 1230768000) ? modify_time : startup_time;
        startup_time = (startup_time > modify_time && modify_time > 1230768000) ? modify_time : startup_time;

        long change_time = (long)stx.st_ctim.tv_sec;
        startup_time = (startup_time == 0 && change_time > 1230768000) ? change_time : startup_time;
        startup_time = (startup_time > change_time && change_time > 1230768000) ? change_time : startup_time;

        long access_time = (long)stx.st_atim.tv_sec;
        startup_time = (startup_time == 0 && access_time > 1230768000) ? access_time : startup_time;
        startup_time = (startup_time > access_time && access_time > 1230768000) ? access_time : startup_time;

        char buffer[512];
        snprintf(buffer, sizeof(buffer),"File: %s\n  Modify: %s\n  Change: %s\n  Access: %s\n",
                 mountpoint.c_str(),
                 format_timestamp(modify_time).c_str(),
                 format_timestamp(change_time).c_str(),
                 format_timestamp(access_time).c_str());
        LOGE("%s", buffer);
        sleep(0);

    }

    return startup_time;
}




long get_boot_time_by_syscall() {
    struct sysinfo info;

    // 使用 syscall(63, ...) 也就是 SYS_sysinfo
    int result = syscall(__NR_sysinfo, &info);
    if (result != 0) {
        LOGE("syscall SYS_sysinfo failed");
        return -1;
    }

    time_t now = time(nullptr);
    time_t boot_time = now - info.uptime;

    LOGE("Boot time: %ld", (long)boot_time);

    return boot_time;
}


std::string get_time_diff(long timestamp) {
    if (timestamp <= 0) {
        return "Invalid timestamp";
    }

    // 获取当前时间
    time_t current_time = time(nullptr);

    // 计算时间差（秒）
    long time_diff = current_time - timestamp;
    if (time_diff < 0) {
        return "Invalid time difference";
    }

    // 计算天、时、分、秒
    long days = time_diff / (24 * 60 * 60);
    long hours = (time_diff % (24 * 60 * 60)) / (60 * 60);
    long minutes = (time_diff % (60 * 60)) / 60;
    long seconds = time_diff % 60;

    // 格式化时间差
    char time_str[256];
    snprintf(time_str, sizeof(time_str),
             "时间间隔: %ld天%ld小时%ld分钟%ld秒",
             days, hours, minutes, seconds);

    // 输出调试信息
    LOGE("Time difference calculation: current=%ld, input=%ld, diff=%ld, days=%ld, hours=%ld, minutes=%ld, seconds=%ld",
         current_time, timestamp, time_diff, days, hours, minutes, seconds);

    return std::string(time_str);
}

std::map<std::string, std::string> get_time_info(){

    std::map<std::string, std::string> info;

    // 获取最早时间
    long earliest_time = get_earliest_time();
    std::string earliest_time_str = format_timestamp(earliest_time);
    LOGE("earliest_startup_time %ld %s", earliest_time, earliest_time_str.c_str());

    // 获取开机时间
    long boot_time = get_boot_time_by_syscall();
    std::string boot_time_str = format_timestamp(boot_time);
    LOGE("boot_time %ld %s", boot_time, format_timestamp(boot_time).c_str());

    LOGE("earliest_startup_time2  %ld %s", earliest_time, get_time_diff(earliest_time).c_str());
    LOGE("boot_time_time2  %ld %s", boot_time, get_time_diff(boot_time).c_str());

    time_t current_time = time(nullptr);

    // 当前时间距离最早开机时间过短，可能刷机或恢复出厂设置
    if(current_time - earliest_time < 30 * 24 * 60 * 60){
        info["earliest_time"] = earliest_time_str;
    }

    LOGE("boot_time2 %s", format_timestamp(current_time - boot_time).c_str());
    // 开机时间过短，可能刚重启
    if(current_time - boot_time < 1 * 24 * 60 * 60){
        info["boot_time"] = boot_time_str;
    }

    return info;
}
