//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_UTIL_H
#define OVERT_UTIL_H

#include <string>
#include <map>
#include <vector>
#include <asm-generic/fcntl.h>
#include "libc.h"
#include "android/log.h"
#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

std::string get_line(int fd);

std::vector<std::string> get_file_lines(std::string path);

#endif //OVERT_UTIL_H
