//
// Created by lxz on 2025/6/6.
//


#include "zLog.h"
#include "zFile.h"
#include "zRootStateInfo.h"

/**
 * 获取Root文件信息(查杀分离 — 采集端)
 * 只遍历并上报常见Root相关文件路径的存在状态(全量原始数据)，不做任何风险判定；
 * 风险判定(哪些路径存在应判为风险)由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{文件路径 -> {value: "1"存在 / "0"不存在}}
 */
map<string, map<string, string>> get_root_state_info(){
    LOGD("get_root_file_info called");
    map<string, map<string, string>> info;

    // 采集范围：常见Root相关文件路径(仅WHERE to look，不是风险名单)
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
            "/apex/com.android.runtime/bin/suu",
    };

    // 遍历检测每个路径，全量上报存在状态，不做过滤
    for (const char* path : paths) {
        LOGI("Checking path: %s", path);
        zFile file(path);
        info[path]["value"] = file.exists() ? "1" : "0";
    }

    LOGI("root_state_info raw count=%zu", info.size());
    return info;
}
