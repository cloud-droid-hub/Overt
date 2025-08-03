//
// Created by lxz on 2025/6/6.
//

#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include <linux/resource.h>
#include <sys/resource.h>

#include "zLog.h"
#include "zUtil.h"
#include "syscall.h"
#include "zFile.h"

#define MAX_CPU 8

string get_line(int fd) {
    LOGD("get_line called with fd: %d", fd);
    char buffer;
    string line = "";
    while (true) {
        ssize_t bytes_read = read(fd, &buffer, sizeof(buffer));
        if (bytes_read == 0) break;
        line += buffer;
        if (buffer == '\n') break;
    }
    return line;
}

vector<string> get_file_lines(string path){
    LOGD("get_file_lines called with path: %s", path.c_str());
    vector<string> file_lines = vector<string>();
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return file_lines;
    }
    while (true) {
        string line = get_line(fd);
        if (line.size() == 0) {
            break;
        }
        file_lines.push_back(line);
    }
    close(fd);
    return file_lines;
}

// 分割字符串，返回字符串数组，分割算法用 c 来实现，不利用 api
vector<string> split_str(const string& str, const string& split) {
    LOGD("split_str called with str: %s, split: %s", str.c_str(), split.c_str());
    vector<string> result;

    if (split.empty()) {
        result.push_back(str);  // 不分割
        return result;
    }

    const char* s = str.c_str();
    size_t str_len = str.length();
    size_t split_len = split.length();

    size_t i = 0;
    while (i < str_len) {
        size_t j = i;

        // 查找下一个分隔符位置
        while (j <= str_len - split_len && memcmp(s + j, split.c_str(), split_len) != 0) {
            ++j;
        }

        // 提取子串
        result.emplace_back(s + i, j - i);

        // 跳过分隔符
        i = (j <= str_len - split_len) ? j + split_len : str_len;
    }

    // 处理末尾刚好是分隔符的情况，需补一个空串
    if (str_len >= split_len && str.substr(str_len - split_len) == split) {
        result.emplace_back("");
    }

    return result;
}

vector<string> split_str(const string& str, char delim) {
    LOGD("split_str called with char delim: %c", delim);
    vector<string> result;
    const char* s = str.c_str();
    size_t start = 0;
    size_t len = str.length();

    for (size_t i = 0; i <= len; ++i) {
        if (s[i] == delim || s[i] == '\0') {
            // 只有当子串不为空时才添加到结果中
            if (i > start) {
                result.emplace_back(s + start, i - start);
            }
            start = i + 1;
        }
    }
    return result;
}

bool string_end_with(const char *str, const char *suffix) {
    LOGD("string_end_with called");
    if (!str || !suffix) {
        return false;
    }

    size_t len_str = strlen(str);
    size_t len_suffix = strlen(suffix);

    if (len_suffix > len_str) {
        return false;
    }

    return (strcmp(str + len_str - len_suffix, suffix) == 0);
}

bool string_start_with(const char *str, const char *prefix) {
  LOGD("string_start_with called");
  if (!str || !prefix) {
    return false;
  }

  size_t len_str    = strlen(str);
  size_t len_prefix = strlen(prefix);

  if (len_prefix > len_str) {
    return false;
  }

  return (strncmp(str, prefix, len_prefix) == 0);
}

// 将 map<string, string> 转换为 Java Map<String, String>
jobject cmap_to_jmap(JNIEnv *env, const map<string, string>& cmap){
    LOGD("cmap_to_jmap called, map size: %zu", cmap.size());
    LOGI("cmap_to_jmap: starting conversion, map size=%zu", cmap.size());
    LOGD("cmap_to_jmap: map type info - is nonstd::map: %s", typeid(cmap).name());
    
    // 遍历并打印map内容
    LOGD("cmap_to_jmap: map contents:");
    for (const auto &entry : cmap) {
        LOGD("cmap_to_jmap: key='%s', value='%s'", entry.first.c_str(), entry.second.c_str());
    }
    
    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGD("cmap_to_jmap: Failed to find HashMap class");
        return nullptr;
    }
    
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGD("cmap_to_jmap: Failed to find HashMap constructor");
        return nullptr;
    }
    
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGD("cmap_to_jmap: Failed to find HashMap put method");
        return nullptr;
    }
    
    // 创建HashMap对象
    jobject jobjectMap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jobjectMap) {
        LOGD("cmap_to_jmap: Failed to create HashMap object");
        return nullptr;
    }
    
    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGD("cmap_to_jmap: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }
    
    // 填充数据
    for (const auto &entry : cmap) {
        LOGD("cmap_to_jmap: processing entry: key='%s', value='%s'", 
             entry.first.c_str(), entry.second.c_str());
        
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        jstring jvalue = env->NewStringUTF(entry.second.c_str());
        
        if (!jkey || !jvalue) {
            LOGD("cmap_to_jmap: Failed to create string objects");
            if (jkey) env->DeleteLocalRef(jkey);
            if (jvalue) env->DeleteLocalRef(jvalue);
            continue;
        }
        
        env->CallObjectMethod(jobjectMap, putMethod, jkey, jvalue);
        
        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGD("cmap_to_jmap: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }
        
        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    
    LOGI("cmap_to_jmap: conversion completed successfully");
    return jobjectMap;
}

// 将 map<string, map<string, string>> 转换为 Java Map<String, Map<String, String>>
jobject cmap_to_jmap_nested(JNIEnv* env, const map<string, map<string, string>>& cmap) {
    LOGD("cmap_to_jmap_nested called, map size: %zu", cmap.size());
    LOGI("cmap_to_jmap_nested: starting conversion, map size=%zu", cmap.size());
    LOGD("cmap_to_jmap_nested: map type info - is nonstd::map: %s", typeid(cmap).name());
    
    // 遍历并打印map内容
    LOGD("cmap_to_jmap_nested: map contents:");
    for (const auto &entry : cmap) {
        LOGD("cmap_to_jmap_nested: key='%s', inner map size=%zu", entry.first.c_str(), entry.second.size());
        for (const auto &inner_entry : entry.second) {
            LOGD("cmap_to_jmap_nested: inner key='%s', value='%s'", inner_entry.first.c_str(), inner_entry.second.c_str());
        }
    }
    
    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap class");
        return nullptr;
    }
    
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap constructor");
        return nullptr;
    }
    
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap put method");
        return nullptr;
    }
    
    // 创建HashMap对象
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jmap) {
        LOGE("cmap_to_jmap_nested: Failed to create HashMap object");
        return nullptr;
    }
    
    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGE("cmap_to_jmap_nested: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }
    
    // 填充数据
    for (const auto& entry : cmap) {
        LOGD("cmap_to_jmap_nested: processing entry: key='%s', inner map size=%zu", 
             entry.first.c_str(), entry.second.size());
        
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        if (!jkey) {
            LOGE("cmap_to_jmap_nested: Failed to create key string");
            continue;
        }
        
        jobject jvalue = cmap_to_jmap(env, entry.second); // 调用之前定义的 cmap_to_jmap 方法
        if (!jvalue) {
            LOGE("cmap_to_jmap_nested: Failed to convert inner map");
            env->DeleteLocalRef(jkey);
            continue;
        }
        
        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);
        
        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap_nested: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }
        
        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    
    LOGI("cmap_to_jmap_nested: conversion completed successfully");
    return jmap;
}

// 主函数：将 map<string, map<string, map<string, string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
jobject cmap_to_jmap_nested_3(JNIEnv* env, const map<string, map<string, map<string, string>>>& cmap) {
    LOGD("cmap_to_jmap_nested_3 called, map size: %zu", cmap.size());
    LOGI("cmap_to_jmap_nested_3: starting conversion, map size=%zu", cmap.size());
    LOGD("cmap_to_jmap_nested_3: map type info - is nonstd::map: %s", typeid(cmap).name());
    
    // 遍历并打印map内容
    LOGD("cmap_to_jmap_nested_3: map contents:");
    for (const auto &entry : cmap) {
        LOGD("cmap_to_jmap_nested_3: key='%s', inner map size=%zu", entry.first.c_str(), entry.second.size());
        for (const auto &inner_entry : entry.second) {
            LOGD("cmap_to_jmap_nested_3: inner key='%s', inner inner map size=%zu", inner_entry.first.c_str(), inner_entry.second.size());
            for (const auto &inner_inner_entry : inner_entry.second) {
                LOGD("cmap_to_jmap_nested_3: inner inner key='%s', value='%s'", inner_inner_entry.first.c_str(), inner_inner_entry.second.c_str());
            }
        }
    }
    
    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap class");
        return nullptr;
    }
    
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap constructor");
        return nullptr;
    }
    
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap put method");
        return nullptr;
    }
    
    // 创建HashMap对象
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jmap) {
        LOGE("cmap_to_jmap_nested_3: Failed to create HashMap object");
        return nullptr;
    }
    
    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGE("cmap_to_jmap_nested_3: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }
    
    // 填充数据
    for (const auto& entry : cmap) {
        LOGD("cmap_to_jmap_nested_3: processing entry: key='%s', inner map size=%zu", 
             entry.first.c_str(), entry.second.size());
        
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        if (!jkey) {
            LOGE("cmap_to_jmap_nested_3: Failed to create key string");
            continue;
        }
        
        jobject jvalue = cmap_to_jmap_nested(env, entry.second);
        if (!jvalue) {
            LOGE("cmap_to_jmap_nested_3: Failed to convert inner map");
            env->DeleteLocalRef(jkey);
            continue;
        }
        
        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);
        
        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap_nested_3: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }
        
        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    
    LOGI("cmap_to_jmap_nested_3: conversion completed successfully");
    return jmap;
}


string format_timestamp(long timestamp) {
    LOGD("format_timestamp called with timestamp: %ld", timestamp);
    LOGI("format_timestamp: called with timestamp=%ld", timestamp);

    if (timestamp <= 0) {
        LOGE("format_timestamp: invalid timestamp (<=0): %ld", timestamp);
        return "Invalid timestamp";
    }

    // 转换为本地时间
    time_t time = (time_t)timestamp;
    LOGD("format_timestamp: converted to time_t: %ld", time);

    struct tm* timeinfo = localtime(&time);
    if (timeinfo == nullptr) {
        LOGE("format_timestamp: localtime failed for timestamp=%ld", timestamp);
        return "Failed to convert time";
    }

    // 格式化时间
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 输出调试信息
    LOGD("format_timestamp: input=%ld, year=%d, month=%d, day=%d, hour=%d, min=%d, sec=%d",
         timestamp, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
         timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    LOGI("format_timestamp: returning formatted string: %s", buffer);
    return string(buffer);
}

pid_t gettid() {
    return __syscall0(SYS_gettid);
}

vector<int> get_big_core_list() {
    LOGI("get_big_core_list called");
    
    vector<int> big_cores;
    int max_freq = 0;
    int freqs[MAX_CPU] = {0};

    LOGD("Scanning CPU frequencies for %d CPUs", MAX_CPU);
    for (int cpu = 0; cpu < MAX_CPU; ++cpu) {

        string path = string_format("/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu);

        LOGV("Checking CPU %d frequency file: %s", cpu, path.c_str());

        zFile cpuinfo_max_freq_file(path);

        string cpu_freq_str = cpuinfo_max_freq_file.readAllText();
        LOGE("freq_str %s", cpu_freq_str.c_str());

        int cpu_freq = atoi(cpu_freq_str.c_str());
        LOGE("cpu_freq %d", cpu_freq);

        freqs[cpu] = cpu_freq;
        if (cpu_freq > max_freq) {
            max_freq = cpu_freq;
            LOGI("New max frequency found: %d kHz (CPU %d)", max_freq, cpu);
        }
    }

    LOGI("Max frequency found: %d kHz", max_freq);
    LOGD("Identifying big cores with max frequency...");
    
    for (int i = 0; i < MAX_CPU; ++i) {
        if (freqs[i] == max_freq) {
            big_cores.push_back(i);
            LOGI("Added CPU %d as big core (freq: %d kHz)", i, freqs[i]);
        }
    }

    LOGI("Found %zu big cores out of %d CPUs", big_cores.size(), MAX_CPU);
    return big_cores;
}

void bind_self_to_least_used_big_core() {
    LOGI("bind_self_to_least_used_big_core called");
    
    LOGD("Getting list of big cores...");
    vector<int> big_cores = get_big_core_list();
    
    if (big_cores.empty()) {
        LOGW("No big cores found, falling back to CPU 0");
        // fallback绑定CPU0
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        pid_t tid = gettid();
        LOGD("Attempting to bind thread %d to fallback CPU 0", tid);
        
        int ret = sched_setaffinity(tid, sizeof(cpuset), &cpuset);
        if (ret == 0) {
            LOGI("Successfully bound thread %d to fallback CPU 0", tid);
        } else {
            LOGE("Failed to bind thread %d to fallback CPU 0 (errno: %d: %s)", 
                 tid, errno, strerror(errno));
        }
        return;
    }

    LOGI("Found %zu big cores: [", big_cores.size());
    for (size_t i = 0; i < big_cores.size(); ++i) {
        if (i > 0) LOGI(", ");
        LOGI("%d", big_cores[i]);
    }
    LOGI("]");

    LOGD("Initializing CPU affinity set for big cores...");
    cpu_set_t set;
    // 初始化清空 CPU 集合，否则可能残留旧数据，可能引发未预期的绑定行为。
    CPU_ZERO(&set);
    
    // 加入目标 CPU 核心 id
    LOGD("Adding big cores to CPU affinity set:");
    for (size_t i = 0; i < big_cores.size(); ++i) {
        CPU_SET(big_cores[i], &set);
        LOGD("  Added CPU %d to affinity set", big_cores[i]);
    }
    
    // 将一个进程或线程的调度限制在指定的 CPU 核心集合中运行。
    pid_t tid = gettid();
    LOGD("Attempting to bind thread %d to big core set (size: %zu)", tid, big_cores.size());
    
    int ret = sched_setaffinity(tid, sizeof(set), &set);
    if (ret == 0) {
        LOGI("Successfully bound thread %d to big core set", tid);
        
        // 验证绑定结果
        cpu_set_t verify_set;
        CPU_ZERO(&verify_set);
        if (sched_getaffinity(tid, sizeof(verify_set), &verify_set) == 0) {
            LOGD("Verification - current CPU affinity:");
            for (int cpu = 0; cpu < MAX_CPU; ++cpu) {
                if (CPU_ISSET(cpu, &verify_set)) {
                    LOGD("  CPU %d is in affinity set", cpu);
                }
            }
        } else {
            LOGW("Failed to verify CPU affinity (errno: %d: %s)", errno, strerror(errno));
        }
    } else {
        LOGE("Failed to bind thread %d to big core set (errno: %d: %s)", 
             tid, errno, strerror(errno));
        if (errno == EINVAL) {
            LOGE("Invalid CPU set or size");
        } else if (errno == EPERM) {
            LOGE("Permission denied - insufficient privileges");
        }
    }
}

void raise_thread_priority(int nice_priority) {
    LOGI("raise_thread_priority called - nice_priority: %d", nice_priority);

    if (nice_priority > 19 || nice_priority < -20) {
        LOGE("Invalid nice value: %d (range: -20 to 19)", nice_priority);
        return;
    }

    pid_t tid = gettid();
    LOGD("Current thread ID: %d", tid);

    int ret = setpriority(PRIO_PROCESS, tid, nice_priority);
    if (ret != 0) {
        LOGE("setpriority failed for thread %d (errno: %d: %s)", tid, errno, strerror(errno));
        if (errno == EPERM) {
            LOGE("Permission denied: You need root or CAP_SYS_NICE to increase priority (negative nice value)");
        }
    } else {
        int actual = getpriority(PRIO_PROCESS, tid);
        LOGI("Successfully set thread %d priority to %d (actual nice: %d)", tid, nice_priority, actual);
    }
}
