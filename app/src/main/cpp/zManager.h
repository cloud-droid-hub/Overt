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

    int time_interval = 10;  // 任务执行间隔（秒）
    
    // 任务执行状态管理
    struct TaskStatus {
        time_t last_execution = 0;  // 最后执行时间
        bool is_running = false;    // 是否正在运行
        int execution_count = 0;    // 执行次数
        time_t last_success = 0;    // 最后成功执行时间
    };
    
    map<string, TaskStatus> task_status_map;  // 任务状态映射
    mutable std::mutex task_status_mutex;     // 任务状态互斥锁

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
        // 双重检查锁定模式：先检查，避免不必要的锁开销
        if (instance == nullptr) {
            static std::mutex instance_mutex;
            std::lock_guard<std::mutex> lock(instance_mutex);
            
            // 再次检查，防止多线程竞争
            if (instance == nullptr) {
                try {
                    instance = new zManager();
                    LOGI("zManager: Created new singleton instance");
                } catch (const std::exception& e) {
                    LOGE("zManager: Failed to create singleton instance: %s", e.what());
                    return nullptr;
                } catch (...) {
                    LOGE("zManager: Failed to create singleton instance with unknown error");
                    return nullptr;
                }
            }
        }
        return instance;
    }

    // 析构函数
    ~zManager();
    
    // 清理单例实例（主要用于测试或程序退出时）
    static void cleanup() {
        // 使用与 getInstance() 相同的互斥锁
        static std::mutex instance_mutex;
        std::lock_guard<std::mutex> lock(instance_mutex);
        
        if (instance != nullptr) {
            try {
                delete instance;
                instance = nullptr;
                LOGI("zManager: Singleton instance cleaned up");
            } catch (const std::exception& e) {
                LOGE("zManager: Exception during cleanup: %s", e.what());
            } catch (...) {
                LOGE("zManager: Unknown exception during cleanup");
            }
        }
    }

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

    const map<string, map<string, string>> get_info(const string& key);

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
    void notice_java(string title);
    void round_tasks();
    
    // 将 map<string, string> 转换为 Java Map<String, String>
    jobject cmap_to_jmap(JNIEnv *env, const map<string, string>& cmap){
        LOGD("cmap_to_jmap called, map size: %zu", cmap.size());
        LOGI("cmap_to_jmap: starting conversion, map size=%zu", cmap.size());
        LOGD("cmap_to_jmap: map type info - is nonstd::map: %s", typeid(cmap).name());

        // 遍历并打印map内容
        LOGD("cmap_to_jmap: map contents:");
        for (const auto &entry : cmap) {
            LOGD("cmap_to_jmap: key='%s', value='%s'", entry.first.c_str(), entry.second.c_str());
        }

        // 查找类和方法ID（只查找一次）
        jclass hashMapClass = env->FindClass("java/util/HashMap");
        if (!hashMapClass) {
            LOGD("cmap_to_jmap: Failed to find HashMap class");
            return nullptr;
        }

        jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
        if (!hashMapConstructor) {
            LOGD("cmap_to_jmap: Failed to find HashMap constructor");
            return nullptr;
        }

        jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        if (!putMethod) {
            LOGD("cmap_to_jmap: Failed to find HashMap put method");
            return nullptr;
        }

        // 创建HashMap对象
        jobject jobjectMap = env->NewObject(hashMapClass, hashMapConstructor);
        if (!jobjectMap) {
            LOGD("cmap_to_jmap: Failed to create HashMap object");
            return nullptr;
        }

        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGD("cmap_to_jmap: Exception occurred during HashMap creation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            return nullptr;
        }

        // 填充数据
        for (const auto &entry : cmap) {
            LOGD("cmap_to_jmap: processing entry: key='%s', value='%s'",
                 entry.first.c_str(), entry.second.c_str());

            jstring jkey = env->NewStringUTF(entry.first.c_str());
            jstring jvalue = env->NewStringUTF(entry.second.c_str());

            if (!jkey || !jvalue) {
                LOGD("cmap_to_jmap: Failed to create string objects");
                if (jkey) env->DeleteLocalRef(jkey);
                if (jvalue) env->DeleteLocalRef(jvalue);
                continue;
            }

            env->CallObjectMethod(jobjectMap, putMethod, jkey, jvalue);

            // 检查是否抛出异常
            if (env->ExceptionCheck()) {
                LOGD("cmap_to_jmap: Exception occurred during put operation");
                env->ExceptionDescribe();
                env->ExceptionClear();
                env->DeleteLocalRef(jkey);
                env->DeleteLocalRef(jvalue);
                continue;
            }

            // 清理局部引用
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
        }

        LOGI("cmap_to_jmap: conversion completed successfully");
        return jobjectMap;
    }

// 将 map<string, map<string, string>> 转换为 Java Map<String, Map<String, String>>
    jobject cmap_to_jmap_nested(JNIEnv* env, const map<string, map<string, string>>& cmap) {
        LOGD("cmap_to_jmap_nested called, map size: %zu", cmap.size());
        LOGI("cmap_to_jmap_nested: starting conversion, map size=%zu", cmap.size());
        LOGD("cmap_to_jmap_nested: map type info - is nonstd::map: %s", typeid(cmap).name());

        // 遍历并打印map内容
        LOGD("cmap_to_jmap_nested: map contents:");
        for (const auto &entry : cmap) {
            LOGD("cmap_to_jmap_nested: key='%s', inner map size=%zu", entry.first.c_str(), entry.second.size());
            for (const auto &inner_entry : entry.second) {
                LOGD("cmap_to_jmap_nested: inner key='%s', value='%s'", inner_entry.first.c_str(), inner_entry.second.c_str());
            }
        }

        // 查找类和方法ID（只查找一次）
        jclass hashMapClass = env->FindClass("java/util/HashMap");
        if (!hashMapClass) {
            LOGE("cmap_to_jmap_nested: Failed to find HashMap class");
            return nullptr;
        }

        jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
        if (!hashMapConstructor) {
            LOGE("cmap_to_jmap_nested: Failed to find HashMap constructor");
            return nullptr;
        }

        jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        if (!putMethod) {
            LOGE("cmap_to_jmap_nested: Failed to find HashMap put method");
            return nullptr;
        }

        // 创建HashMap对象
        jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);
        if (!jmap) {
            LOGE("cmap_to_jmap_nested: Failed to create HashMap object");
            return nullptr;
        }

        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap_nested: Exception occurred during HashMap creation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            return nullptr;
        }

        // 填充数据
        for (const auto& entry : cmap) {
            LOGD("cmap_to_jmap_nested: processing entry: key='%s', inner map size=%zu",
                 entry.first.c_str(), entry.second.size());

            jstring jkey = env->NewStringUTF(entry.first.c_str());
            if (!jkey) {
                LOGE("cmap_to_jmap_nested: Failed to create key string");
                continue;
            }

            jobject jvalue = cmap_to_jmap(env, entry.second); // 调用之前定义的 cmap_to_jmap 方法
            if (!jvalue) {
                LOGE("cmap_to_jmap_nested: Failed to convert inner map");
                env->DeleteLocalRef(jkey);
                continue;
            }

            env->CallObjectMethod(jmap, putMethod, jkey, jvalue);

            // 检查是否抛出异常
            if (env->ExceptionCheck()) {
                LOGE("cmap_to_jmap_nested: Exception occurred during put operation");
                env->ExceptionDescribe();
                env->ExceptionClear();
                env->DeleteLocalRef(jkey);
                env->DeleteLocalRef(jvalue);
                continue;
            }

            // 清理局部引用
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
        }

        LOGI("cmap_to_jmap_nested: conversion completed successfully");
        return jmap;
    }

// 主函数：将 map<string, map<string, map<string, string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
    jobject cmap_to_jmap_nested_3(JNIEnv* env, const map<string, map<string, map<string, string>>>& cmap) {
        LOGD("cmap_to_jmap_nested_3 called, map size: %zu", cmap.size());
        LOGI("cmap_to_jmap_nested_3: starting conversion, map size=%zu", cmap.size());
        LOGD("cmap_to_jmap_nested_3: map type info - is nonstd::map: %s", typeid(cmap).name());

        // 遍历并打印map内容
        LOGD("cmap_to_jmap_nested_3: map contents:");
        for (const auto &entry : cmap) {
            LOGD("cmap_to_jmap_nested_3: key='%s', inner map size=%zu", entry.first.c_str(), entry.second.size());
            for (const auto &inner_entry : entry.second) {
                LOGD("cmap_to_jmap_nested_3: inner key='%s', inner inner map size=%zu", inner_entry.first.c_str(), inner_entry.second.size());
                for (const auto &inner_inner_entry : inner_entry.second) {
                    LOGD("cmap_to_jmap_nested_3: inner inner key='%s', value='%s'", inner_inner_entry.first.c_str(), inner_inner_entry.second.c_str());
                }
            }
        }

        // 查找类和方法ID（只查找一次）
        jclass hashMapClass = env->FindClass("java/util/HashMap");
        if (!hashMapClass) {
            LOGE("cmap_to_jmap_nested_3: Failed to find HashMap class");
            return nullptr;
        }

        jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
        if (!hashMapConstructor) {
            LOGE("cmap_to_jmap_nested_3: Failed to find HashMap constructor");
            return nullptr;
        }

        jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        if (!putMethod) {
            LOGE("cmap_to_jmap_nested_3: Failed to find HashMap put method");
            return nullptr;
        }

        // 创建HashMap对象
        jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);
        if (!jmap) {
            LOGE("cmap_to_jmap_nested_3: Failed to create HashMap object");
            return nullptr;
        }

        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap_nested_3: Exception occurred during HashMap creation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            return nullptr;
        }

        // 填充数据
        for (const auto& entry : cmap) {
            LOGD("cmap_to_jmap_nested_3: processing entry: key='%s', inner map size=%zu",
                 entry.first.c_str(), entry.second.size());

            jstring jkey = env->NewStringUTF(entry.first.c_str());
            if (!jkey) {
                LOGE("cmap_to_jmap_nested_3: Failed to create key string");
                continue;
            }

            jobject jvalue = cmap_to_jmap_nested(env, entry.second);
            if (!jvalue) {
                LOGE("cmap_to_jmap_nested_3: Failed to convert inner map");
                env->DeleteLocalRef(jkey);
                continue;
            }

            env->CallObjectMethod(jmap, putMethod, jkey, jvalue);

            // 检查是否抛出异常
            if (env->ExceptionCheck()) {
                LOGE("cmap_to_jmap_nested_3: Exception occurred during put operation");
                env->ExceptionDescribe();
                env->ExceptionClear();
                env->DeleteLocalRef(jkey);
                env->DeleteLocalRef(jvalue);
                continue;
            }

            // 清理局部引用
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
        }

        LOGI("cmap_to_jmap_nested_3: conversion completed successfully");
        return jmap;
    }

};

#endif //OVERT_ZMANAGER_H
