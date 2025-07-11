//
// Created by lxz on 2025/6/6.
//


#include "root_file_info.h"
#include "util.h"
#include <fcntl.h>

#include "zLog.h"



std::map<std::string, std::map<std::string, std::string>> get_root_file_info(){
    std::map<std::string, std::map<std::string, std::string>> info;
    const char* paths[] = {
            "/sbin/su",
            "/system/bin/su",
            "/system/xbin/su",
            "/data/local/xbin/su",
            "/data/local/bin/su",
            "/system/sd/xbin/su",
            "/system/bin/failsafe/su",
            "/data/local/su",
            "/system/xbin/mu",
            "/system_ext/bin/su",
            "/apex/com.android.runtime/bin/suu"
    };

    for (const char* path : paths) {
        if (check_file_exist(path)) {
            info[path]["risk"] = "error";
            info[path]["explain"] = "black file but exist";
        }
    }

    return info;
}
