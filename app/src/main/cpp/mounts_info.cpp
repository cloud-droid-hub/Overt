//
// Created by lxz on 2025/6/6.
//

#include "mounts_info.h"
#include "zUtil.h"
#include "zLog.h"
#include "zFile.h"

/**
 * 获取挂载点信息
 * 检测系统中异常的挂载点，如dex2oat、APatch、shamiko等
 * 这些挂载点通常与Root工具或系统修改相关
 * @return 包含检测结果的Map，格式：{挂载信息 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_mounts_info(){
    LOGD("get_mounts_info called");
    map<string, map<string, string>> info;

    // 定义需要检测的异常挂载点名称
    const char* paths[] = {
            "dex2oat",    // Hunter认为dex2oat存在是不合理的
            "APatch",     // APatch框架相关
            "shamiko",    // Shamiko模块相关
    };

    // 读取/proc/self/mounts文件，获取当前进程的挂载点信息
    vector<string> mounts_lines = zFile("/proc/self/mounts").readAllLines();
    LOGI("Read %zu lines from /proc/self/mounts", mounts_lines.size());
    
    // 遍历每一行挂载信息
    for(int i = 0; i < mounts_lines.size(); i++){
        LOGD("Processing line %d: %s", i, mounts_lines[i].c_str());
        
        // 检查是否包含异常挂载点名称
        for (const char* path : paths) {
            if (strstr(mounts_lines[i].c_str(), path) != nullptr){
                LOGE("check_mounts error %d %s", i, mounts_lines[i].c_str());
                info[mounts_lines[i].c_str()]["risk"] = "error";
                info[mounts_lines[i].c_str()]["explain"] = "black name but in system path";
            }
        }
        
        // 检查系统目录是否被overlay挂载（这通常表示系统被修改）
        if(strstr(mounts_lines[i].c_str(), "/system ") != nullptr && 
           strstr(mounts_lines[i].c_str(), "overlay") != nullptr){
            LOGE("check_mounts error %d %s", i, mounts_lines[i].c_str());
            info[mounts_lines[i].c_str()]["risk"] = "error";
            info[mounts_lines[i].c_str()]["explain"] = "black name but in system path";
        }
    }
    return info;
}
