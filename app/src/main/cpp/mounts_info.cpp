//
// Created by lxz on 2025/6/6.
//

#include "mounts_info.h"
#include "zUtil.h"
#include "zLog.h"
#include "zFile.h"


map<string, map<string, string>> get_mounts_info(){
    LOGD("get_mounts_info called");
    map<string, map<string, string>> info;

    const char* paths[] = {
            "dex2oat",// hunter 似乎认为 dex2oat 存在是不合理的
            "APatch",
            "shamiko",
    };

    vector<string> mounts_lines = zFile("/proc/self/mounts").readAllLines();
    LOGI("Read %zu lines from /proc/self/mounts", mounts_lines.size());
    for(int i = 0; i < mounts_lines.size(); i++){
        LOGD("Processing line %d: %s", i, mounts_lines[i].c_str());
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
