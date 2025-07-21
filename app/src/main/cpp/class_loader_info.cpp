//
// Created by lxz on 2025/7/3.
//
#include <jni.h>
#include "class_loader_info.h"
#include "zClassLoader.h"
#include "zJavaVm.h"
#include "zLog.h"
#include "nonstd_libc.h"


map<string, map<string, string>> get_class_loader_info(){
    map<string, map<string, string>> info;

    // 处理类加载器列表
    for(const string& str : zClassLoader::getInstance()->classLoaderStringList) {
        if (str.empty()) {
            continue;
        }
        LOGE("classloader：%s", str.c_str());
        if(nonstd_strstr(str.c_str(), "LspModuleClassLoader")) {
            info[str]["risk"] = "error";
            info[str]["explain"] = "black classloader";
        }else if(nonstd_strstr(str.c_str(), "InMemoryDexClassLoader") && nonstd_strstr(str.c_str(), "InMemoryDexFile[cookie=[0, -")) {
            info[str]["risk"] = "error";
            info[str]["explain"] = "black classloader";
        }
    }
    return info;
}

map<string, map<string, string>> get_class_info(){
    map<string, map<string, string>> info;

    vector<string> black_name_list = {
            "lsposed", "lspd", "XposedHooker", "XposedHelpers", "io.github.libxposed.api", "XposedInit", "XposedBridge"
    };

    for(string className : zClassLoader::getInstance()->classNameList){
        if (className.empty()) {
            continue;
        }
        // transform(className.begin(), className.end(), className.begin(), [](unsigned char c) { return tolower(c); });
        for(string black_name: black_name_list){
            // transform(black_name.begin(), black_name.end(), black_name.begin(), [](unsigned char c) { return tolower(c); });
            if(nonstd_strstr(className.c_str(), black_name.c_str())){
                LOGE("get_class_info className %s is find", className.c_str());
                info[className]["risk"] = "error";
                info[className]["explain"] = "Risk: black class";
                continue;
            }
        }
    }

    return info;
}