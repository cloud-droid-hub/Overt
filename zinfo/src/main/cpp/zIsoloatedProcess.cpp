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

    std::string response = zBinder::getInstance()->sendMessage("hello");
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
            
            // 遍历 JSON 对象的每个键值对
            int processCount = 0;
            for (auto it = json.begin(); it != json.end(); ++it) {

                    string key = it.key();
                    zJson value = it.value();
                    
                    LOGI("get_isoloated_process_info: processing key: %s", key.c_str());
                    
                    if (value.is_object()) {
                        map<string, string> processInfo;
                        
                        // 获取 explain 和 risk 字段
                        string explain = value.value("explain", "");
                        string risk = value.value("risk", "");
                        
                        if (!explain.empty() || !risk.empty()) {
                            if (!explain.empty()) {
                                processInfo["explain"] = explain;
                            }
                            if (!risk.empty()) {
                                processInfo["risk"] = risk;
                            }
                        
                            // 将进程信息添加到结果中
                            info[key] = processInfo;
                            processCount++;
                            LOGI("get_isoloated_process_info: process[%d] key=%s, explain=%s, risk=%s", 
                                 processCount, key.c_str(), 
                                 explain.c_str(), 
                                 risk.c_str());
                        } else {
                            LOGW("get_isoloated_process_info: both explain and risk are empty for key: %s", key.c_str());
                        }
                    } else {
                        LOGW("get_isoloated_process_info: value is not object for key: %s", key.c_str());
                    }

            }
            LOGI("get_isoloated_process_info: parsed %d processes", processCount);
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