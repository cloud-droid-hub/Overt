//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_ZUTIL_H
#define OVERT_ZUTIL_H

#include <jni.h>
#include <asm-generic/fcntl.h>

#include "config.h"

string get_line(int fd);

vector<string> get_file_lines(string path);

vector<string> split_str(const string& str, const string& split);

vector<string> split_str(const string& str, char delim);

bool string_end_with(const char *str, const char *suffix);
bool string_start_with(const char *str, const char *prefix);

string format_timestamp(long timestamp);

// 将 map<string, string> 转换为 Java Map<String, String>
jobject cmap_to_jmap(JNIEnv *env, const map<string, string>& cmap);

// 将 map<string, map<string, string>> 转换为 Java Map<String, Map<String, String>>
jobject cmap_to_jmap_nested(JNIEnv* env, const map<string, map<string, string>>& cmap);

// 主函数：将 map<string, map<string, map<string, string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
jobject cmap_to_jmap_nested_3(JNIEnv* env, const map<string, map<string, map<string, string>>>& cmap);

// string的字符串格式化函数
template<typename ... Args>
string string_format(const string &format, Args ... args)
{
    auto size_buf = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;

    std::unique_ptr<char[]> buf(new(std::nothrow) char[size_buf]);

    if (!buf) return string("");

    std::snprintf(buf.get(), size_buf, format.c_str(), args ...);

    return string(buf.get(), buf.get() + size_buf - 1);
}

#endif //OVERT_ZUTIL_H
