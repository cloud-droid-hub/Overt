//
// Created by lxz on 2025/7/10.
//

#include "zDevice.h"

// 静态成员变量初始化
zDevice* zDevice::instance = nullptr;

// 设备信息存储Map初始化
map<string, map<string, map<string, string>>> zDevice::device_info;

/**
 * 构造函数实现
 * 初始化设备信息管理器
 */
zDevice::zDevice() {
    LOGD("Constructor called");
    // 初始化代码可以在这里添加
}

/**
 * 析构函数实现
 * 清理资源
 */
zDevice::~zDevice() {
    LOGD("Destructor called");
    // 清理代码可以在这里添加
}

/**
 * 获取所有设备信息
 * 使用读锁保证线程安全，允许多个线程同时读取
 * @return 三层嵌套的Map，包含所有收集到的设备信息
 */
const map<string, map<string, map<string, string>>>& zDevice::get_device_info() const{
    LOGD("get_device_info called");
    // 使用共享读锁，允许多个线程同时读取
    std::shared_lock<std::shared_mutex> lock(device_info_mtx_);
    LOGI("get_device_info: device_info size=%zu", device_info.size());
    return device_info;
};

/**
 * 更新设备信息
 * 使用写锁保证线程安全，确保数据一致性
 * @param key 信息类别（如"task_info", "maps_info"等）
 * @param value 该类别下的具体信息
 */
void zDevice::update_device_info(const string& key, const map<string, map<string, string>>& value){
    LOGD("update_device_info called, key=%s", key.c_str());
    // 使用独占写锁，确保更新操作的原子性
    std::unique_lock<std::shared_mutex> lock(device_info_mtx_);
    device_info[key] = value;
    LOGI("update_device_info: updated key=%s", key.c_str());
};

/**
 * 清空所有设备信息
 * 使用写锁保证线程安全
 * 通常在信息返回给Java层后调用，避免重复返回
 */
void zDevice::clear_device_info(){
    LOGD("clear_device_info called");
    // 使用独占写锁，确保清空操作的原子性
    std::unique_lock<std::shared_mutex> lock(device_info_mtx_);
    device_info.clear();
    LOGI("clear_device_info");
};
