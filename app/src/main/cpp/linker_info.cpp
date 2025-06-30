//
// Created by lxz on 2025/6/12.
//

#include "linker_info.h"
#include "zLinker.h"
#include <android/log.h>

#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

std::map<std::string, std::string> get_linker_info(){
    std::map<std::string, std::string> info;
    std::vector<std::string> libpath_list = zLinker::getInstance()->get_libpath_list();
    for (int i = 0; i < libpath_list.size(); ++i) {
        if(strstr(libpath_list[i].c_str(), "lsposed")){
            info[libpath_list[i]] = "error";
        }
    }
    return info;
}

