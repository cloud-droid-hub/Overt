//
// Created by lxz on 2025/6/12.
//

#include <sys/sysinfo.h>
#include "zLibc.h"
#include "zLog.h"
#include "zFile.h"
#include "zHttps.h"
#include "zJson.h"

#include "zSensorInfo.h"
#include "zSensorManager.h"

map<string, map<string, string>> get_sensor_info() {
    LOGI("get_sensor_info: starting...");

    map<string, map<string, string>> info;

    zSensorManager* manager = zSensorManager::getInstance();

    if (!manager) {
        LOGW("Failed to get sensor manager instance");
        info["sensor_score"]["value"] = "-1";
        info["sensor_bits"]["value"] = "0";
        return info;
    }

    // 采集端：只上报风险评分与风险位(原始数据)，阈值/位判定由 zengine 负责
    int score = manager->getRiskScore();
    uint32_t riskBits = manager->getRiskBits();
    LOGI("sensor risk score: %d, riskBits: 0x%x", score, riskBits);

    info["sensor_score"]["value"] = to_string(score);
    info["sensor_bits"]["value"] = to_string(riskBits);

    return info;
}
