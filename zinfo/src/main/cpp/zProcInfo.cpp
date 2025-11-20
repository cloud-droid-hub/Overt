//
// Created by lxz on 2025/8/24.
//
#include "zLog.h"
#include "zLibc.h"
#include "zFile.h"

#include "zProcInfo.h"
#include "zStdUtil.h"
#include "zProcMaps.h"

/**
 * 获取内存映射信息
 * 分析/proc/self/maps文件，检测关键系统库是否被篡改
 * 主要检测libart.so和libc.so等关键库的映射数量和权限是否正确
 * @return 包含检测结果的Map，格式：{库名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_maps_info() {
    LOGD("get_maps_info called");
    map<string, map<string, string>> info;

    zProcMaps maps = zProcMaps();

    // 定义需要检查的关键库列表
    vector<string> check_lib_list = {
            "libart.so",    // Android运行时库
            "libc.so",      // C标准库
            "libinput.so",      // 输入库
    };

    for (string lib_name: check_lib_list) {
        maps_so_t* maps_so = maps.find_so_by_name(lib_name);
        if(maps_so == nullptr) continue;
        // 检查映射数量是否正确（正常情况下应该有4个映射）
        if(maps_so->lines.size() != 4) {
            info[lib_name]["risk"] = "error";
            info[lib_name]["explain"] = "reference count error";
        }
        // 检查权限是否正确（正常情况下应该是r--p, r-xp, r--p, rw-p）
        else if (maps_so->lines[0].permissions != "r--p" ||
                 (maps_so->lines[1].permissions != "r-xp" && maps_so->lines[1].permissions != "--xp") ||
                 (maps_so->lines[2].permissions != "r--p" && maps_so->lines[2].permissions != "rw-p") ||
                 (maps_so->lines[3].permissions != "rw-p" && maps_so->lines[3].permissions != "r--p")) {
            info[lib_name]["risk"] = "error";
            info[lib_name]["explain"] = "permissions error";
        }
    }

    return info;
}

/**
 * 获取挂载点信息
 * 检测/proc/self/mounts文件中的异常挂载点
 * 主要用于检测系统被修改的痕迹，如overlay挂载、可疑模块等
 * @return 包含检测结果的Map，格式：{挂载点信息 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_mounts_info() {
    LOGI("get_mounts_info called");
    map<string, map<string, string>> info;

    // 定义需要检测的异常挂载点名称
    const char *paths[] = {
            "dex2oat",    // Hunter认为dex2oat存在是不合理的
            "APatch",     // APatch框架相关
            "shamiko",    // Shamiko模块相关
    };

    // 读取/proc/self/mounts文件，获取当前进程的挂载点信息
    vector<string> mounts_lines = zFile("/proc/self/mounts").readAllLines();
    LOGI("Read %zu lines from /proc/self/mounts", mounts_lines.size());

    // 遍历每一行挂载信息
    for (int i = 0; i < mounts_lines.size(); i++) {
        LOGI("Processing line %d: %s", i, mounts_lines[i].c_str());

        // 检查是否包含异常挂载点名称
        for (const char *path: paths) {
            if (strstr(mounts_lines[i].c_str(), path) != nullptr) {
                LOGE("check_mounts error %d %s", i, mounts_lines[i].c_str());
                info[mounts_lines[i].c_str()]["risk"] = "error";
                info[mounts_lines[i].c_str()]["explain"] = "black name but in system path";
            }
        }

        // 检查系统目录是否被overlay挂载（这通常表示系统被修改）
        if (strstr(mounts_lines[i].c_str(), "/system ") != nullptr &&
            strstr(mounts_lines[i].c_str(), "overlay") != nullptr) {
            LOGE("check_mounts error %d %s", i, mounts_lines[i].c_str());
            info[mounts_lines[i].c_str()]["risk"] = "error";
            info[mounts_lines[i].c_str()]["explain"] = "black name but in system path";
        }
    }

    return info;
}

/**
 * 获取任务信息
 * 检测当前进程的所有线程，查找Frida等调试工具注入的痕迹
 * 通过分析/proc/self/task目录下的线程状态信息进行检测
 * @return 包含检测结果的Map，格式：{线程信息 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_task_info() {
    LOGD("get_task_info called");
    map<string, map<string, string>> info;

    // 获取当前进程的所有任务目录列表
    vector<string> task_dir_list = zFile("/proc/self/task").listDirectories();
    LOGI("Found %zu task directories", task_dir_list.size());

    // 遍历每个任务目录
    for (string task_dir: task_dir_list) {
        LOGD("Processing task_dir: %s", task_dir.c_str());

        // 构建线程状态文件路径
        string stat_path = "/proc/self/task/" + task_dir + "/stat";

        // 读取线程状态信息
        vector<string> stat_line_list = zFile(stat_path).readAllLines();

        // 分析每行状态信息
        for (string stat_line: stat_line_list) {
            LOGI("Processing stat_line: %s", stat_line.c_str());

            // 检测Frida注入的gmain线程
            if (strstr(stat_line.c_str(), "gamin") != nullptr) {
                LOGE("gmain is found in stat line");
                info[stat_line.c_str()]["risk"] = "error";
                info[stat_line.c_str()]["explain"] = "frida hooked this process";
            }

            // 检测Frida注入的pool-frida线程
            if (strstr(stat_line.c_str(), "pool-frida") != nullptr) {
                LOGE("pool-frida is found in stat line");
                info[stat_line.c_str()]["risk"] = "error";
                info[stat_line.c_str()]["explain"] = "frida hooked this process";
            }
        }
    }
    return info;
}


/**
 * 获取进程属性信息
 * 检测/proc/self/attr/prev文件中的进程属性信息
 * 主要用于检测Magisk等Root框架的痕迹
 * @return 包含检测结果的Map，格式：{属性信息 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_attr_prev_info() {
    LOGI("get_attr_prev_info called");
    map<string, map<string, string>> info;

    vector<string> lines = zFile("/proc/self/attr/prev").readAllLines();

    // 遍历每一行挂载信息
    for (string line: lines) {
        LOGI("line %s", line.c_str());
        // 检测Frida注入的pool-frida线程
        if (strstr(line.c_str(), "zygote") != nullptr) {
            LOGE("magisk is found in prev line");
            info[line.c_str()]["risk"] = "error";
            info[line.c_str()]["explain"] = "magisk is found in prev";
        }
    }

    return info;
}

/**
 * 获取网络TCP信息
 * 检测/proc/self/net/tcp文件中的网络连接信息
 * 主要用于检测Frida、IDA等调试工具的端口使用情况
 * @return 包含检测结果的Map，格式：{网络连接信息 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_net_tcp_info() {
    LOGI("get_net_tcp_info called");
    map<string, map<string, string>> info;

    // android7 之后没权限
    vector<string> lines = zFile("/proc/self/net/tcp").readAllLines();

    // 遍历每一行挂载信息
    for (string line: lines) {
        LOGI("line %s", line.c_str());

        if (strstr(line.c_str(), ":69A2") != nullptr || strstr(line.c_str(), ":69A3") != nullptr) {
            LOGE("black port is found in tcp line");
            info[line.c_str()]["risk"] = "error";
            info[line.c_str()]["explain"] = "find frida port";
        }
        if (strstr(line.c_str(), ":5D8A") != nullptr) {
            LOGE("black port is found in tcp line");
            info[line.c_str()]["risk"] = "error";
            info[line.c_str()]["explain"] = "find ida port";
        }
    }

    return info;
}

/**
 * 获取进程信息的主函数
 * 整合所有进程相关的检测功能，包括内存映射、挂载点、任务状态等
 * 通过多种检测手段综合分析进程的安全状态
 * @return 包含所有检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_proc_info() {
    map<string, map<string, string>> info;

    LOGI("get_maps_info is called");
    map<string, map<string, string>> maps_info = get_maps_info();
    LOGI("get_maps_info insert is called");
    info.insert(maps_info.begin(), maps_info.end());

    LOGI("get_mounts_info is called");
    map<string, map<string, string>> mounts_info = get_mounts_info();
    LOGI("get_mounts_info insert is called");
    info.insert(mounts_info.begin(), mounts_info.end());

    LOGI("get_task_info is called");
    map<string, map<string, string>> task_info = get_task_info();
    LOGI("get_task_info insert is called");
    info.insert(task_info.begin(), task_info.end());

    LOGI("get_attr_prev_info is called");
    map<string, map<string, string>> attr_prev_info = get_attr_prev_info();
    LOGI("get_attr_prev_info insert is called");
    info.insert(attr_prev_info.begin(), attr_prev_info.end());

    LOGI("get_net_tcp_info is called");
    map<string, map<string, string>> net_tcp_info = get_net_tcp_info();
    LOGI("get_net_tcp_info insert is called");
    info.insert(net_tcp_info.begin(), net_tcp_info.end());

    return info;
}