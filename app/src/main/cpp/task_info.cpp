//
// Created by lxz on 2025/7/12.
//

#include <unistd.h>

#include "zLog.h"
#include "zFile.h"
#include "zUtil.h"
#include "zDevice.h"
#include "nonstd_libc.h"
#include "task_info.h"

map<string, map<string, string>> get_task_info(){
    LOGD("[task_info] get_task_info called");
    map<string, map<string, string>> info;
    vector<string> task_dir_list = zFile("/proc/self/task").listDirectories();
    LOGI("[task_info] Found %zu task directories", task_dir_list.size());
    for(string task_dir : task_dir_list){
        LOGD("[task_info] Processing task_dir: %s", task_dir.c_str());
        string stat_path = "/proc/self/task/" + task_dir + "/stat";
        vector<string> stat_line_list = zFile(stat_path).readAllLines();
        for(string stat_line : stat_line_list) {
            LOGD("[task_info] Processing stat_line: %s", stat_line.c_str());
            if (strstr(stat_line.c_str(), "gamin") != nullptr) {
                LOGE("gmain is found in stat line");
                info[stat_line.c_str()]["risk"] = "error";
                info[stat_line.c_str()]["explain"] = "frida hooked this process";
            }
            if(strstr(stat_line.c_str(), "pool-frida") != nullptr){
                LOGE("pool-frida is found in stat line");
                info[stat_line.c_str()]["risk"] = "error";
                info[stat_line.c_str()]["explain"] = "frida hooked this process";
            }
        }
    }
    return info;
}