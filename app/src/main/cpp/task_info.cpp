//
// Created by lxz on 2025/7/12.
//

#include "task_info.h"

#include <unistd.h>
#include "zLog.h"
#include "zFile.h"
#include "util.h"
#include "zDevice.h"

map<string, map<string, string>> get_task_info(){
    map<string, map<string, string>> info;
    vector<string> task_dir_list = zFile("/proc/self/task").listDirectories();
    for(string task_dir : task_dir_list){
        string stat_path = "/proc/self/task/" + task_dir + "/stat";
        vector<string> stat_line_list = zFile(stat_path).readAllLines();
        for(string stat_line : stat_line_list) {
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