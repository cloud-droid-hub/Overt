//
// Created by lxz on 2025/7/10.
//
#include <linux/resource.h>
#include <sys/resource.h>
#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zFile.h"
#include "zDevice.h"
#include "syscall.h"

#define MAX_CPU 8

// 静态成员变量初始化
zDevice* zDevice::instance = nullptr;

// 设备信息存储Map初始化
map<string, map<string, map<string, string>>> zDevice::device_info;

/**
 * 构造函数实现
 * 初始化设备信息管理器
 */
zDevice::zDevice() {
    LOGD("Constructor called");
    // 初始化代码可以在这里添加
}

/**
 * 析构函数实现
 * 清理资源
 */
zDevice::~zDevice() {
    LOGD("Destructor called");
    // 清理代码可以在这里添加
}

/**
 * 获取所有设备信息
 * 使用读锁保证线程安全，允许多个线程同时读取
 * @return 三层嵌套的Map，包含所有收集到的设备信息
 */
const map<string, map<string, map<string, string>>>& zDevice::get_device_info() const{
    LOGD("get_device_info called");
    // 使用共享读锁，允许多个线程同时读取
    std::shared_lock<std::shared_mutex> lock(device_info_mtx_);
    LOGI("get_device_info: device_info size=%zu", device_info.size());
    return device_info;
};

/**
 * 更新设备信息
 * 使用写锁保证线程安全，确保数据一致性
 * @param key 信息类别（如"task_info", "maps_info"等）
 * @param value 该类别下的具体信息
 */
void zDevice::update_device_info(const string& key, const map<string, map<string, string>>& value){
    LOGD("update_device_info called, key=%s", key.c_str());
    // 使用独占写锁，确保更新操作的原子性
    std::unique_lock<std::shared_mutex> lock(device_info_mtx_);
    device_info[key] = value;
    LOGI("update_device_info: updated key=%s", key.c_str());
};

/**
 * 清空所有设备信息
 * 使用写锁保证线程安全
 * 通常在信息返回给Java层后调用，避免重复返回
 */
void zDevice::clear_device_info(){
    LOGD("clear_device_info called");
    // 使用独占写锁，确保清空操作的原子性
    std::unique_lock<std::shared_mutex> lock(device_info_mtx_);
    device_info.clear();
    LOGI("clear_device_info");
};



vector<int> zDevice::get_big_core_list() {
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


pid_t zDevice::gettid(){
    return __syscall0(SYS_gettid);
}

void zDevice::bind_self_to_least_used_big_core() {
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

void zDevice::raise_thread_priority(int nice_priority){
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