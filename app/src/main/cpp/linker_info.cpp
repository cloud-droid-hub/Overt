//
// Created by lxz on 2025/6/12.
//



#include "zLog.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zLibc.h"
#include "zLibcUtil.h"

#include "zLinker.h"
#include "linker_info.h"

/**
 * 获取动态链接器信息
 * 分析系统中所有已加载的共享库，检测可疑的库文件和库文件完整性
 * 主要用于检测LSPosed、Frida等框架注入的共享库，以及系统关键库的完整性
 * @return 包含共享库风险信息的映射表
 */
map<string, map<string, string>> get_linker_info(){
    map<string, map<string, string>> info;
    
    // 获取所有已加载共享库的路径列表
    vector<string> libpath_list = zLinker::getInstance()->get_libpath_list();
    
    // 遍历所有共享库路径，检测黑名单库
    for (int i = 0; i < libpath_list.size(); ++i) {
        LOGD("libpath %s", libpath_list[i].c_str());
        
        // 检测LSPosed相关的共享库
        if(strstr(libpath_list[i].c_str(), "lsposed")){
            LOGW("Found blacklisted library: %s", libpath_list[i].c_str());
            info[libpath_list[i]]["risk"] = "error";
            info[libpath_list[i]]["explain"] = "black soname";
        }
        
        // 检测Frida相关的共享库
        if(strstr(libpath_list[i].c_str(), "frida")){
            LOGW("Found blacklisted library: %s", libpath_list[i].c_str());
            info[libpath_list[i]]["risk"] = "error";
            info[libpath_list[i]]["explain"] = "black soname";
        }
    }

    // 定义需要检查CRC校验和的关键系统库列表
    vector<string> so_list{
            "libc.so",      // C标准库，系统核心组件
            "libart.so",    // Android运行时库，系统核心组件
            "libinput.so",  // 输入系统库，系统核心组件
    };

    // 遍历关键系统库，检查其CRC校验和
    for(int i = 0; i < so_list.size(); ++i) {
        string so_path = so_list[i];
        
        // 提取库文件名（去掉路径部分）
        string so_name = so_path.substr(so_path.rfind('/') + 1);
        
        // 只检查.so文件
        if(string_end_with(so_name.c_str(), ".so")){
            // 调用CRC校验和检查函数
            int ret = zLinker::check_lib_crc(so_name.c_str());
            LOGD("check_lib_crc %s %d", so_name.c_str(), ret);
            
            // 如果CRC校验失败，标记为风险
            if (ret!= 0){
                LOGW("CRC check failed for library: %s", so_name.c_str());
                info[so_name]["risk"] = "error";
                info[so_name]["explain"] = "check_lib_crc error";
            }
        }
    }

    return info;
}

