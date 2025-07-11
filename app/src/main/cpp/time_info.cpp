//
// Created by lxz on 2025/6/12.
//

#include "time_info.h"
#include "zLog.h"
#include "zFile.h"
#include "util.h"

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

zFile get_earliest_file() {
    // 初始化最早文件为 /proc/self/maps
    zFile earliest_file("/proc/self/maps");
    time_t earliest_time = earliest_file.getEarliestTime();
    LOGE("初始最早时间: %s -> %ld", earliest_file.getPath().c_str(), earliest_time);

    std::vector<zFile> mount_file_list;

    zFile mounts("/proc/mounts");
    std::vector<std::string> mount_lines = mounts.readAllLines();

    for(std::string line : mount_lines){
        LOGE("mounts: %s", line.c_str());

        std::vector<std::string> tokens = split_str(line, ' ');
        if (tokens.size() < 4) {
            LOGE("mounts行格式错误: %s", line.c_str());
            continue;
        }

        std::string device = tokens[0];
        std::string mountpoint = tokens[1];
        std::string fstype = tokens[2];
        std::string options = tokens[3];

        LOGE("mountpoint: %s, fstype: %s", mountpoint.c_str(), fstype.c_str());

        // 只处理真实路径挂载点，排除某些虚拟fs如 proc, sysfs, tmpfs
        if (fstype == "proc" || fstype == "sysfs" || fstype == "tmpfs" || fstype == "devtmpfs" || fstype == "devpts" || fstype == "cgroup") {
            LOGE("跳过虚拟文件系统: %s", fstype.c_str());
            continue;
        }

        // 魔改rom中这个路径有问题
        if(strncmp(mountpoint.c_str(), "/storage/emulated", strlen("/storage/emulated")) == 0) {
            LOGE("跳过存储路径: %s", mountpoint.c_str());
            continue;
        }

        // 这个目录下可能会有 com.google.android.gms 导致有问题
        if(strncmp(mountpoint.c_str(), "/data/user_de", strlen("/data/user_de")) == 0) {
            LOGE("跳过用户数据路径: %s", mountpoint.c_str());
            continue;
        }

        // 这个目录恢复出厂设置不会改变
        if(strncmp(mountpoint.c_str(), "/product", strlen("/product")) == 0) {
            LOGE("跳过产品路径: %s", mountpoint.c_str());
            continue;
        }

        // 这个目录恢复出厂设置不会改变
        if(strcmp(mountpoint.c_str(), "/") == 0) {
            LOGE("跳过根路径: %s", mountpoint.c_str());
            continue;
        }

        zFile tmp_file(mountpoint);

        mount_file_list.push_back(tmp_file);

    }

    std::sort(mount_file_list.begin(), mount_file_list.end(), [](const zFile& a, const zFile& b) {
        return a.getEarliestTime() < b.getEarliestTime();
    });

    if (mount_file_list.size() < 2){
        return earliest_file;
    }

    // 删除时间差超过10年的文件
    for(int i = mount_file_list.size() - 2; i >= 0; i--) {
        time_t time_diff = mount_file_list[mount_file_list.size() - 1].getEarliestTime() - mount_file_list[i].getEarliestTime();
        if(time_diff > 10 * 12 * 30 * 24 * 60 * 60){
            LOGE("删除时间差超过10年的文件: %s (时间差: %ld秒)", mount_file_list[i].getPath().c_str(), time_diff);
            mount_file_list.erase(mount_file_list.begin() + i);
            sleep(0);
        }
    }

    // 如果还有文件，选择最早的文件
    if (!mount_file_list.empty()) {
        earliest_file = mount_file_list[0];
        LOGE("从挂载点中选择最早文件: %s -> %ld", earliest_file.getPath().c_str(), earliest_file.getEarliestTime());
    }

    LOGE("最终最早文件: %s -> %ld %s", earliest_file.getPath().c_str(), earliest_file.getEarliestTime(), earliest_file.getEarliestTimeFormatted().c_str());
    return earliest_file;
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
    snprintf(time_str, sizeof(time_str),"时间间隔: %ld天%ld小时%ld分钟%ld秒", days, hours, minutes, seconds);

    // 输出调试信息
    LOGE("Time difference calculation: current=%ld, input=%ld, diff=%ld, days=%ld, hours=%ld, minutes=%ld, seconds=%ld", current_time, timestamp, time_diff, days, hours, minutes, seconds);

    return std::string(time_str);
}

std::map<std::string, std::map<std::string, std::string>> get_time_info(){

    std::map<std::string, std::map<std::string, std::string>> info;

    // 获取最早时间
    zFile earliest_file = get_earliest_file();
    LOGE("earliest_startup_time %ld %s", earliest_file.getEarliestTime(), earliest_file.getEarliestTimeFormatted().c_str());

    // 获取开机时间
    long boot_time = get_boot_time_by_syscall();
    std::string boot_time_str = format_timestamp(boot_time);
    LOGE("boot_time %ld %s", boot_time, format_timestamp(boot_time).c_str());
    LOGE("boot_time_time2  %ld %s", boot_time, get_time_diff(boot_time).c_str());

    // 获取当前时间
    time_t current_time = time(nullptr);

    // 当前时间距离最早开机时间过短，可能刷机或恢复出厂设置
    if(current_time - earliest_file.getEarliestTime() < 30 * 24 * 60 * 60){
        info["earliest_time:"+earliest_file.getEarliestTimeFormatted()]["risk"] = "warn";
        info["earliest_time:"+earliest_file.getEarliestTimeFormatted()]["explain"] = "earliest_time is too short";
    }

    // 开机时间过短，可能刚重启
    if(current_time - boot_time < 1 * 24 * 60 * 60){
        info["boot_time:"+boot_time_str]["risk"] = "warn";
        info["boot_time:"+boot_time_str]["explain"] = "boot_time is too short";
    }

    return info;
}
