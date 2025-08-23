//
// Created by lxz on 2025/6/12.
//

#include <sys/sysinfo.h>
#include "syscall.h"
#include <sys/syscall.h>
#include "zLibc.h"
#include "zLog.h"
#include "zFile.h"
#include "zHttps.h"
#include "zJson.h"

#include "zTimeInfo.h"

zFile get_earliest_file() {
    // 将 /proc/self/maps 作为默认值
    zFile earliest_file("/proc/self/maps");
    LOGI("初始最早时间: %s -> %s", earliest_file.getPath().c_str(), earliest_file.getEarliestTimeFormatted().c_str());

    // 遍历 /proc/mounts
    for(string line : zFile("/proc/mounts").readAllLines()){

        vector<string> tokens = split_str(line, ' ');
        if (tokens.size() < 4) {
            LOGD("mounts format is wrong: %s", line.c_str());
            continue;
        }

        string device = tokens[0];
        string mountpoint_path = tokens[1];
        string fstype = tokens[2];
        string options = tokens[3];

        // 只处理真实路径挂载点，排除某些虚拟fs如 proc, sysfs, tmpfs
        if (fstype == "proc" || fstype == "sysfs" || fstype == "tmpfs" || fstype == "devtmpfs" || fstype == "devpts" || fstype == "cgroup") {
            LOGD("跳过虚拟文件系统: %s", fstype.c_str());
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

        LOGD("mountpoint_file %s %d %s", mountpoint_file.getPath().c_str(), mountpoint_file.getEarliestTime(), mountpoint_file.getEarliestTimeFormatted().c_str());
        sleep(0);

        if(mountpoint_file.getEarliestTime() < earliest_file.getEarliestTime()){
            earliest_file = mountpoint_file;
        }

    }

    LOGI("最终最早文件: %s -> %ld %s", earliest_file.getPath().c_str(), earliest_file.getEarliestTime(), earliest_file.getEarliestTimeFormatted().c_str());
    return earliest_file;
}

string get_time_diff(long timestamp) {
  LOGD("get_time_diff: called with timestamp=%ld", timestamp);

  if (timestamp <= 0) {
            LOGW("get_time_diff: invalid timestamp (<=0): %ld", timestamp);
    return "Invalid timestamp";
  }

  // 获取当前时间
  LOGD("get_time_diff: calling time...");
  time_t current_time = time(nullptr);
  LOGD("get_time_diff: current_time=%ld", current_time);

  // 计算时间差（秒）
  long time_diff = current_time - timestamp;
  LOGD("get_time_diff: time_diff calculation: %ld - %ld = %ld", current_time, timestamp, time_diff);

  if (time_diff < 0) {
            LOGW("get_time_diff: negative time difference: %ld", time_diff);
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
  LOGD("Time difference calculation: current=%ld, input=%ld, diff=%ld, days=%ld, hours=%ld, minutes=%ld, seconds=%ld", current_time, timestamp, time_diff, days, hours, minutes, seconds);

  return string(time_str);
}

long get_boot_time_by_syscall() {
    struct sysinfo info;
    LOGD("get_boot_time_by_syscall: starting...");

    // 使用 syscall(63, ...) 也就是 SYS_sysinfo
    int result = __syscall1(__NR_sysinfo, (long)&info);
    LOGD("get_boot_time_by_syscall: sysinfo syscall result=%d, uptime=%ld, loads[0]=%ld, loads[1]=%ld, loads[2]=%ld", 
         result, info.uptime, info.loads[0], info.loads[1], info.loads[2]);
    
    if (result != 0) {
        LOGE("get_boot_time_by_syscall: syscall SYS_sysinfo failed: %d", result);
        return -1;
    }

    LOGD("get_boot_time_by_syscall: calling time...");
    time_t now = time(nullptr);
    LOGD("get_boot_time_by_syscall: time returned: %ld", now);
    
    time_t boot_time = now - info.uptime;
    LOGD("get_boot_time_by_syscall: calculation: %ld - %ld = %ld", now, info.uptime, boot_time);

    LOGI("get_boot_time_by_syscall: final boot_time: %ld", (long)boot_time);
    return boot_time;
}

long get_local_current_time(){
  return time(nullptr);
}

long get_remote_current_time(){
  string pinduoduo_time_url = "https://api.pinduoduo.com/api/server/_stm";
  string pinduoduo_time_fingerprint_sha256 = "604D2DE1AD32FF364041831DE23CBFC2C48AD5DEF8E665103691B6472D07D4D0";

  zHttps https_client(5);
  HttpsRequest request(pinduoduo_time_url, "GET", 3);
  HttpsResponse response = https_client.performRequest(request);

  // 输出证书信息
  if (!response.error_message.empty()) {
            LOGW("Server error_message is not empty");
    return -1;
  }

  if (response.certificate.fingerprint_sha256 != pinduoduo_time_fingerprint_sha256) {
    LOGI("Server Certificate Fingerprint Local : %s", pinduoduo_time_fingerprint_sha256.c_str());
    LOGD("Server Certificate Fingerprint Remote: %s", response.certificate.fingerprint_sha256.c_str());
    return -1;
  }

  LOGD("get_time_info: pinduoduo_time: %s", response.body.c_str());

  zJson json(response.body);
  // 检查解析是否成功
  if (json.isError()) {
            LOGW("Failed to parse JSON response");
    return -1;
  }

  long pinduoduo_time = json.getLong("server_time", -1);
  LOGI("get_time_info: pinduoduo_time: %ld", pinduoduo_time/1000);
  return pinduoduo_time/1000;
}


map<string, map<string, string>> get_time_info(){
    LOGI("get_time_info: starting...");

    map<string, map<string, string>> info;

    time_t local_current_time = get_local_current_time();
    LOGI("get_time_info: current_time=%ld", local_current_time);
    string local_current_time_str = format_timestamp(local_current_time);
    LOGD("get_time_info: format_timestamp result: %s", local_current_time_str.c_str());

    long boot_time = get_boot_time_by_syscall();
    LOGI("get_time_info: get_boot_time_by_syscall returned: %ld", boot_time);
    string boot_time_str = format_timestamp(boot_time);
    LOGD("get_time_info: format_timestamp result: %s", boot_time_str.c_str());

    long remote_current_time = get_remote_current_time();
    LOGI("remote_current_time=%ld", remote_current_time);
    string remote_current_time_str = format_timestamp(remote_current_time);
    LOGD("remote_current_time_str: format_timestamp result: %s", remote_current_time_str.c_str());


    // 开机时间过短，可能刚重启
    long time_diff_seconds = local_current_time - boot_time;
    long one_day_seconds = 1 * 24 * 60 * 60;
    LOGD("get_time_info: time_diff_seconds=%ld, one_day_seconds=%ld", time_diff_seconds, one_day_seconds);
    LOGD("get_time_info: condition check: %ld < %ld = %s", time_diff_seconds, one_day_seconds, (time_diff_seconds < one_day_seconds) ? "true" : "false");
    
    if(time_diff_seconds < one_day_seconds){
        LOGI("get_time_info: adding boot_time info to map");
        info["boot_time"]["risk"] = "warn";
        info["boot_time"]["explain"] = "boot_time is too short " + boot_time_str;
    } else {
        LOGI("get_time_info: boot time is not too short");
        info["boot_time"]["risk"] = "safe";
        info["boot_time"]["explain"] = "boot_time is " + boot_time_str;
    }

    LOGD("remote_current_time diff: %ld", abs(remote_current_time - local_current_time));
    if(abs(remote_current_time - local_current_time) > 60){
        info["local_current_time"]["risk"] = "warn";
        info["local_current_time"]["explain"] = "local_current_time is " + local_current_time_str;
        info["remote_current_time"]["risk"] = "warn";
        info["remote_current_time"]["explain"] = "remote_current_time is " + remote_current_time_str;
    }else{
        info["local_current_time"]["risk"] = "safe";
        info["local_current_time"]["explain"] = "local_current_time is " + local_current_time_str;
        info["remote_current_time"]["risk"] = "safe";
        info["remote_current_time"]["explain"] = "remote_current_time is " + remote_current_time_str;
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

    LOGI("get_time_info: final info map size: %zu", info.size());
    for (const auto& entry : info) {
        LOGD("get_time_info: map entry - key: '%s', inner map size: %zu", entry.first.c_str(), entry.second.size());
        for (const auto& inner_entry : entry.second) {
            LOGD("get_time_info: inner map entry - key: '%s', value: '%s'", inner_entry.first.c_str(), inner_entry.second.c_str());
        }
    }
    return info;
}
