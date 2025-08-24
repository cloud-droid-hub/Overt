//
// Created by lxz on 2025/8/24.
//
#include "zLog.h"
#include "zLibc.h"
#include "zFile.h"

#include "zProcInfo.h"
#include "zStdUtil.h"

/**
 * 内存映射行信息结构体
 * 用于解析/proc/self/maps文件中的每一行信息
 */
struct maps_line_t {
    void* address_range_start;           // 地址范围开始
    void* address_range_end;             // 地址范围结束
    string permissions;            // 权限标志（如r--p, r-xp等）
    string file_offset;            // 文件偏移量
    string device_major_minor;     // 设备号（主设备号:次设备号）
    string inode;                  // 节点号
    string file_path;              // 文件路径
};

/**
 * 获取内存映射信息
 * 分析/proc/self/maps文件，检测关键系统库是否被篡改
 * 主要检测libart.so和libc.so等关键库的映射数量和权限是否正确
 * @return 包含检测结果的Map，格式：{库名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_maps_info(){
    LOGD("get_maps_info called");
    map<string, map<string, string>> info;

    // 按文件路径分组存储映射信息
    map<string, vector<maps_line_t>> maps_table;

    // 打开/proc/self/maps文件
    zFile file("/proc/self/maps");
    LOGD("Created zFile for /proc/self/maps");

    // 检查文件是否成功打开
    if (file.isOpen()) {
        LOGI("File opened successfully: %s", file.getPath().c_str());
    } else {
        LOGE("File open failed: %s", file.getPath().c_str());
        return info;
    }

    // 按行读取maps文件
    vector<string> lines = file.readAllLines();
    LOGI("Read %zu lines from file", lines.size());

    // 解析每一行映射信息
    for (size_t i = 0; i < lines.size(); i++) {
        LOGD("Processing line %zu", i + 1);

        // 只处理.so文件（动态链接库）
        if(string_end_with(lines[i].c_str(), ".so")){
            LOGD("Found .so in line %zu", i + 1);
            LOGD("Line %zu: %s", i + 1, lines[i].c_str());

            maps_line_t maps_line;

            // 按空格分割行内容
            vector<string> parts = split_str(lines[i], ' ');

            // 检查分割后的部分数量是否正确（应该有6个部分）
            if (parts.size() != 6){
                LOGE("Line %zu: insufficient parts %s", i + 1, lines[i].c_str());
                continue;
            }

            // 解析地址范围（格式：start-end）
            vector<string> address_range_parts = split_str(parts[0], '-');
            if (address_range_parts.size() == 2) {
                // 将十六进制字符串转换为指针地址
                maps_line.address_range_start = (void*)strtoul(address_range_parts[0].c_str(), nullptr, 16);
                maps_line.address_range_end  = (void*)strtoul(address_range_parts[1].c_str(), nullptr, 16);
            }

            // 解析其他字段
            maps_line.permissions = parts[1];           // 权限标志
            maps_line.file_offset = parts[2];           // 文件偏移
            maps_line.device_major_minor = parts[3];    // 设备号
            maps_line.inode = parts[4];                 // 节点号
            maps_line.file_path = parts[5];             // 文件路径

            LOGD("Address range: %p - %p permissions: %s file path: %s",
                 maps_line.address_range_start, maps_line.address_range_end,
                 maps_line.permissions.c_str(), maps_line.file_path.c_str());

            // 按文件路径分组存储映射信息
            maps_table[maps_line.file_path].push_back(maps_line);
            sleep(0); // 让出CPU时间片
        }
    }

    // 定义需要检查的关键库列表
    vector<string> check_lib_list = {
            "libart.so",    // Android运行时库
            "libc.so",      // C标准库
    };

    // 检查关键库的映射情况
    for (auto it = maps_table.begin(); it != maps_table.end(); it++){
        for(string lib_name : check_lib_list){
            if(string_end_with(it->first.c_str(), lib_name.c_str())){
                // 检查映射数量是否正确（正常情况下应该有4个映射）
                if(it->second.size() != 4){
                    LOGE("File path: %s, mapping count doesn't match expected: %zu",
                         it->first.c_str(), it->second.size());
                    info[lib_name]["risk"] = "error";
                    info[lib_name]["explain"] = "reference count error";
                }
                // 检查权限是否正确（正常情况下应该是r--p, r-xp, r--p, rw-p）
                else if(it->second.size() == 4 &&
                        (it->second[0].permissions!= "r--p" ||
                         it->second[1].permissions!= "r-xp" ||
                         it->second[2].permissions!= "r--p" ||
                         it->second[3].permissions!="rw-p")){
                    LOGE("File path: %s, mapping permissions don't match expected", it->first.c_str());
                    info[lib_name]["risk"] = "error";
                    info[lib_name]["explain"] = "permissions error";
                }
                break;
            }
        }
    }

    return info;
}

map<string, map<string, string>> get_mounts_info(){
    LOGI("get_mounts_info called");
    map<string, map<string, string>> info;

    // 定义需要检测的异常挂载点名称
    const char* paths[] = {
            "dex2oat",    // Hunter认为dex2oat存在是不合理的
            "APatch",     // APatch框架相关
            "shamiko",    // Shamiko模块相关
    };

    // 读取/proc/self/mounts文件，获取当前进程的挂载点信息
    vector<string> mounts_lines = zFile("/proc/self/mounts").readAllLines();
    LOGI("Read %zu lines from /proc/self/mounts", mounts_lines.size());

    // 遍历每一行挂载信息
    for(int i = 0; i < mounts_lines.size(); i++){
        LOGD("Processing line %d: %s", i, mounts_lines[i].c_str());

        // 检查是否包含异常挂载点名称
        for (const char* path : paths) {
            if (strstr(mounts_lines[i].c_str(), path) != nullptr){
                LOGE("check_mounts error %d %s", i, mounts_lines[i].c_str());
                info[mounts_lines[i].c_str()]["risk"] = "error";
                info[mounts_lines[i].c_str()]["explain"] = "black name but in system path";
            }
        }

        // 检查系统目录是否被overlay挂载（这通常表示系统被修改）
        if(strstr(mounts_lines[i].c_str(), "/system ") != nullptr &&
           strstr(mounts_lines[i].c_str(), "overlay") != nullptr){
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
map<string, map<string, string>> get_task_info(){
    LOGD("get_task_info called");
    map<string, map<string, string>> info;

    // 获取当前进程的所有任务目录列表
    vector<string> task_dir_list = zFile("/proc/self/task").listDirectories();
    LOGI("Found %zu task directories", task_dir_list.size());

    // 遍历每个任务目录
    for(string task_dir : task_dir_list){
        LOGD("Processing task_dir: %s", task_dir.c_str());

        // 构建线程状态文件路径
        string stat_path = "/proc/self/task/" + task_dir + "/stat";

        // 读取线程状态信息
        vector<string> stat_line_list = zFile(stat_path).readAllLines();

        // 分析每行状态信息
        for(string stat_line : stat_line_list) {
            LOGD("Processing stat_line: %s", stat_line.c_str());

            // 检测Frida注入的gmain线程
            if (strstr(stat_line.c_str(), "gamin") != nullptr) {
                LOGE("gmain is found in stat line");
                info[stat_line.c_str()]["risk"] = "error";
                info[stat_line.c_str()]["explain"] = "frida hooked this process";
            }

            // 检测Frida注入的pool-frida线程
            if(strstr(stat_line.c_str(), "pool-frida") != nullptr){
                LOGE("pool-frida is found in stat line");
                info[stat_line.c_str()]["risk"] = "error";
                info[stat_line.c_str()]["explain"] = "frida hooked this process";
            }
        }
    }
    return info;
}


map<string, map<string, string>> get_attr_prev_info(){
    LOGI("get_attr_prev_info called");
    map<string, map<string, string>> info;

    vector<string> lines = zFile("/proc/self/attr/prev").readAllLines();

    // 遍历每一行挂载信息
    for(string line : lines){
        LOGI("line %s", line.c_str());
        // 检测Frida注入的pool-frida线程
        if(strstr(line.c_str(), "zygote") != nullptr){
            LOGE("magisk is found in prev line");
            info[line.c_str()]["risk"] = "error";
            info[line.c_str()]["explain"] = "magisk is found in prev";
        }
    }

    return info;
}


map<string, map<string, string>> get_proc_info(){
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
    info.insert(task_info.begin(),   task_info.end());

    LOGI("get_attr_prev_info is called");
    map<string, map<string, string>> attr_prev_info = get_attr_prev_info();
    LOGI("get_attr_prev_info insert is called");
    info.insert(attr_prev_info.begin(), attr_prev_info.end());

    return info;
}