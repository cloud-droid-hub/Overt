//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_UTIL_H
#define OVERT_UTIL_H

#include <jni.h>
#include <string>
#include <map>
#include <vector>
#include <asm-generic/fcntl.h>

std::string get_line(int fd);

std::vector<std::string> get_file_lines(std::string path);

std::vector<std::string> split_str(const std::string& str, const std::string& split);

std::vector<std::string> split_str(const std::string& str, char delim);

bool string_end_with(const char *str, const char *suffix);

bool check_file_exist(std::string path);

// 将 std::map<std::string, std::string> 转换为 Java Map<String, String>
jobject cmap_to_jmap(JNIEnv *env, std::map<std::string, std::string> cmap);

// 将 std::map<std::string, std::map<std::string, std::string>> 转换为 Java Map<String, Map<String, String>>
jobject cmap_to_jmap_nested(JNIEnv* env, const std::map<std::string, std::map<std::string, std::string>>& cmap);

// 主函数：将 std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
jobject cmap_to_jmap_nested_3(JNIEnv* env, const std::map<std::string, std::map<std::string, std::map<std::string, std::string>>>& cmap);

#endif //OVERT_UTIL_H
