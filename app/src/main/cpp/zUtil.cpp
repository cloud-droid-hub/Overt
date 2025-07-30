//
// Created by lxz on 2025/6/6.
//

#include <unistd.h>
#include <fcntl.h>
#include <jni.h>

#include "zLog.h"
#include "zUtil.h"

string get_line(int fd) {
    char buffer;
    string line = "";
    while (true) {
        ssize_t bytes_read = read(fd, &buffer, sizeof(buffer));
        if (bytes_read == 0) break;
        line += buffer;
        if (buffer == '\n') break;
    }
    return line;
}

vector<string> get_file_lines(string path){
    vector<string> file_lines = vector<string>();
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return file_lines;
    }
    while (true) {
        string line = get_line(fd);
        if (line.size() == 0) {
            break;
        }
        file_lines.push_back(line);
    }
    close(fd);
    return file_lines;
}

// 分割字符串，返回字符串数组，分割算法用 c 来实现，不利用 api
vector<string> split_str(const string& str, const string& split) {
    vector<string> result;

    if (split.empty()) {
        result.push_back(str);  // 不分割
        return result;
    }

    const char* s = str.c_str();
    size_t str_len = str.length();
    size_t split_len = split.length();

    size_t i = 0;
    while (i < str_len) {
        size_t j = i;

        // 查找下一个分隔符位置
        while (j <= str_len - split_len && memcmp(s + j, split.c_str(), split_len) != 0) {
            ++j;
        }

        // 提取子串
        result.emplace_back(s + i, j - i);

        // 跳过分隔符
        i = (j <= str_len - split_len) ? j + split_len : str_len;
    }

    // 处理末尾刚好是分隔符的情况，需补一个空串
    if (str_len >= split_len && str.substr(str_len - split_len) == split) {
        result.emplace_back("");
    }

    return result;
}

vector<string> split_str(const string& str, char delim) {
    vector<string> result;
    const char* s = str.c_str();
    size_t start = 0;
    size_t len = str.length();

    for (size_t i = 0; i <= len; ++i) {
        if (s[i] == delim || s[i] == '\0') {
            // 只有当子串不为空时才添加到结果中
            if (i > start) {
                result.emplace_back(s + start, i - start);
            }
            start = i + 1;
        }
    }
    return result;
}

bool string_end_with(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }

    size_t len_str = strlen(str);
    size_t len_suffix = strlen(suffix);

    if (len_suffix > len_str) {
        return false;
    }

    return (strcmp(str + len_str - len_suffix, suffix) == 0);
}

bool string_start_with(const char *str, const char *prefix) {
  if (!str || !prefix) {
    return false;
  }

  size_t len_str    = strlen(str);
  size_t len_prefix = strlen(prefix);

  if (len_prefix > len_str) {
    return false;
  }

  return (strncmp(str, prefix, len_prefix) == 0);
}

// 将 map<string, string> 转换为 Java Map<String, String>
jobject cmap_to_jmap(JNIEnv *env, const map<string, string>& cmap){
    LOGE("cmap_to_jmap: starting conversion, map size=%zu", cmap.size());
    LOGE("cmap_to_jmap: map type info - is nonstd::map: %s", typeid(cmap).name());
    
    // 遍历并打印map内容
    LOGE("cmap_to_jmap: map contents:");
    for (const auto &entry : cmap) {
        LOGE("cmap_to_jmap: key='%s', value='%s'", entry.first.c_str(), entry.second.c_str());
    }
    
    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGE("cmap_to_jmap: Failed to find HashMap class");
        return nullptr;
    }
    
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGE("cmap_to_jmap: Failed to find HashMap constructor");
        return nullptr;
    }
    
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGE("cmap_to_jmap: Failed to find HashMap put method");
        return nullptr;
    }
    
    // 创建HashMap对象
    jobject jobjectMap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jobjectMap) {
        LOGE("cmap_to_jmap: Failed to create HashMap object");
        return nullptr;
    }
    
    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGE("cmap_to_jmap: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }
    
    // 填充数据
    for (const auto &entry : cmap) {
        LOGE("cmap_to_jmap: processing entry: key='%s', value='%s'", 
             entry.first.c_str(), entry.second.c_str());
        
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        jstring jvalue = env->NewStringUTF(entry.second.c_str());
        
        if (!jkey || !jvalue) {
            LOGE("cmap_to_jmap: Failed to create string objects");
            if (jkey) env->DeleteLocalRef(jkey);
            if (jvalue) env->DeleteLocalRef(jvalue);
            continue;
        }
        
        env->CallObjectMethod(jobjectMap, putMethod, jkey, jvalue);
        
        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }
        
        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    
    LOGE("cmap_to_jmap: conversion completed successfully");
    return jobjectMap;
}

// 将 map<string, map<string, string>> 转换为 Java Map<String, Map<String, String>>
jobject cmap_to_jmap_nested(JNIEnv* env, const map<string, map<string, string>>& cmap) {
    LOGE("cmap_to_jmap_nested: starting conversion, map size=%zu", cmap.size());
    LOGE("cmap_to_jmap_nested: map type info - is nonstd::map: %s", typeid(cmap).name());
    
    // 遍历并打印map内容
    LOGE("cmap_to_jmap_nested: map contents:");
    for (const auto &entry : cmap) {
        LOGE("cmap_to_jmap_nested: key='%s', inner map size=%zu", entry.first.c_str(), entry.second.size());
        for (const auto &inner_entry : entry.second) {
            LOGE("cmap_to_jmap_nested: inner key='%s', value='%s'", inner_entry.first.c_str(), inner_entry.second.c_str());
        }
    }
    
    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap class");
        return nullptr;
    }
    
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap constructor");
        return nullptr;
    }
    
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGE("cmap_to_jmap_nested: Failed to find HashMap put method");
        return nullptr;
    }
    
    // 创建HashMap对象
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jmap) {
        LOGE("cmap_to_jmap_nested: Failed to create HashMap object");
        return nullptr;
    }
    
    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGE("cmap_to_jmap_nested: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }
    
    // 填充数据
    for (const auto& entry : cmap) {
        LOGE("cmap_to_jmap_nested: processing entry: key='%s', inner map size=%zu", 
             entry.first.c_str(), entry.second.size());
        
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        if (!jkey) {
            LOGE("cmap_to_jmap_nested: Failed to create key string");
            continue;
        }
        
        jobject jvalue = cmap_to_jmap(env, entry.second); // 调用之前定义的 cmap_to_jmap 方法
        if (!jvalue) {
            LOGE("cmap_to_jmap_nested: Failed to convert inner map");
            env->DeleteLocalRef(jkey);
            continue;
        }
        
        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);
        
        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap_nested: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }
        
        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    
    LOGE("cmap_to_jmap_nested: conversion completed successfully");
    return jmap;
}

// 主函数：将 map<string, map<string, map<string, string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
jobject cmap_to_jmap_nested_3(JNIEnv* env, const map<string, map<string, map<string, string>>>& cmap) {
    LOGE("cmap_to_jmap_nested_3: starting conversion, map size=%zu", cmap.size());
    LOGE("cmap_to_jmap_nested_3: map type info - is nonstd::map: %s", typeid(cmap).name());
    
    // 遍历并打印map内容
    LOGE("cmap_to_jmap_nested_3: map contents:");
    for (const auto &entry : cmap) {
        LOGE("cmap_to_jmap_nested_3: key='%s', inner map size=%zu", entry.first.c_str(), entry.second.size());
        for (const auto &inner_entry : entry.second) {
            LOGE("cmap_to_jmap_nested_3: inner key='%s', inner inner map size=%zu", inner_entry.first.c_str(), inner_entry.second.size());
            for (const auto &inner_inner_entry : inner_entry.second) {
                LOGE("cmap_to_jmap_nested_3: inner inner key='%s', value='%s'", inner_inner_entry.first.c_str(), inner_inner_entry.second.c_str());
            }
        }
    }
    
    // 查找类和方法ID（只查找一次）
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (!hashMapClass) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap class");
        return nullptr;
    }
    
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (!hashMapConstructor) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap constructor");
        return nullptr;
    }
    
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (!putMethod) {
        LOGE("cmap_to_jmap_nested_3: Failed to find HashMap put method");
        return nullptr;
    }
    
    // 创建HashMap对象
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);
    if (!jmap) {
        LOGE("cmap_to_jmap_nested_3: Failed to create HashMap object");
        return nullptr;
    }
    
    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGE("cmap_to_jmap_nested_3: Exception occurred during HashMap creation");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }
    
    // 填充数据
    for (const auto& entry : cmap) {
        LOGE("cmap_to_jmap_nested_3: processing entry: key='%s', inner map size=%zu", 
             entry.first.c_str(), entry.second.size());
        
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        if (!jkey) {
            LOGE("cmap_to_jmap_nested_3: Failed to create key string");
            continue;
        }
        
        jobject jvalue = cmap_to_jmap_nested(env, entry.second);
        if (!jvalue) {
            LOGE("cmap_to_jmap_nested_3: Failed to convert inner map");
            env->DeleteLocalRef(jkey);
            continue;
        }
        
        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);
        
        // 检查是否抛出异常
        if (env->ExceptionCheck()) {
            LOGE("cmap_to_jmap_nested_3: Exception occurred during put operation");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jkey);
            env->DeleteLocalRef(jvalue);
            continue;
        }
        
        // 清理局部引用
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    
    LOGE("cmap_to_jmap_nested_3: conversion completed successfully");
    return jmap;
}


string format_timestamp(long timestamp) {
    LOGE("format_timestamp: called with timestamp=%ld", timestamp);

    if (timestamp <= 0) {
        LOGE("format_timestamp: invalid timestamp (<=0): %ld", timestamp);
        return "Invalid timestamp";
    }

    // 转换为本地时间
    time_t time = (time_t)timestamp;
    LOGE("format_timestamp: converted to time_t: %ld", time);

    struct tm* timeinfo = localtime(&time);
    if (timeinfo == nullptr) {
        LOGE("format_timestamp: localtime failed for timestamp=%ld", timestamp);
        return "Failed to convert time";
    }

    // 格式化时间
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 输出调试信息
    LOGE("Timestamp conversion: input=%ld, year=%d, month=%d, day=%d, hour=%d, min=%d, sec=%d",
         timestamp, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
         timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    LOGE("format_timestamp: returning formatted string: %s", buffer);
    return string(buffer);
}

