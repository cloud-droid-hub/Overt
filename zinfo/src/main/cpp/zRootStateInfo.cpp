//
// Created by lxz on 2025/6/6.
//


#include "zLog.h"
#include "zFile.h"
#include "zRootStateInfo.h"

/**
 * 获取Root文件信息
 * 检测系统中常见的Root相关文件，如su、mu等
 * 这些文件的存在通常表明设备已被Root
 * @return 包含检测结果的Map，格式：{文件路径 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_root_state_info(){
    LOGD("get_root_file_info called");
    map<string, map<string, string>> info;

    // 定义需要检测的Root相关文件路径列表
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

    // 遍历检测每个路径
    for (const char* path : paths) {
        LOGI("Checking path: %s", path);
        zFile file(path);
        
        // 检查文件是否存在
        if(file.exists()){
            LOGI("Black file exists: %s", path);
            // 标记为错误级别，说明检测到Root相关文件
            info[path]["risk"] = "error";
            info[path]["explain"] = "black file but exist";
        }
    }

    return info;
}
