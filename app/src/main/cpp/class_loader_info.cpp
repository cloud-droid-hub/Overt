//
// Created by lxz on 2025/7/3.
//
#include <jni.h>

#include "zClassLoader.h"
#include "zJavaVm.h"
#include "zLog.h"
#include "class_loader_info.h"

/**
 * 获取类加载器信息
 * 分析系统中所有类加载器，检测可疑的类加载器类型
 * 主要用于检测LSPosed、Xposed等框架注入的类加载器
 * @return 包含类加载器风险信息的映射表
 */
map<string, map<string, string>> get_class_loader_info(){
    LOGD("get_class_loader_info called");
    map<string, map<string, string>> info;

    // 遍历所有类加载器字符串列表
    for(const string& str : zClassLoader::getInstance()->classLoaderStringList) {
        LOGD("Checking classloader string: %s", str.c_str());
        
        // 跳过空字符串
        if (str.empty()) {
            continue;
        }
        
        LOGI("classloader：%s", str.c_str());
        
        // 检测LSPosed框架的类加载器
        if(strstr(str.c_str(), "LspModuleClassLoader")) {
            info[str]["risk"] = "error";
            info[str]["explain"] = "black classloader";
        }
        // 检测内存中动态加载的DEX类加载器（常用于Xposed等框架）
        else if(strstr(str.c_str(), "InMemoryDexClassLoader") && 
                strstr(str.c_str(), "InMemoryDexFile[cookie=[0, -")) {
            info[str]["risk"] = "error";
            info[str]["explain"] = "black classloader";
        }
    }
    return info;
}

/**
 * 获取类信息
 * 分析系统中所有已加载的类，检测可疑的类名
 * 主要用于检测LSPosed、Xposed等框架相关的类
 * @return 包含类风险信息的映射表
 */
map<string, map<string, string>> get_class_info(){
    LOGD("get_class_info called");
    map<string, map<string, string>> info;

    // 定义黑名单类名列表，包含常见的Xposed/LSPosed相关类
    vector<string> black_name_list = {
            "lsposed",           // LSPosed框架相关
            "lspd",              // LSPosed框架相关
            "XposedHooker",      // Xposed钩子类
            "XposedHelpers",     // Xposed辅助类
            "io.github.libxposed.api", // Xposed API包
            "XposedInit",        // Xposed初始化类
            "XposedBridge"       // Xposed桥接类
    };

    // 遍历黑名单中的每个类名
    for(string black_name: black_name_list){
        LOGD("Checking black_name: %s", black_name.c_str());
        
        // 遍历所有已加载的类名
        for(string className : zClassLoader::getInstance()->classNameList){
            LOGD("Checking className: %s", className.c_str());
            
            // 跳过空类名
            if (className.empty()) {
                continue;
            }
            
            // 检查类名是否包含黑名单中的字符串
            if(strstr(className.c_str(), black_name.c_str())){
                LOGE("get_class_info className %s is find", className.c_str());
                info[className]["risk"] = "error";
                info[className]["explain"] = "Risk: black class";
                break; // 找到匹配项后跳出内层循环
            }
        }
    }

    return info;
}