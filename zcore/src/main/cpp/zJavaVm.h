//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZJAVAVM_H
#define OVERT_ZJAVAVM_H

#include <jni.h>
#include <asm-generic/mman.h>
#include <sys/mman.h>
#include <shared_mutex>
#include <map>
#include <mutex>
#include <thread>

#include "zLog.h"


// 页面起始地址计算宏
#define PAGE_START(x)  ((x) & PAGE_MASK)

/**
 * Java虚拟机管理类
 * 采用单例模式，负责管理JVM实例、JNI环境、Context和ClassLoader
 * 提供跨线程的Java对象访问能力，支持动态类加载
 */
class zJavaVm {
private:
    // 私有构造函数，防止外部实例化
    zJavaVm();
    
    // 禁用拷贝构造函数
    zJavaVm(const zJavaVm&) = delete;
    
    // 禁用赋值操作符
    zJavaVm& operator = (const zJavaVm&) = delete;
    
    // 单例实例指针
    static zJavaVm* instance;
    
    // Java虚拟机指针
    JavaVM* jvm = nullptr;
    
    // 线程ID到JNIEnv的映射表（线程安全）
    std::map<int, JNIEnv*> thread_env_map;
    mutable std::mutex env_map_mutex;
    
    // Android上下文对象（全局引用）
    jobject context = nullptr;
    
    // 自定义上下文对象（全局引用），当正常Context不可用时使用
    jobject custom_context = nullptr;
    
    // 应用类加载器（全局引用）
    jobject class_loader = nullptr;

public:
    /**
     * 获取单例实例
     * @return zJavaVm单例指针
     */

    static zJavaVm* getInstance() {
        // 双重检查锁定模式：先检查，避免不必要的锁开销
        if (instance == nullptr) {
            static std::mutex instance_mutex;
            std::lock_guard<std::mutex> lock(instance_mutex);

            // 再次检查，防止多线程竞争
            if (instance == nullptr) {
                try {
                    instance = new zJavaVm();
                    LOGI("zJavaVm: Created new singleton instance");
                } catch (const std::exception& e) {
                    LOGE("zJavaVm: Failed to create singleton instance: %s", e.what());
                    return nullptr;
                } catch (...) {
                    LOGE("zJavaVm: Failed to create singleton instance with unknown error");
                    return nullptr;
                }
            }
        }
        return instance;
    }
    /**
     * 初始化JVM管理器
     * 只能在主线程调用
     */
    void init();

    /**
     * 获取Java虚拟机指针
     * @return JavaVM指针
     */
    JavaVM* getJvm();

    /**
     * 获取JNI环境指针
     * 将当前线程附加到JVM并获取JNI环境
     * @return JNIEnv指针，失败时返回nullptr
     */
    JNIEnv* getEnv();

    /**
     * 获取Android上下文对象
     * 优先使用当前Context，失败时创建新的Context
     * @return 全局引用的Context对象
     */
    jobject getContext();

    /**
     * 获取应用的类加载器
     * 缓存ClassLoader实例，避免重复获取
     * @return 全局引用的ClassLoader对象
     */
    jobject getClassLoader();

    /**
     * 在子线程中获取非系统类
     * 子线程中获取非系统类必须通过这个方法，使用应用的ClassLoader
     * @param className 类名（全限定名）
     * @return 加载的类对象，失败时返回nullptr
     */
    jclass findClass(const char* className);

    /**
     * 清理当前线程的JNIEnv
     * 在线程退出时调用，避免内存泄漏
     */
    void cleanupCurrentThreadEnv();
    
    /**
     * 退出JVM
     * 通过内存操作使JVM退出，用于清理资源
     */
    void exit();
};

#endif //OVERT_ZJAVAVM_H
