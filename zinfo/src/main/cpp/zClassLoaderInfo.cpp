//
// Created by lxz on 2025/7/3.
//
#include <jni.h>
#include "zLog.h"
#include "zStd.h"
#include "zLibc.h"
#include "zClassLoader.h"
#include "zJavaVm.h"
#include "zClassLoaderInfo.h"

/**
 * 获取类加载器信息(查杀分离 — 采集端)
 * 遍历系统中所有类加载器并全量上报其字符串表示，不做任何过滤；
 * 风险判定(哪些类加载器特征可疑)由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{类加载器字符串 -> {value: 相同字符串}}
 */
map<string, map<string, string>> get_class_loader_info(){
    LOGD("get_class_loader_info called");
    map<string, map<string, string>> info;

    // 遍历所有类加载器字符串列表，全量上报(不内置过滤)
    for(const string& str : zClassLoader::getInstance()->classLoaderStringList) {
        LOGD("Checking classloader string: %s", str.c_str());

        // 跳过空字符串
        if (str.empty()) {
            continue;
        }

        info[str]["value"] = str;
    }
    LOGI("class_loader_info raw count=%zu", info.size());
    return info;
}

/**
 * 获取类信息(查杀分离 — 采集端)
 * 遍历所有已加载的类并全量上报类名，不做任何过滤；
 * 风险判定(哪些类名可疑)由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{类名 -> {value: 相同类名}}
 */
map<string, map<string, string>> get_class_info(){
    LOGD("get_class_info called");
    map<string, map<string, string>> info;

    // 全量上报所有已加载的非空类名(不内置黑名单过滤)
    for(string className : zClassLoader::getInstance()->classNameList){
        // 跳过空类名
        if (className.empty()) {
            continue;
        }
        info[className]["value"] = className;
    }

    LOGI("class_info raw count=%zu", info.size());
    return info;
}