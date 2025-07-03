//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_ROOT_FILE_INFO_H
#define OVERT_ROOT_FILE_INFO_H

#include <string>
#include <map>
#include <vector>

bool check_file_exist_2(std::string path);

std::map<std::string, std::map<std::string, std::string>> get_root_file_info();

#endif //OVERT_ROOT_FILE_INFO_H
