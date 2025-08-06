//
// Created by lxz on 2025/4/24.
//

#ifndef DEMO_CLASSLOADER_H
#define DEMO_CLASSLOADER_H

#include <jni.h>
#include "zStd.h"


/**
 * 类加载器检测类
 * 采用单例模式，负责检测和分析系统中的类加载器
 * 通过ART内部API遍历全局引用和弱全局引用，收集类加载器信息
 * 支持检测动态加载的类和类加载器的详细信息
 */
class zClassLoader {
private:
    // 私有构造函数，防止外部实例化
    zClassLoader();

    // 禁用拷贝构造函数
    zClassLoader(const zClassLoader&) = delete;

    // 禁用赋值操作符
    zClassLoader& operator=(const zClassLoader&) = delete;

    // 单例实例指针
    static zClassLoader* instance;

public:
    /**
     * 获取单例实例
     * 采用懒加载模式，首次调用时创建实例
     * @return zClassLoader单例指针
     */
    static zClassLoader* getInstance() {
        if (instance == nullptr) {
            instance = new zClassLoader();
        }
        return instance;
    }

    // 析构函数
    ~zClassLoader();

    // 存储检测到的类名列表
    vector<string> classNameList;
    
    // 存储检测到的类加载器字符串表示列表
    vector<string> classLoaderStringList;
    
    // 初始化标志
    bool initialized = false;

    /**
     * 遍历系统中的类加载器
     * 检查全局引用和弱全局引用中的所有类加载器
     * @param env JNI环境指针
     */
    void traverseClassLoader(JNIEnv* env);
    
    /**
     * 检查全局引用中的类加载器
     * 通过ART的VisitRoots机制遍历所有全局引用
     * @param env JNI环境指针
     * @param clazz 目标类加载器类型
     */
    void checkGlobalRef(JNIEnv *env, jclass clazz);
    
    /**
     * 检查弱全局引用中的类加载器
     * 通过ART的SweepJniWeakGlobals机制遍历所有弱全局引用
     * @param env JNI环境指针
     * @param clazz 目标类加载器类型
     */
    void checkWeakGlobalRef(JNIEnv *env, jclass clazz);
};

#endif //DEMO_CLASSLOADER_H
