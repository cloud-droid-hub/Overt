//
// Created by lxz on 2025/8/11.
//

#include <sys/stat.h>
#include "zLog.h"
#include "zLibc.h"
#include "zStdUtil.h"
#include "zShell.h"
#include "zLogcatInfo.h"
#include "zFile.h"

/**
 * 获取日志信息(查杀分离 — 采集端)
 * 遍历PID范围并grep出含 avc + u:r:su:s0 的日志行全量上报，不做风险判定；
 * Zygisk痕迹判定(u:r:su:s0)由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{"logcat_record" -> {value: "命中行"}} 或空
 */
map<string, map<string, string>> get_logcat_info(){
    map<string, map<string, string>> info;
    LOGE("get_logcat_info is called");

    // 遍历PID范围，检查每个进程的日志记录(全量收集，不立即判定返回)
    for(int i = 1500; i < 2000; i++){
        if(i % 10 == 0){
            LOGD("get_logcat_info pid %d", i);
        }

        string pid_str = to_string(i);
        string pid_path_str = "/proc/" + pid_str;
        string cmd_str = "logcat -d | grep avc | grep u:r:su:s0 | grep " + pid_str;

        struct stat st;
        if (stat(pid_path_str.c_str(), &st) != 0) {
            continue; // 进程不存在，跳过(原逻辑也只在stat成功时检查)
        }
        string ret = runShell(cmd_str);
        vector<string> ret_split = split_str(ret, '\n');

        // 全量收集命中的日志行(不内置过滤)
        for(string str : ret_split){
            if(strstr(str.c_str(), "u:r:su:s0")){
                LOGI("find zygiskd in logcat %s", str.c_str());
                info["logcat_record"]["value"] = str.c_str();
            }
        }
    }
    LOGI("logcat_info raw count=%zu", info.size());
    return info;
}