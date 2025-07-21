//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_UTIL_H
#define OVERT_UTIL_H

#include <jni.h>
#include <asm-generic/fcntl.h>
#include "config.h"

string get_line(int fd);

vector<string> get_file_lines(string path);

vector<string> split_str(const string& str, const string& split);

vector<string> split_str(const string& str, char delim);

bool string_end_with(const char *str, const char *suffix);

// 将 map<string, string> 转换为 Java Map<String, String>
jobject cmap_to_jmap(JNIEnv *env, map<string, string> cmap);

// 将 map<string, map<string, string>> 转换为 Java Map<String, Map<String, String>>
jobject cmap_to_jmap_nested(JNIEnv* env, const map<string, map<string, string>>& cmap);

// 主函数：将 map<string, map<string, map<string, string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
jobject cmap_to_jmap_nested_3(JNIEnv* env, const map<string, map<string, map<string, string>>>& cmap);

#endif //OVERT_UTIL_H
