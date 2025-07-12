//
// Created by lxz on 2025/7/12.
//

#include "fd_info.h"

#include <unistd.h>
#include "zLog.h"
#include "zFile.h"
#include "util.h"
#include "zDevice.h"

std::map<std::string, std::map<std::string, std::string>> get_fd_info(){
    std::map<std::string, std::map<std::string, std::string>> info;

    zFile google("/data/data/com.google.android.gms");

    LOGE("google: %s", google.getEarliestTimeFormatted().c_str());


//    zFile fd("/proc/self/fd");
//
//    std::vector<std::string> files = fd.listFiles();
//
//    for(std::string path : files){
//        LOGE("path: %s", path.c_str());
//    }


    return info;

}