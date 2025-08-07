//
// Created by lxz on 2025/6/6.
//


#include "zLog.h"
#include "zFile.h"
#include "zRootFileInfo.h"

/**
 * 获取Root文件信息
 * 检测系统中常见的Root相关文件，如su、mu等
 * 这些文件的存在通常表明设备已被Root
 * @return 包含检测结果的Map，格式：{文件路径 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_root_file_info(){
    LOGD("get_root_file_info called");
    map<string, map<string, string>> info;
    
    // 定义需要检测的Root相关文件路径列表
    const char* paths[] = {
            "/sbin/su",                    // 常见的su二进制文件位置
            "/system/bin/su",              // 系统bin目录下的su
            "/system/xbin/su",             // 系统xbin目录下的su
            "/data/local/xbin/su",         // 本地xbin目录下的su
            "/data/local/bin/su",          // 本地bin目录下的su
            "/system/sd/xbin/su",          // SD卡上的su
            "/system/bin/failsafe/su",     // 安全模式下的su
            "/data/local/su",              // 本地目录下的su
            "/system/xbin/mu",             // mu二进制文件（另一种Root工具）
            "/system_ext/bin/su",          // 系统扩展目录下的su
            "/apex/com.android.runtime/bin/suu"  // APEX运行时目录下的suu
    };

    // 遍历检测每个路径
    for (const char* path : paths) {
        LOGD("Checking path: %s", path);
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
