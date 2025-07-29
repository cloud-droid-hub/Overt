//
// Created by lxz on 2025/7/10.
//

#include "zDevice.h"

zDevice* zDevice::instance = nullptr;

map<string, map<string, map<string, string>>> zDevice::device_info;

// 构造函数实现
zDevice::zDevice() {
    LOGE("zDevice: constructor called");
    // 初始化代码可以在这里添加
}

// 析构函数实现
zDevice::~zDevice() {
    // 清理代码可以在这里添加
}

const map<string, map<string, map<string, string>>>& zDevice::get_device_info() const{
  std::shared_lock<std::shared_mutex> lock(device_info_mtx_);
  LOGE("zDevice::get_device_info: called, device_info size=%zu", device_info.size());
  return device_info;
};

void zDevice::update_device_info(const string& key, const map<string, map<string, string>>& value){
  LOGE("zDevice::update_device_info: called, key=%s", key.c_str());
  std::unique_lock<std::shared_mutex> lock(device_info_mtx_);
  device_info[key] = value;
};
