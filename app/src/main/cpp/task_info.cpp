//
// Created by lxz on 2025/7/12.
//

#include <unistd.h>

#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zFile.h"
#include "zDevice.h"
#include "task_info.h"

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