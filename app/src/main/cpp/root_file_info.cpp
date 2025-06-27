//
// Created by lxz on 2025/6/6.
//

#include <asm-generic/fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/fcntl.h>
#include "root_file_info.h"
#include "string.h"
#include "libc.h"
#include <android/log.h>

#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

bool check_file_exist_1(nonstd::string path) {
    int fd = nonstd::open(path.c_str(), O_RDONLY);
    if (fd >= 0) {
        close(fd);
        return true;
    }
    return false;
}

bool check_file_exist_2(std::string path) {
    if (access(path.c_str(), F_OK) != -1) {
        return true;
    }
    return false;
}

// 综合检查函数
bool check_file_exist(nonstd::string path) {
    bool file_exist = false;

    // 使用多种方法检查文件是否存在
    bool exist1 = check_file_exist_1(path);
    bool exist2 = check_file_exist_2(path.c_str());

    // 记录每种方法的检查结果
    LOGE("check_file_1 (ifstream): %d", exist1);
    LOGE("check_file_2 (access): %d", exist2);

    // 如果任一方法检测到文件存在，则认为文件存在
    file_exist = exist1 || exist2;

    return file_exist;
}

std::map<std::string, std::string> get_root_file_info(){
    std::map<std::string, std::string> info;
    const char* paths[] = {
            "/sbin/su",
            "/system/bin/su",
            "/system/xbin/su",
            "/data/local/xbin/su",
            "/data/local/bin/su",
            "/system/sd/xbin/su",
            "/system/bin/failsafe/su",
            "/data/local/su",
            "/system/xbin/mu",
            "/system_ext/bin/su",
            "/apex/com.android.runtime/bin/suu"
    };

    for (const char* path : paths) {
        if (check_file_exist(path)) {
            info[path] = "exist";
        }
    }

    return info;
}
