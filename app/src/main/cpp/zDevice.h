//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZDEVICE_H
#define OVERT_ZDEVICE_H

#include <shared_mutex>
#include "config.h"
#include "zLog.h"

/**
 * 设备信息管理器类
 * 采用单例模式，负责管理和存储各种设备信息
 * 使用三层嵌套的Map结构存储信息：类别 -> 项目 -> 属性 -> 值
 * 线程安全，支持多线程并发访问
 */
class zDevice {
private:
    // 私有构造函数，防止外部实例化
    zDevice();

    // 禁用拷贝构造函数
    zDevice(const zDevice&) = delete;

    // 禁用赋值操作符
    zDevice& operator=(const zDevice&) = delete;

    // 单例实例指针
    static zDevice* instance;

    // 设备信息存储结构：类别 -> 项目 -> 属性 -> 值
    // 例如：{"task_info" -> {"进程名" -> {"risk" -> "error", "explain" -> "检测到Frida"}}}
    static map<string, map<string, map<string, string>>> device_info;

    // 读写锁，保证线程安全
    mutable std::shared_mutex device_info_mtx_;

public:
    /**
     * 获取单例实例
     * @return zDevice单例指针
     */
    static zDevice* getInstance() {
        if (instance == nullptr) {
            instance = new zDevice();
        }
        return instance;
    }

    // 析构函数
    ~zDevice();

    /**
     * 获取所有设备信息
     * @return 三层嵌套的Map，包含所有收集到的设备信息
     */
    const map<string, map<string, map<string, string>>>& get_device_info() const;

    /**
     * 更新设备信息
     * @param key 信息类别（如"task_info", "maps_info"等）
     * @param value 该类别下的具体信息
     */
    void update_device_info(const string& key, const map<string, map<string, string>>& value);

    /**
     * 清空所有设备信息
     * 通常在信息返回给Java层后调用，避免重复返回
     */
    void clear_device_info();
};

#endif //OVERT_ZDEVICE_H
