#ifndef ZSENSORMANAGER_H
#define ZSENSORMANAGER_H

#include "zStd.h"

#include <android/sensor.h>
#include <mutex>

// 风控风险位掩码
enum SensorRiskBits {
    SENSOR_FIFO_EMPTY      = 1 << 0,        // FIFO 全为 0
    SENSOR_WAKEUP_TOO_FEW  = 1 << 1,        // wake-up sensors too few
    SENSOR_DELAY_UNIFORM   = 1 << 3,        // minDelay/maxDelay 统一
    SENSOR_COUNT_LOW       = 1 << 5,        // 传感器数量过少
};

// 传感器信息类 - 代表单个传感器
class zSensor {
public:
    zSensor(const ASensor* sensor);
    ~zSensor();
    
    // 获取传感器信息
    const char* getName() const { return name; }
    const char* getVendor() const { return vendor; }
    int getType() const { return type; }
    float getResolution() const { return resolution; }
    int32_t getMinDelay() const { return minDelay; }
    int32_t getMaxDelay() const { return maxDelay; }
    int32_t getFifoMaxEventCount() const { return fifoMaxEventCount; }
    int32_t getFifoReservedEventCount() const { return fifoReservedEventCount; }
    bool isWakeUpSensor() const { return isWakeUpSensor_; }
    
    // 更新信息（如果需要重新获取）
    void update();
    
    // 打印传感器信息
    void print() const;
    
    // 获取传感器类型名称
    const char* getTypeName() const;
    
private:
    const ASensor* sensor_;
    const char* name;
    const char* vendor;
    int type;
    float resolution;
    int32_t minDelay;
    int32_t maxDelay;
    int32_t fifoMaxEventCount;
    int32_t fifoReservedEventCount;
    bool isWakeUpSensor_;
};

// 传感器管理器类 - 单例模式，在构造时完成所有信息收集和检测
class zSensorManager {
public:
    // 获取单例实例
    static zSensorManager* getInstance();
    
    // 打印所有传感器信息
    void printAllSensors() const;
    
    // 打印检测结果
    void printDetectionResults() const;
    
    // 获取传感器列表
    const vector<zSensor*>& getSensors() const { return sensors; }
    
    // 获取检测结果
    uint32_t getRiskBits() const { return riskBits; }
    int getRiskScore() const { return riskScore; }
    
    // 禁止拷贝和赋值
    zSensorManager(const zSensorManager&) = delete;
    zSensorManager& operator=(const zSensorManager&) = delete;
    
private:
    // 私有构造函数
    zSensorManager();
    ~zSensorManager();
    
    // 静态单例实例
    static zSensorManager* instance;
    
    ASensorManager* manager;
    vector<zSensor*> sensors;
    uint32_t riskBits;
    int riskScore;
    
    // 内部方法：在构造时调用
    void enumerateSensors();
    void performDetection();
    
    // 检测方法
    uint32_t detectStructureAnomalies();
    bool detectBatchFake();
    bool detectTimestampAnomalies();
    
    // 计算风险评分
    int calculateRiskScore(uint32_t bits);
};

#endif // ZSENSORMANAGER_H

