//
// Created by lxz on 2025/6/6.
//

#include <fcntl.h>


#include "zUtil.h"
#include "zLog.h"
#include "zFile.h"
#include "root_file_info.h"

map<string, map<string, string>> get_root_file_info(){
    LOGD("get_root_file_info called");
    map<string, map<string, string>> info;
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
        LOGD("Checking path: %s", path);
        zFile file(path);
        if(file.exists()){
            LOGI("Black file exists: %s", path);
            info[path]["risk"] = "error";
            info[path]["explain"] = "black file but exist";
        }
    }

    return info;
}
