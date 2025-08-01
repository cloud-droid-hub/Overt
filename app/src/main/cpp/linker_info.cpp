//
// Created by lxz on 2025/6/12.
//


#include "zLinker.h"
#include "zUtil.h"
#include "zLog.h"
#include "linker_info.h"

map<string, map<string, string>> get_linker_info(){
    map<string, map<string, string>> info;
    vector<string> libpath_list = zLinker::getInstance()->get_libpath_list();
    for (int i = 0; i < libpath_list.size(); ++i) {
        LOGD("libpath %s", libpath_list[i].c_str());
        if(strstr(libpath_list[i].c_str(), "lsposed")){
            LOGW("Found blacklisted library: %s", libpath_list[i].c_str());
            info[libpath_list[i]]["risk"] = "error";
            info[libpath_list[i]]["explain"] = "black soname";
        }
        if(strstr(libpath_list[i].c_str(), "frida")){
            LOGW("Found blacklisted library: %s", libpath_list[i].c_str());
            info[libpath_list[i]]["risk"] = "error";
            info[libpath_list[i]]["explain"] = "black soname";
        }
    }

    vector<string> so_list{
            "libc.so",
            "libart.so",
            "libinput.so",
    };

    for(int i = 0; i < so_list.size(); ++i) {
        string so_path = so_list[i];
        string so_name = so_path.substr(so_path.rfind('/') + 1);
        if(string_end_with(so_name.c_str(), ".so")){
            int ret = zLinker::check_lib_crc(so_name.c_str());
            LOGD("check_lib_crc %s %d", so_name.c_str(), ret);
            if (ret!= 0){
                LOGW("CRC check failed for library: %s", so_name.c_str());
                info[so_name]["risk"] = "error";
                info[so_name]["explain"] = "check_lib_crc error";
            }
        }
    }

    return info;
}

