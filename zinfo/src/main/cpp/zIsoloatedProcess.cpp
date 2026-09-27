//
// Created by lxz on 2026/1/8.
//

#include "zLog.h"
#include "zIsoloatedProcess.h"
#include "zProcInfo.h"
#include "zJson.h"
#include "zBinder.h"

/**
 * 获取隔离进程信息(查杀分离 — 纯传输管道)
 * 通过 Binder 共享内存通道向隔离进程请求 proc_info 原始数据(隔离进程侧也是全量原始输出)，
 * 不做风险判定；主进程收到后与 proc_info 类别一样交给 zengine 分析引擎。
 * @return 包含原始数据的Map，格式：{检查项 -> {value: 原始值}}
 */
map<string, map<string, string>> get_isoloated_process_info(){
    map<string, map<string, string>> info;

    std::string response = zBinder::getInstance()->sendMessage("get_isoloated_process_info");
    if (!response.empty()) {
        LOGI("Received response: %s", response.c_str());
    } else {
        LOGE("Failed to get response for message");
    }

    LOGI("get_isoloated_process_info: response: %s", response.c_str());

    if(!response.empty()){
        LOGI("get_isoloated_process_info: start parsing JSON");
        try {
            zJson json = zJson::parse(response.c_str());
            LOGI("get_isoloated_process_info: JSON parsed successfully");
            
            // 使用 get 方法直接转换为 map<string, map<string, string>>
            if (json.is_object()) {
                info = json.get<map<string, map<string, string>>>();
                LOGI("get_isoloated_process_info: parsed processes");
            } else {
                LOGW("get_isoloated_process_info: JSON is not an object");
            }
        } catch (zJson::parse_error &e) {
            LOGE("get_isoloated_process_info: zJson::parse_error: %s", e.what());
        } catch (...) {
            LOGE("get_isoloated_process_info: unknown exception");
        }
    } else {
        LOGW("get_isoloated_process_info: response is empty");
    }
    
    LOGI("get_isoloated_process_info: return %d processes", (int)info.size());

    return info;
}
