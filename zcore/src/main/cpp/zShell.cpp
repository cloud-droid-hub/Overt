//
// Created by lxz on 2025/8/11.
//
#include <unistd.h>
#include "zLog.h"
#include "zStd.h"

string runShell(string cmd){
    string ret = "";
    FILE *fp = popen(cmd.c_str(), "r");
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        ret += buffer;
    }
    pclose(fp);
    return ret;
}
