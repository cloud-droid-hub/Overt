//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZMANAGER_H
#define OVERT_ZMANAGER_H

#include <shared_mutex>
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zStdUtil.h"

/**
 * 设备信息管理器类
 * 采用单例模式，负责管理和存储各种设备信息
 * 使用三层嵌套的Map结构存储信息：类别 -> 项目 -> 属性 -> 值
 * 线程安全，支持多线程并发访问
 */
class zManager {
private:
    // 私有构造函数，防止外部实例化
    zManager();

    // 禁用拷贝构造函数
    zManager(const zManager&) = delete;

    // 禁用赋值操作符
    zManager& operator=(const zManager&) = delete;

    // 单例实例指针
    static zManager* instance;

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
    static zManager* getInstance() {
        if (instance == nullptr) {
            instance = new zManager();
        }
        return instance;
    }

    // 析构函数
    ~zManager();

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

    /**
    * 获取大核心CPU列表
        * 通过读取CPU频率信息识别大核心
    * @return 大核心CPU ID列表
    */
    vector<int> get_big_core_list();


    pid_t gettid();

    void bind_self_to_least_used_big_core();

    void raise_thread_priority(int sched_priority = 0);

    void update_ssl_info();
    void update_local_network_info();
    void update_task_info();
    void update_maps_info();
    void update_root_file_info();
    void update_mounts_info();
    void update_system_prop_info();
    void update_linker_info();
    void update_port_info();
    void update_class_loader_info();
    void update_package_info();
    void update_system_setting_info();
    void update_tee_info();
    void update_time_info();
    void update_logcat_info();

};

#endif //OVERT_ZMANAGER_H
