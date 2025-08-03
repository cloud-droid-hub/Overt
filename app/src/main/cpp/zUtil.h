//
// Created by lxz on 2025/6/6.
//

#ifndef OVERT_ZUTIL_H
#define OVERT_ZUTIL_H

#include <jni.h>
#include <asm-generic/fcntl.h>

#include "config.h"

/**
 * 从文件描述符读取一行数据
 * @param fd 文件描述符
 * @return 读取到的行内容
 */
string get_line(int fd);

/**
 * 读取文件的所有行
 * @param path 文件路径
 * @return 文件的所有行内容
 */
vector<string> get_file_lines(string path);

/**
 * 按字符串分割字符串
 * @param str 待分割的字符串
 * @param split 分割符
 * @return 分割后的字符串数组
 */
vector<string> split_str(const string& str, const string& split);

/**
 * 按字符分割字符串
 * @param str 待分割的字符串
 * @param delim 分割字符
 * @return 分割后的字符串数组
 */
vector<string> split_str(const string& str, char delim);

/**
 * 检查字符串是否以指定后缀结尾
 * @param str 待检查的字符串
 * @param suffix 后缀字符串
 * @return 是否以指定后缀结尾
 */
bool string_end_with(const char *str, const char *suffix);

/**
 * 检查字符串是否以指定前缀开头
 * @param str 待检查的字符串
 * @param prefix 前缀字符串
 * @return 是否以指定前缀开头
 */
bool string_start_with(const char *str, const char *prefix);

/**
 * 格式化时间戳为可读字符串
 * @param timestamp 时间戳
 * @return 格式化后的时间字符串
 */
string format_timestamp(long timestamp);

/**
 * 将 map<string, string> 转换为 Java Map<String, String>
 * @param env JNI环境指针
 * @param cmap C++ Map对象
 * @return Java Map对象
 */
jobject cmap_to_jmap(JNIEnv *env, const map<string, string>& cmap);

/**
 * 将 map<string, map<string, string>> 转换为 Java Map<String, Map<String, String>>
 * @param env JNI环境指针
 * @param cmap C++嵌套Map对象
 * @return Java嵌套Map对象
 */
jobject cmap_to_jmap_nested(JNIEnv* env, const map<string, map<string, string>>& cmap);

/**
 * 将 map<string, map<string, map<string, string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
 * @param env JNI环境指针
 * @param cmap C++三层嵌套Map对象
 * @return Java三层嵌套Map对象
 */
jobject cmap_to_jmap_nested_3(JNIEnv* env, const map<string, map<string, map<string, string>>>& cmap);

/**
 * 字符串格式化函数模板
 * 支持可变参数格式化字符串
 * @param format 格式化字符串
 * @param args 可变参数
 * @return 格式化后的字符串
 */
template<typename ... Args>
string string_format(const string &format, Args ... args)
{
    auto size_buf = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;

    std::unique_ptr<char[]> buf(new(std::nothrow) char[size_buf]);

    if (!buf) return string("");

    std::snprintf(buf.get(), size_buf, format.c_str(), args ...);

    return string(buf.get(), buf.get() + size_buf - 1);
}

/**
 * 获取大核心CPU列表
 * 通过读取CPU频率信息识别大核心
 * @return 大核心CPU ID列表
 */
vector<int> get_big_core_list();

/**
 * 将当前线程绑定到使用最少的大核心上
 * 用于优化性能，确保线程运行在高性能核心上
 */
void bind_self_to_least_used_big_core();

/**
 * 提升线程优先级
 * @param sched_priority 调度优先级（默认0）
 */
void raise_thread_priority(int sched_priority = 0);

#endif //OVERT_ZUTIL_H
