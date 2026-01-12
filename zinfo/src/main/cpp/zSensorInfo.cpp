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
        info["sensor_info"]["risk"] = "error";
        info["sensor_info"]["explain"] = "Failed to get sensor manager instance";
        return info;
    }

    int score = manager->getRiskScore();
    LOGI("sensor risk score: %d", score);

    if(score > 0){
        string level = score > 60 ? "error" : "warn";
        uint32_t riskBits = manager->getRiskBits();
        if (riskBits & SENSOR_FIFO_EMPTY) {
            info["fifo"]["risk"] = level;
            info["fifo"]["explain"] = "sensor fifo is empty";
        }
        if (riskBits & SENSOR_WAKEUP_TOO_FEW) {
            info["wakeup_sensor"]["risk"] = level;
            info["wakeup_sensor"]["explain"] = "wakeup sensors too few";
        }
        if (riskBits & SENSOR_DELAY_UNIFORM) {
            info["delay"]["risk"] = level;
            info["delay"]["explain"] = "sensor uniform delays";
        }
        if (riskBits & SENSOR_COUNT_LOW) {
            info["count"]["risk"] = level;
            info["count"]["explain"] = "sensor count is too low";
        }
    }

    return info;
}
