//
// Created by lxz on 2025/8/6.
//

#ifndef OVERT_ZSTDUTIL_H
#define OVERT_ZSTDUTIL_H

#include "zStd.h"

string get_line(int fd);

vector<string> get_file_lines(string path);

// 分割字符串，返回字符串数组，分割算法用 c 来实现，不利用 api
vector<string> split_str(const string& str, const string& split);

vector<string> split_str(const string& str, char delim);

string format_timestamp(long timestamp);

bool string_end_with(const char *str, const char *suffix);

bool string_start_with(const char *str, const char *prefix);

string itoa(int value, int base);

unsigned long stoul(const char* str, char** endptr, int base);


string string_format(const char* format, ...);


#endif //OVERT_ZSTDUTIL_H
