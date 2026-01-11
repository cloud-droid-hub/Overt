//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZMANAGER_H
#define OVERT_ZMANAGER_H

#include <shared_mutex>
#include <jni.h>
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zStdUtil.h"

/**
 * 设备信息管理器类 - Overt安全检测工具的核心管理器
 * 
 * 功能说明：
 * 1. 采用单例模式，确保全局唯一实例
 * 2. 负责管理和存储各种设备安全检测信息
 * 3. 使用三层嵌套的Map结构存储信息：类别 -> 项目 -> 属性 -> 值
 * 4. 提供线程安全的数据访问和更新机制
 * 5. 调度和管理各种安全检测任务
 * 6. 处理Native层与Java层的数据交互
 * 
 * 设计特点：
 * - 单例模式：确保全局唯一，避免资源冲突
 * - 线程安全：使用读写锁保护数据访问
 * - 模块化：支持多种检测模块的集成
 * - 异步处理：支持后台任务执行
 * - 内存优化：智能管理检测数据生命周期
 * 
 * 数据存储结构：
 * device_info[检测类别][检测项目][属性名] = 属性值
 * 例如：device_info["root_state_info"]["su文件检测"]["risk"] = "error"
 * 
 * 线程安全机制：
 * - 使用std::shared_mutex实现读写锁
 * - 读操作使用共享锁，允许多线程并发读取
 * - 写操作使用独占锁，确保数据一致性
 * 
 * 生命周期管理：
 * - 在Native库加载时自动创建
 * - 在应用程序退出时自动清理
 * - 支持手动清理和重置
 */
class zManager {
private:
    // ==================== 单例模式相关 ====================
    
    /**
     * 私有构造函数，防止外部实例化
     * 确保只能通过getInstance()方法获取实例
     */
    zManager();

    /**
     * 禁用拷贝构造函数
     * 防止通过拷贝创建多个实例
     */
    zManager(const zManager&) = delete;

    /**
     * 禁用赋值操作符
     * 防止通过赋值创建多个实例
     */
    zManager& operator=(const zManager&) = delete;

    // ==================== 单例实例管理 ====================
    
    /**
     * 单例实例指针
     * 使用静态成员变量存储唯一实例
     * 采用懒加载模式，首次调用时创建
     */
    static zManager* instance;

    // ==================== 任务调度配置 ====================
    
    /**
     * 任务执行间隔时间（秒）
     * 控制各个检测任务的执行频率
     * 默认10秒，可根据需要调整
     */
    int time_interval = 10;
    
    // ==================== 任务状态管理 ====================
    
    /**
     * 任务执行状态结构体
     * 用于跟踪每个检测任务的执行状态和统计信息
     */
    struct TaskStatus {
        time_t last_execution = 0;  // 最后执行时间戳
        bool is_running = false;    // 当前是否正在运行
        int execution_count = 0;    // 累计执行次数
        time_t last_success = 0;    // 最后成功执行时间戳
    };
    
    /**
     * 任务状态映射表
     * 键：任务名称（如"root_state_info"）
     * 值：任务状态信息
     */
    map<string, TaskStatus> task_status_map;
    
    /**
     * 任务状态互斥锁
     * 保护任务状态映射表的并发访问
     */
    mutable std::mutex task_status_mutex;

    // ==================== 设备信息存储 ====================
    
    /**
     * 设备信息存储结构（三层嵌套Map）
     * 第一层：检测类别（如"root_state_info"、"proc_info"）
     * 第二层：检测项目（如"su文件检测"、"进程名检测"）
     * 第三层：属性值对（如"risk"->"error", "explain"->"检测到Root"）
     * 
     * 示例结构：
     * device_info["root_state_info"]["su文件检测"]["risk"] = "error"
     * device_info["root_state_info"]["su文件检测"]["explain"] = "检测到su文件"
     */
    static map<string, map<string, map<string, string>>> device_info;

    /**
     * 设备信息读写锁
     * 使用shared_mutex实现读写锁机制：
     * - 读操作使用共享锁，允许多线程并发读取
     * - 写操作使用独占锁，确保数据一致性
     */
    mutable std::shared_mutex device_info_mtx_;

public:
    // ==================== 单例模式接口 ====================
    
    /**
     * 获取单例实例
     * 
     * 功能说明：
     * 1. 采用线程安全的懒加载模式
     * 2. 首次调用时创建实例，后续调用返回同一实例
     * 3. 使用std::call_once确保线程安全
     * 
     * 线程安全：
     * - 使用std::call_once保证只创建一次
     * - 支持多线程并发调用
     * 
     * @return zManager单例指针，永远不会为null
     */
    static zManager* getInstance();

    /**
     * 析构函数
     * 
     * 功能说明：
     * 1. 清理实例资源
     * 2. 记录销毁日志
     * 3. 执行必要的清理工作
     */
    ~zManager();
    
    /**
     * 清理单例实例
     * 
     * 功能说明：
     * 1. 手动清理单例实例
     * 2. 主要用于测试或程序退出时
     * 3. 防止内存泄漏
     * 
     * 使用场景：
     * - 单元测试中重置状态
     * - 应用程序退出时清理资源
     * - 内存泄漏检测
     */
    static void cleanup();

    // ==================== 设备信息管理接口 ====================
    
    /**
     * 获取所有设备信息
     * 
     * 功能说明：
     * 1. 返回完整的设备信息存储结构
     * 2. 使用读锁保护，支持并发读取
     * 3. 返回引用，避免数据拷贝
     * 
     * 线程安全：
     * - 使用共享读锁，允许多线程并发读取
     * - 不会阻塞其他读操作
     * 
     * @return 三层嵌套Map的常量引用，包含所有检测结果
     */
    const map<string, map<string, map<string, string>>>& get_device_info() const;

    /**
     * 更新设备信息
     * 
     * 功能说明：
     * 1. 更新指定类别的设备信息
     * 2. 使用写锁保护，确保数据一致性
     * 3. 支持增量更新和全量替换
     * 
     * 线程安全：
     * - 使用独占写锁，确保更新原子性
     * - 会阻塞其他读写操作
     * 
     * @param key 信息类别标识（如"root_state_info"、"proc_info"）
     * @param value 该类别下的具体信息，二层嵌套Map
     */
    void update_device_info(const string& key, const map<string, map<string, string>>& value);

    /**
     * 获取指定类别的设备信息
     * 
     * 功能说明：
     * 1. 根据类别键获取对应的设备信息
     * 2. 使用写锁保护，确保数据一致性
     * 3. 返回数据副本，避免外部修改
     * 
     * @param key 信息类别标识
     * @return 指定类别的设备信息副本
     */
    const map<string, map<string, string>> get_info(const string& key);

    /**
     * 清空所有设备信息
     * 
     * 功能说明：
     * 1. 清空所有存储的设备信息
     * 2. 释放相关内存资源
     * 3. 通常在信息返回给Java层后调用
     * 
     * 使用场景：
     * - 避免重复返回相同信息
     * - 内存优化和清理
     * - 重置检测状态
     * 
     * 线程安全：
     * - 使用独占写锁，确保清空操作原子性
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

    void update_test_info();
    void notice_java(string title);
    void round_tasks();

private:
    /**
     * 统一的更新和通知方法
     * 封装所有信息收集、存储和通知的通用逻辑
     * 
     * 功能说明：
     * 1. 调用信息收集函数获取数据
     * 2. 更新设备信息存储
     * 3. 通知Java层更新UI
     * 4. 统一的异常处理
     * 
     * @param key 信息类别标识（如"proc_info"、"root_state_info"等）
     * @param get_info_func 获取信息的函数指针
     */
    void update_info(const string& key, map<string, map<string, string>> (*get_info_func)());

};

#endif //OVERT_ZMANAGER_H
