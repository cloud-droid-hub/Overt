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
        info["sensor_raw_count"]["value"] = "0";
        return info;
    }

    // 查杀分离—采集端：直接上报每个传感器的原始字段(不做任何聚合统计/判定)。
    // 字段顺序固定: name,type,minDelay,maxDelay,fifoMax,fifoReserved,isWakeUp
    // 统计(sensor 总数/fifo为0数/wakeup数)与评分判定全部由 zengine 负责。
    const vector<zSensor*>& sensors = manager->getSensors();
    for (size_t i = 0; i < sensors.size(); i++) {
        const zSensor* s = sensors[i];
        string raw = string_format("%s,%d,%d,%d,%d,%d,%d",
                                   s->getName() ? s->getName() : "",
                                   s->getType(),
                                   s->getMinDelay(),
                                   s->getMaxDelay(),
                                   s->getFifoMaxEventCount(),
                                   s->getFifoReservedEventCount(),
                                   s->isWakeUpSensor() ? 1 : 0);
        info["sensor:" + to_string(i)]["value"] = raw;
    }
    info["sensor_raw_count"]["value"] = to_string(sensors.size());
    LOGI("sensor raw count=%zu", sensors.size());

    return info;
}
