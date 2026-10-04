//
// Created by lxz on 2025/6/12.
//



#include "zLog.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zLibc.h"
#include "zLibcUtil.h"

#include "zLinker.h"
#include "zLinkerInfo.h"

/**
 * 获取动态链接器信息(查杀分离 — 采集端)
 * 遍历所有已加载共享库路径并上报CRC校验结果(全量原始数据)，不做风险判定；
 * 黑名单库名(lsposed/frida)与CRC失败判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：
 *   {库路径 -> {value: "lsposed"/"frida"/"so"}} 与 {库名 -> {value: "crc:N"}}
 */
map<string, map<string, string>> get_linker_info(){
    map<string, map<string, string>> info;

    zLinker* linker = zLinker::getInstance();
    if (linker == nullptr) {
        LOGE("get_linker_info: zLinker instance is null");
        return info; // 初始化失败，空原始数据(风险判定交 zengine)
    }

    // 获取所有已加载共享库的路径列表，全量上报(不内置过滤)
    vector<string> libpath_list = linker->get_libpath_list();
    for (int i = 0; i < libpath_list.size(); ++i) {
        LOGD("libpath %s", libpath_list[i].c_str());
        info[libpath_list[i]]["value"] = "so";
    }
    LOGI("linker_info libs raw count=%zu", info.size());

    // 采集范围：关键系统库(仅WHERE to look，不是风险判定)
    vector<string> so_list{
            "libc.so",      // C标准库，系统核心组件
            "libart.so",    // Android运行时库，系统核心组件
            "libinput.so",  // 输入系统库，系统核心组件
    };

    // 遍历关键系统库，上报CRC校验和(原始值，判定交 zengine)
    for(int i = 0; i < so_list.size(); ++i) {
        string so_path = so_list[i];
        size_t slash_pos = so_path.rfind('/');
        string so_name = (slash_pos == string::npos) ? so_path : so_path.substr(slash_pos + 1);

        if(string_end_with(so_name.c_str(), ".so")){
            int ret = zLinker::check_lib_crc(so_name.c_str());
            LOGD("check_lib_crc %s %d", so_name.c_str(), ret);
            info[so_name]["value"] = string_format("crc:%d", ret);
        }
    }

    return info;
}
