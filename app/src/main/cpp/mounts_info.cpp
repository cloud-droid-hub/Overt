//
// Created by lxz on 2025/6/6.
//

#include "mounts_info.h"
#include "util.h"

std::map<std::string, std::map<std::string, std::string>> get_mounts_info(){
    std::map<std::string, std::map<std::string, std::string>> info;

    const char* paths[] = {
            "dex2oat",// hunter 似乎认为 dex2oat 存在是不合理的
            "APatch",
            "shamiko",
    };

    std::vector<std::string> mounts_lines = get_file_lines("/proc/self/mounts");
    for(int i = 0; i < mounts_lines.size(); i++){
        for (const char* path : paths) {
            if (strstr(mounts_lines[i].c_str(), path) != nullptr){
                LOGE("check_mounts error %d %s", i, mounts_lines[i].c_str());
                info[mounts_lines[i].c_str()]["risk"] = "error";
                info[mounts_lines[i].c_str()]["explain"] = "black name but in system path";

            }else if(strstr(mounts_lines[i].c_str(), "/system ") != nullptr && strstr(mounts_lines[i].c_str(), "overlay") != nullptr){
                LOGE("check_mounts error %d %s", i, mounts_lines[i].c_str());
                info[mounts_lines[i].c_str()]["risk"] = "error";
                info[mounts_lines[i].c_str()]["explain"] = "black name but in system path";
            }
        }
    }
    return info;
}
