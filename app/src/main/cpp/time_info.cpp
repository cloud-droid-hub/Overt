//
// Created by lxz on 2025/6/12.
//

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

#include "zLog.h"
#include "zFile.h"
#include "zUtil.h"
#include "syscall.h"
#include "time_info.h"

string format_timestamp(long timestamp) {
    LOGE("format_timestamp: called with timestamp=%ld", timestamp);
    
    if (timestamp <= 0) {
        LOGE("format_timestamp: invalid timestamp (<=0): %ld", timestamp);
        return "Invalid timestamp";
    }

    // 转换为本地时间
    time_t time = (time_t)timestamp;
    LOGE("format_timestamp: converted to time_t: %ld", time);
    
    struct tm* timeinfo = localtime(&time);
    if (timeinfo == nullptr) {
        LOGE("format_timestamp: localtime failed for timestamp=%ld", timestamp);
        return "Failed to convert time";
    }

    // 格式化时间
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 输出调试信息
    LOGE("Timestamp conversion: input=%ld, year=%d, month=%d, day=%d, hour=%d, min=%d, sec=%d",
         timestamp, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
         timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    LOGE("format_timestamp: returning formatted string: %s", buffer);
    return string(buffer);
}

zFile get_earliest_file() {
    // 将 /proc/self/maps 作为默认值
    zFile earliest_file("/proc/self/maps");
    LOGE("初始最早时间: %s -> %s", earliest_file.getPath().c_str(), earliest_file.getEarliestTimeFormatted().c_str());

    // 遍历 /proc/mounts
    for(string line : zFile("/proc/mounts").readAllLines()){

        vector<string> tokens = split_str(line, ' ');
        if (tokens.size() < 4) {
            LOGE("mounts format is wrong: %s", line.c_str());
            continue;
        }

        string device = tokens[0];
        string mountpoint_path = tokens[1];
        string fstype = tokens[2];
        string options = tokens[3];

        // 只处理真实路径挂载点，排除某些虚拟fs如 proc, sysfs, tmpfs
        if (fstype == "proc" || fstype == "sysfs" || fstype == "tmpfs" || fstype == "devtmpfs" || fstype == "devpts" || fstype == "cgroup") {
            LOGE("跳过虚拟文件系统: %s", fstype.c_str());
            continue;
        }

        zFile mountpoint_file(mountpoint_path);

//        // 魔改rom中这个路径有问题
//        if(mountpoint_file.pathStartWith("/storage/emulated")){
//            LOGE("跳过存储路径: %s", mountpoint_file.getPath().c_str());
//            continue;
//        }
//
//        // 这个目录下可能会有 com.google.android.gms 导致有问题
//        if(mountpoint_file.pathStartWith("/data/user_de")){
//            LOGE("跳过用户数据路径: %s", mountpoint_file.getPath().c_str());
//            continue;
//        }
//
//        // 这个目录恢复出厂设置不会改变
//        if(mountpoint_file.pathStartWith("/product")){
//            LOGE("跳过产品路径: %s", mountpoint_file.getPath().c_str());
//            continue;
//        }
//
//        // 这个目录恢复出厂设置不会改变
//        if(mountpoint_file.pathEquals("/")){
//            LOGE("跳过根路径: %s", mountpoint_file.getPath().c_str());
//            continue;
//        }

        if(mountpoint_file.getUid() < 1000){
            continue;
        }

        // 屏蔽 2010 年之前的时间
        if(mountpoint_file.getEarliestTime() < 1262275200){
            continue;
        }

        LOGE("mountpoint_file %s %d %s", mountpoint_file.getPath().c_str(), mountpoint_file.getEarliestTime(), mountpoint_file.getEarliestTimeFormatted().c_str());
        sleep(0);

        if(mountpoint_file.getEarliestTime() < earliest_file.getEarliestTime()){
            earliest_file = mountpoint_file;
        }

    }

    LOGE("最终最早文件: %s -> %ld %s", earliest_file.getPath().c_str(), earliest_file.getEarliestTime(), earliest_file.getEarliestTimeFormatted().c_str());
    return earliest_file;
}

long get_boot_time_by_syscall() {
    struct sysinfo info;
    LOGE("get_boot_time_by_syscall: starting...");

    // 使用 syscall(63, ...) 也就是 SYS_sysinfo
    int result = __syscall1(__NR_sysinfo, (long)&info);
    LOGE("get_boot_time_by_syscall: sysinfo syscall result=%d, uptime=%ld, loads[0]=%ld, loads[1]=%ld, loads[2]=%ld", 
         result, info.uptime, info.loads[0], info.loads[1], info.loads[2]);
    
    if (result != 0) {
        LOGE("get_boot_time_by_syscall: syscall SYS_sysinfo failed: %d", result);
        return -1;
    }

    LOGE("get_boot_time_by_syscall: calling time...");
    time_t now = time(nullptr);
    LOGE("get_boot_time_by_syscall: time returned: %ld", now);
    
    time_t boot_time = now - info.uptime;
    LOGE("get_boot_time_by_syscall: calculation: %ld - %ld = %ld", now, info.uptime, boot_time);

    LOGE("get_boot_time_by_syscall: final boot_time: %ld", (long)boot_time);
    return boot_time;
}


string get_time_diff(long timestamp) {
    LOGE("get_time_diff: called with timestamp=%ld", timestamp);
    
    if (timestamp <= 0) {
        LOGE("get_time_diff: invalid timestamp (<=0): %ld", timestamp);
        return "Invalid timestamp";
    }

    // 获取当前时间
    LOGE("get_time_diff: calling time...");
    time_t current_time = time(nullptr);
    LOGE("get_time_diff: current_time=%ld", current_time);

    // 计算时间差（秒）
    long time_diff = current_time - timestamp;
    LOGE("get_time_diff: time_diff calculation: %ld - %ld = %ld", current_time, timestamp, time_diff);
    
    if (time_diff < 0) {
        LOGE("get_time_diff: negative time difference: %ld", time_diff);
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

    return string(time_str);
}

map<string, map<string, string>> get_time_info(){
    LOGE("get_time_info: starting...");

    map<string, map<string, string>> info;

    LOGE("get_time_info: calling time for current_time...");
    time_t current_time = time(nullptr);
    LOGE("get_time_info: current_time=%ld", current_time);

    // 获取开机时间
    LOGE("get_time_info: calling get_boot_time_by_syscall...");
    long boot_time = get_boot_time_by_syscall();
    LOGE("get_time_info: get_boot_time_by_syscall returned: %ld", boot_time);
    
    LOGE("get_time_info: calling format_timestamp...");
    string boot_time_str = format_timestamp(boot_time);
    LOGE("get_time_info: format_timestamp result: %s", boot_time_str.c_str());
    
    LOGE("boot_time %ld %s", boot_time, format_timestamp(boot_time).c_str());
    LOGE("boot_time_time2  %ld %s", boot_time, get_time_diff(boot_time).c_str());

    // 开机时间过短，可能刚重启
    long time_diff_seconds = current_time - boot_time;
    long one_day_seconds = 1 * 24 * 60 * 60;
    LOGE("get_time_info: time_diff_seconds=%ld, one_day_seconds=%ld", time_diff_seconds, one_day_seconds);
    LOGE("get_time_info: condition check: %ld < %ld = %s", time_diff_seconds, one_day_seconds, (time_diff_seconds < one_day_seconds) ? "true" : "false");
    
    if(time_diff_seconds < one_day_seconds){
        LOGE("get_time_info: adding boot_time info to map");
        
        // 使用更明确的方式创建嵌套map
        string key = "boot_time:" + boot_time_str;
        map<string, string> inner_map;
        inner_map["risk"] = "warn";
        inner_map["explain"] = "boot_time is too short";
        
        LOGE("get_time_info: created inner_map with size: %zu", inner_map.size());
        info[key] = inner_map;
        
        LOGE("get_time_info: info map size after adding: %zu", info.size());
    } else {
        LOGE("get_time_info: boot time is not too short, not adding to map");
    }

//    获取文件的最早时间，不太稳定，或许有问题
//    // 获取最早时间
//    zFile earliest_file = get_earliest_file();
//    LOGE("earliest_startup_time %ld %s", earliest_file.getEarliestTime(), earliest_file.getEarliestTimeFormatted().c_str());

//    // 当前时间距离最早开机时间过短，可能刷机或恢复出厂设置
//    if(current_time - earliest_file.getEarliestTime() < 30 * 24 * 60 * 60){
//        info["earliest_time:"+earliest_file.getEarliestTimeFormatted()]["risk"] = "warn";
//        info["earliest_time:"+earliest_file.getEarliestTimeFormatted()]["explain"] = "earliest_time is too short";
//    }

    LOGE("get_time_info: final info map size: %zu", info.size());
    for (const auto& entry : info) {
        LOGE("get_time_info: map entry - key: '%s', inner map size: %zu", entry.first.c_str(), entry.second.size());
        for (const auto& inner_entry : entry.second) {
            LOGE("get_time_info: inner map entry - key: '%s', value: '%s'", inner_entry.first.c_str(), inner_entry.second.c_str());
        }
    }
    return info;
}
