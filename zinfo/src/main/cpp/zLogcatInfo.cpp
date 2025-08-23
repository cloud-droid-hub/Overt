//
// Created by lxz on 2025/8/11.
//

#include <sys/stat.h>
#include "zLog.h"
#include "zLibc.h"
#include "zStdUtil.h"
#include "zShell.h"
#include "zLogcatInfo.h"

map<string, map<string, string>> get_logcat_info(){
    map<string, map<string, string>> info;
    for(int i = 1500; i < 2000; i++){
        if(i % 10 == 0){
            LOGE("test pid %d", i);
        }

        string pid_str = itoa(i, 10);

        string pid_path_str = "/proc/" + pid_str;
        string cmd_str = "logcat -d | grep avc | grep u:r:su:s0 | grep " + pid_str;

        struct stat st;
        stat(pid_path_str.c_str(), &st);
        string ret = runShell(cmd_str);

        if(strstr(ret.c_str(), "u:r:su:s0")){
            LOGE("find zygiskd in logcat %s", ret.c_str());
            info[ret.c_str()]["risk"] = "error";
            info[ret.c_str()]["explain"] = "u:r:su:s0 find in logcat";
            break;
        }
    }
    return info;
}