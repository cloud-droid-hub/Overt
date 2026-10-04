//
// Created by lxz on 2025/8/11.
//

#include <sys/stat.h>
#include "zLog.h"
#include "zLibc.h"
#include "zStdUtil.h"
#include "zShell.h"
#include "zSelinuxInfo.h"
#include "zFile.h"

/**
 * 获取 SELinux 检测信息(查杀分离 — 采集端)
 * 合并自原 zLogcatInfo(zygisk 痕迹) + 新增 SELinux context 探测：
 *   1. 进程 SELinux context(/proc/self/attr/current)——正常 app 为 untrusted_app 域,
 *      若为 magisk/su 域则说明被提权(只上报原始值,判定交 zengine)
 *   2. logcat zygisk 痕迹(u:r:su:s0 审计)——zygisk 注入的 SELinux 侧证据
 * 只上报原始数据，不做风险判定。
 * @return 包含原始数据的Map，格式：
 *   {"selinux_context" -> {value: "当前SELinux上下文"}} +
 *   {"logcat_record" -> {value: "zygisk命中行"}}(如有)
 */
map<string, map<string, string>> get_selinux_info(){
    map<string, map<string, string>> info;

    // —— 1) 进程 SELinux context 探测 ——
    // 正常 app 上下文形如 "u:r:untrusted_app:s0"；若为 "u:r:magisk:s0" 或含 "su" 域
    // 标记，说明进程被 root 框架提权。只上报原始值，由 zengine 判定。
    // 这个检测点是从 https://github.com/WsttXm/RiskEngine 抄过来的，我的手机没有检测到，可能和版本有关
    zFile attr_current("/proc/self/attr/current");
    if (attr_current.exists()) {
        string context = attr_current.readAllText();
        // 去除尾部换行/空字符
        while (!context.empty() && (context.back() == '\n' || context.back() == '\r' || context.back() == '\0')) {
            context.pop_back();
        }
        info["selinux_context"]["value"] = context;
        LOGI("selinux_context: %s", context.c_str());
    } else {
        LOGW("selinux_context: /proc/self/attr/current unreadable");
        info["selinux_context"]["value"] = "unreadable";
    }

    // —— 2) logcat zygisk 痕迹(原 zLogcatInfo 逻辑保留) ——
    // 合并自 zLogcatInfo：遍历 PID 范围,grep 出含 avc + u:r:su:s0 的日志行全量上报。
    LOGE("get_selinux_info: scanning logcat for zygisk traces");

    for(int i = 1500; i < 2000; i++){
        if(i % 10 == 0){
            LOGD("get_selinux_info pid %d", i);
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
    LOGI("selinux_info raw count=%zu", info.size());
    return info;
}