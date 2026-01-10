//
// Created by lxz on 2026/1/8.
//

#include "zLog.h"
#include "zIsoloatedProcess.h"
#include "zProcInfo.h"
#include "zJson.h"
#include "zBinder.h"

/**
 * 获取本地网络信息
 * 通过UDP广播机制检测同一网络中的其他Overt设备
 * 使用端口7476进行广播通信
 * @return 包含检测结果的Map，格式：{IP地址 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_isoloated_process_info(){
    map<string, map<string, string>> info;

    std::string response = zBinder::getInstance()->sendMessage("get_isoloated_process_info");
    if (!response.empty()) {
        LOGI("Received response23: %s", response.c_str());
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