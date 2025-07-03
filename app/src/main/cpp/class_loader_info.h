//
// Created by lxz on 2025/7/3.
//

#ifndef OVERT_CLASS_LOADER_INFO_H
#define OVERT_CLASS_LOADER_INFO_H


#include <string>
#include <map>
#include <vector>
std::map<std::string, std::map<std::string, std::string>> get_class_loader_info(JNIEnv *env);
std::map<std::string, std::map<std::string, std::string>> get_class_info(JNIEnv *env);


#endif //OVERT_CLASS_LOADER_INFO_H
