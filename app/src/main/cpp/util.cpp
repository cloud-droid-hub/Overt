//
// Created by lxz on 2025/6/6.
//

#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include "util.h"
#include "zLog.h"

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

// 将 map<string, string> 转换为 Java Map<String, String>
jobject cmap_to_jmap(JNIEnv *env, map<string, string> cmap){
    jobject jobjectMap = env->NewObject(env->FindClass("java/util/HashMap"), env->GetMethodID(env->FindClass("java/util/HashMap"), "<init>", "()V"));
    for (auto &entry : cmap) {
        env->CallObjectMethod(jobjectMap, env->GetMethodID(env->FindClass("java/util/HashMap"), "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"), env->NewStringUTF(entry.first.c_str()), env->NewStringUTF(entry.second.c_str()));
    }
    return jobjectMap;
}

// 将 map<string, map<string, string>> 转换为 Java Map<String, Map<String, String>>
jobject cmap_to_jmap_nested(JNIEnv* env, const map<string, map<string, string>>& cmap) {
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);

    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    for (const auto& entry : cmap) {
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        jobject jvalue = cmap_to_jmap(env, entry.second); // 调用之前定义的 cmap_to_jmap 方法
        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    return jmap;
}

// 主函数：将 map<string, map<string, map<string, string>>> 转换为 Java Map<String, Map<String, Map<String, String>>>
jobject cmap_to_jmap_nested_3(JNIEnv* env, const map<string, map<string, map<string, string>>>& cmap) {
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    jobject jmap = env->NewObject(hashMapClass, hashMapConstructor);

    jmethodID putMethod = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    for (const auto& entry : cmap) {
        jstring jkey = env->NewStringUTF(entry.first.c_str());
        jobject jvalue = cmap_to_jmap_nested(env, entry.second);
        env->CallObjectMethod(jmap, putMethod, jkey, jvalue);
        env->DeleteLocalRef(jkey);
        env->DeleteLocalRef(jvalue);
    }
    return jmap;
}