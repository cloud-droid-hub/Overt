//
// Created by lxz on 2025/7/3.
//
#include <jni.h>
#include "class_loader_info.h"
#include "zClassLoader.h"
#include "zJavaVm.h"
#include "zLog.h"

std::map<std::string, std::map<std::string, std::string>> get_class_loader_info(){
    std::map<std::string, std::map<std::string, std::string>> info;

    // 处理类加载器列表
    for(const std::string& str : zClassLoader::getInstance()->classLoaderStringList) {
        if (str.empty()) {
            continue;
        }
        LOGE("classloader：%s", str.c_str());
        if(strstr(str.c_str(), "LspModuleClassLoader")) {
            info[str]["risk"] = "black";
            info[str]["explain"] = "Risk: blacklisted classloader";
        }else if(strstr(str.c_str(), "InMemoryDexClassLoader") && strstr(str.c_str(), "InMemoryDexFile[cookie=[0, -")) {
            info[str]["risk"] = "black";
            info[str]["explain"] = "Risk: blacklisted classloader";
        }
    }
    return info;
}

std::map<std::string, std::map<std::string, std::string>> get_class_info(){
    std::map<std::string, std::map<std::string, std::string>> info;

    std::vector<std::string> black_name_list = {
            "lsposed", "lspd"
            "XposedHooker"
    };

    for(std::string className : zClassLoader::getInstance()->classNameList){
        if (className.empty()) {
            continue;
        }
        std::transform(className.begin(), className.end(), className.begin(), [](unsigned char c) { return std::tolower(c); });
        for(std::string black_name: black_name_list){
            std::transform(black_name.begin(), black_name.end(), black_name.begin(), [](unsigned char c) { return std::tolower(c); });
            if(strstr(className.c_str(), black_name.c_str())){
                LOGE("className %s", className.c_str());
                info[className]["risk"] = "error";
                info[className]["explain"] = "Risk: blacklisted class";
            }
        }
    }

    return info;
}