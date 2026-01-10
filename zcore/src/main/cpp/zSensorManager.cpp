#include "zSensorManager.h"
#include "zLog.h"

#include <android/sensor.h>
#include <mutex>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <numeric>

// ============================================================================
// zSensor 类实现 - 构造时自动收集所有信息
// ============================================================================

zSensor::zSensor(const ASensor* sensor) : sensor_(sensor) {
    update();
}

zSensor::~zSensor() {
    // sensor_ 是指向系统管理的传感器，不需要释放
}

void zSensor::update() {
    if (!sensor_) return;

    // 传感器名称（由 Sensor HAL 提供，通常包含芯片型号或功能描述）
    name = ASensor_getName(sensor_);

    // 传感器厂商标识（HAL 填充）
    // 真实手机多为具体厂商（如 BOSCH / QTI / AKM 等），
    // 虚拟设备或开发环境中常见为 "Vendor String" / "AOSP"
    vendor = ASensor_getVendor(sensor_);

    // 传感器类型（Android 标准类型或厂商自定义类型）
    // 用于区分物理传感器与融合 / 虚拟传感器
    type = ASensor_getType(sensor_);

    // 传感器分辨率（最小可区分变化量）
    // 反映传感器硬件精度或 HAL 声明精度；
    // 在真实设备中通常为非整值，在模拟实现中常为规则或固定值
    resolution = ASensor_getResolution(sensor_);

    // 传感器支持的最小采样周期（单位：微秒）
    // 表示硬件或 HAL 所允许的最高采样频率上限；
    // 与 FIFO / batching 能力强相关
    minDelay = ASensor_getMinDelay(sensor_);

    // 该传感器在 FIFO 中最多可以缓存的事件数量
    fifoMaxEventCount = ASensor_getFifoMaxEventCount(sensor_);

    // 系统为该传感器"保证"预留的 FIFO 事件数量，该真实缓存的事件数量一定在 range(fifoReservedEventCount, fifoMaxEventCount)
    fifoReservedEventCount = ASensor_getFifoReservedEventCount(sensor_);

    // 具备「低功耗 + 可唤醒系统」能力的传感器
    // 在 SoC 进入 suspend / deep sleep 状态时，该传感器产生的事件可以唤醒CPU，并把事件交付给 SensorService。
    isWakeUpSensor_ = ASensor_isWakeUpSensor(sensor_) != 0;

}

const char* zSensor::getTypeName() const {
    // 只使用确定存在的标准传感器类型常量
    switch (type) {
        case ASENSOR_TYPE_ACCELEROMETER: return "ACCELEROMETER";
        case ASENSOR_TYPE_MAGNETIC_FIELD: return "MAGNETIC_FIELD";
        case ASENSOR_TYPE_GYROSCOPE: return "GYROSCOPE";
        case ASENSOR_TYPE_LIGHT: return "LIGHT";
        case ASENSOR_TYPE_PROXIMITY: return "PROXIMITY";
        case ASENSOR_TYPE_PRESSURE: return "PRESSURE";
        case ASENSOR_TYPE_AMBIENT_TEMPERATURE: return "AMBIENT_TEMPERATURE";
        case ASENSOR_TYPE_STEP_COUNTER: return "STEP_COUNTER";
        case ASENSOR_TYPE_STEP_DETECTOR: return "STEP_DETECTOR";
        case ASENSOR_TYPE_SIGNIFICANT_MOTION: return "SIGNIFICANT_MOTION";
        case ASENSOR_TYPE_HEART_RATE: return "HEART_RATE";
        default: {
            // 对于未知类型，返回类型编号
            static char buffer[32];
            snprintf(buffer, sizeof(buffer), "TYPE_%d", type);
            return buffer;
        }
    }
}

void zSensor::print() const {
    LOGI("========================================");
    LOGI("Sensor: %s", name ? name : "NULL");
    LOGI("  Vendor: %s", vendor ? vendor : "NULL");
    LOGI("  Type: %s (%d)", getTypeName(), type);
    LOGI("  Resolution: %.6f", resolution);
    LOGI("  MinDelay: %d μs", minDelay);
    LOGI("  MaxDelay: %d μs (inferred)", maxDelay);
    LOGI("  FIFO Max: %d", fifoMaxEventCount);
    LOGI("  FIFO Reserved: %d", fifoReservedEventCount);
    LOGI("  Wake-up: %s", isWakeUpSensor_ ? "YES" : "NO");
    LOGI("========================================");
}

// ============================================================================
// zSensorManager 类实现 - 单例模式，构造时完成所有信息收集和检测
// ============================================================================

// 静态成员变量初始化
zSensorManager* zSensorManager::instance = nullptr;

// 获取单例实例
zSensorManager* zSensorManager::getInstance() {
    // 使用 std::call_once 确保线程安全的单例初始化
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        try {
            instance = new zSensorManager();
            LOGI("zSensorManager: Created singleton instance");
        } catch (const std::exception& e) {
            LOGE("zSensorManager: Failed to create singleton instance: %s", e.what());
        } catch (...) {
            LOGE("zSensorManager: Failed to create singleton instance with unknown error");
        }
    });

    return instance;
}

zSensorManager::zSensorManager() : manager(nullptr), riskBits(0), riskScore(0) {
    // 构造时自动初始化并收集所有信息
    manager = ASensorManager_getInstance();
    if (!manager) {
        LOGW("Failed to get ASensorManager");
        return;
    }
    
    // 枚举所有传感器（构造 zSensor 时会自动收集信息）
    enumerateSensors();
    
    // 执行检测
    performDetection();
}

zSensorManager::~zSensorManager() {
    for (auto* sensor : sensors) {
        delete sensor;
    }
    sensors.clear();
}

void zSensorManager::enumerateSensors() {
    if (!manager) {
        LOGW("Manager not available");
        return;
    }
    
    ASensorList sensorList;
    int count = ASensorManager_getSensorList(manager, &sensorList);
    
    LOGI("=== Enumerating Sensors ===");
    LOGI("Total sensors found: %d", count);
    
    for (int i = 0; i < count; i++) {
        // 构造 zSensor 时会自动调用 update() 收集所有信息
        zSensor* sensor = new zSensor(sensorList[i]);
        sensors.push_back(sensor);
    }
}

void zSensorManager::printAllSensors() const {
    LOGI("=== All Sensors Information ===");
    for (size_t i = 0; i < sensors.size(); i++) {
        LOGI("--- Sensor #%zu ---", i + 1);
        sensors[i]->print();
    }
    LOGI("=== Total: %zu sensors ===", sensors.size());
}

void zSensorManager::printDetectionResults() const {
    LOGI("=== Detection Results ===");
    LOGI("Risk Bits: 0x%08x", riskBits);
    LOGI("Risk Score: %d/100", riskScore);
    
    if (riskBits == 0) {
        LOGI("Risk Description: No risk detected");
    } else {
        LOGI("Risk Description:");
        if (riskBits & SENSOR_FIFO_EMPTY) LOGI("  - FIFO empty");
        if (riskBits & SENSOR_WAKEUP_TOO_FEW) LOGI("  - wake-up sensors too few");
        if (riskBits & SENSOR_DELAY_UNIFORM) LOGI("  - Uniform delays");
        if (riskBits & SENSOR_COUNT_LOW) LOGI("  - Low sensor count");
    }
}

uint32_t zSensorManager::detectStructureAnomalies() {
    uint32_t bits = 0;
    
    // 检测传感器数量
    if (sensors.size() < 20) {
        LOGW("L1: Sensor count too low: %zu", sensors.size());
        bits |= SENSOR_COUNT_LOW;
    }
    
    // 检测 FIFO 全为 0
    int fifoZeroCount = 0;
    for (const auto* sensor : sensors) {
        if (sensor->getFifoMaxEventCount() == 0 && sensor->getFifoReservedEventCount() == 0) {
            fifoZeroCount++;
        }
    }
    
    if (fifoZeroCount == (int)sensors.size()) {
        LOGW("L1: All sensors have FIFO = 0");
        bits |= SENSOR_FIFO_EMPTY;
    }
    
    // 检测 wake-up sensor
    int wakeUpCount = 0;
    for (const auto* sensor : sensors) {
        if (sensor->isWakeUpSensor()) {
            wakeUpCount++;
        }
    }
    
    if (wakeUpCount < 2) {
        LOGW("L1: wake-up sensors too few");
        bits |= SENSOR_WAKEUP_TOO_FEW;
    }
    
    LOGI("L1 Detection: wakeUpCount=%d, fifoZeroCount=%d", wakeUpCount, fifoZeroCount);
    
    return bits;
}

int zSensorManager::calculateRiskScore(uint32_t bits) {
    int score = 0;
    
    if (bits & SENSOR_FIFO_EMPTY) score += 30;
    if (bits & SENSOR_COUNT_LOW) score += 30;
    if (bits & SENSOR_WAKEUP_TOO_FEW) score += 20;
    if (bits & SENSOR_DELAY_UNIFORM) score += 20;

    if ((bits & SENSOR_WAKEUP_TOO_FEW) && (bits & SENSOR_FIFO_EMPTY)) {
        score += 20;
    }

    return score > 100 ? 100 : score;
}

void zSensorManager::performDetection() {
    LOGI("=== Starting Sensor Risk Detection ===");
    
    riskBits = 0;
    
    // L1 检测
    uint32_t l1Risks = detectStructureAnomalies();
    riskBits |= l1Risks;
    LOGI("risks: 0x%08x", l1Risks);
    
    // 计算评分
    riskScore = calculateRiskScore(riskBits);
    
    LOGI("=== Detection Complete ===");
}
