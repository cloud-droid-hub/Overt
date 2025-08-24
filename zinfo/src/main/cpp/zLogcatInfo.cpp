//
// Created by lxz on 2025/8/11.
//

#include <sys/stat.h>
#include "zLog.h"
#include "zLibc.h"
#include "zStdUtil.h"
#include "zShell.h"
#include "zLogcatInfo.h"

// 这个检测有点不太稳定，一会儿能查到一会儿查不到，但只要查到了肯定是安装了 zygiskd
map<string, map<string, string>> get_logcat_info(){
    map<string, map<string, string>> info;
    for(int i = 1500; i < 2000; i++){
        if(i % 10 == 0){
            LOGD("get_logcat_info pid %d", i);
        }

        string pid_str = to_string(i);
        string pid_path_str = "/proc/" + pid_str;
        string cmd_str = "logcat -d | grep avc | grep u:r:su:s0 | grep " + pid_str;

        struct stat st;
        stat(pid_path_str.c_str(), &st);
        string ret = runShell(cmd_str);
        vector<string> ret_split = split_str(ret, '\n');
        for(string str : ret_split){
            if(strstr(str.c_str(), "u:r:su:s0")){
                LOGI("find zygiskd in logcat %s", str.c_str());
                info["logcat_record"]["risk"] = "error";
                info["logcat_record"]["explain"] = str.c_str();
                return info;
            }
        }
    }
    return info;
}