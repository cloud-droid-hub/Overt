//
// Created by lxz on 2025/6/12.
//

#include "linker_info.h"
#include "zLinker.h"
#include "util.h"
#include <android/log.h>

#include "zLog.h"


std::map<std::string, std::map<std::string, std::string>> get_linker_info(){
    std::map<std::string, std::map<std::string, std::string>> info;
    std::vector<std::string> libpath_list = zLinker::getInstance()->get_libpath_list();
    for (int i = 0; i < libpath_list.size(); ++i) {
        LOGE("libpath %s", libpath_list[i].c_str());
        if(strstr(libpath_list[i].c_str(), "lsposed")){
            info[libpath_list[i]]["risk"] = "error";
            info[libpath_list[i]]["explain"] = "black soname";
        }
        if(strstr(libpath_list[i].c_str(), "frida")){
            info[libpath_list[i]]["risk"] = "error";
            info[libpath_list[i]]["explain"] = "black soname";
        }
    }

    std::vector<std::string> so_list{
            "libc.so",
            "libart.so",
            "libinput.so",
    };

    for(int i = 0; i < so_list.size(); ++i) {
        std::string so_path = so_list[i];
        std::string so_name = so_path.substr(so_path.rfind('/') + 1);
        if(string_end_with(so_name.c_str(), ".so")){
            int ret = zLinker::check_lib_hash(so_name.c_str());
            LOGE("check_lib_hash %s %d", so_name.c_str(), ret);
            if (ret!= 0){
                info[so_name]["risk"] = "error";
                info[so_name]["explain"] = "check_lib_hash error";
            }
        }
    }

    return info;
}

