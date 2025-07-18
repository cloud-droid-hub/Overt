//
// Created by lxz on 2025/7/17.
//

#ifndef OVERT_TEE_INFO_H
#define OVERT_TEE_INFO_H



#include <string>
#include <map>
#include <vector>
#include <jni.h>

std::map<std::string, std::map<std::string, std::string>> get_tee_info();
std::map<std::string, std::map<std::string, std::string>> get_tee_info(JNIEnv* env, jobject context);


#endif //OVERT_TEE_INFO_H
