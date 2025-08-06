//
// Created by lxz on 2025/8/6.
//

#include <ctype.h>
#include <sched.h>
#include <errno.h>
#include <linux/resource.h>
#include <sys/resource.h>

#include "syscall.h"
#include "zLog.h"
#include "zLibc.h"
#include "zStdUtil.h"


string get_line(int fd) {
    LOGD("get_line called with fd: %d", fd);
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
    LOGD("get_file_lines called with path: %s", path.c_str());
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
    LOGD("split_str called with str: %s, split: %s", str.c_str(), split.c_str());
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
    LOGD("split_str called with char delim: %c", delim);
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

string itoa(int value, int base = 10){
    if (base < 2 || base > 36) {
        throw std::invalid_argument("Invalid base. Base must be between 2 and 36.");
    }

    string result;
    bool isNegative = (value < 0 && base == 10);
    unsigned int uValue = isNegative ? (unsigned int)(-value) : (unsigned int)value;

    do {
        int remainder = uValue % base;
        result = (remainder < 10 ? "0" + remainder : "a" + remainder - 10) + result;
        uValue /= base;
    } while (uValue);

    if (isNegative) {
        result = "-" + result;
    }

    return result;
}

// 自定义stoul函数
// 参数：
//   str：输入的字符串
//   endptr：指向字符串末尾的指针（可选）
//   base：进制基数（默认为10）
// 返回值：
//   转换后的无符号长整数

unsigned long stoul(const char* str, char** endptr, int base) {
    if (base < 2 || base > 36) {
        fprintf(stderr, "Invalid base. Base must be between 2 and 36.\n");
        exit(EXIT_FAILURE);
    }

    // 跳过前导空格
    while (isspace((unsigned char)*str)) {
        str++;
    }

    // 检查是否有符号（无符号类型，理论上不应该有符号）
    if (*str == '+' || *str == '-') {
        fprintf(stderr, "Invalid character in input string.\n");
        exit(EXIT_FAILURE);
    }

    unsigned long result = 0;
    const char* start = str;

    for (; *str != '\0'; str++) {
        int value;
        if (isdigit((unsigned char)*str)) {
            value = *str - '0';
        } else if (isalpha((unsigned char)*str)) {
            value = tolower((unsigned char)*str) - 'a' + 10;
        } else {
            break; // 遇到非数字字符停止
        }

        if (value >= base) {
            break; // 当前字符超出进制范围
        }

        // 检查是否超出范围
        if (result > (ULONG_MAX - value) / base) {
            fprintf(stderr, "Converted value out of range.\n");
            exit(EXIT_FAILURE);
        }

        result = result * base + value;
    }

    if (str == start) {
        fprintf(stderr, "No valid conversion could be performed.\n");
        exit(EXIT_FAILURE);
    }

    if (endptr) {
        *endptr = (char*)str; // 设置剩余字符串的起始位置
    }

    return result;
}

string format_timestamp(long timestamp) {
    LOGD("format_timestamp called with timestamp: %ld", timestamp);
    LOGI("format_timestamp: called with timestamp=%ld", timestamp);

    if (timestamp <= 0) {
        LOGE("format_timestamp: invalid timestamp (<=0): %ld", timestamp);
        return "Invalid timestamp";
    }

    // 转换为本地时间
    time_t time = (time_t)timestamp;
    LOGD("format_timestamp: converted to time_t: %ld", time);

    struct tm* timeinfo = localtime(&time);
    if (timeinfo == nullptr) {
        LOGE("format_timestamp: localtime failed for timestamp=%ld", timestamp);
        return "Failed to convert time";
    }

    // 格式化时间
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 输出调试信息
    LOGD("format_timestamp: input=%ld, year=%d, month=%d, day=%d, hour=%d, min=%d, sec=%d",
         timestamp, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
         timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    LOGI("format_timestamp: returning formatted string: %s", buffer);
    return string(buffer);
}

bool string_end_with(const char *str, const char *suffix) {
    LOGD("string_end_with called");
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
    LOGD("string_start_with called");
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

string string_format(const char* format, ...) {
    va_list args;
    va_start(args, format);

    // 计算格式化后的字符串长度
    int size = vsnprintf(nullptr, 0, format, args);
    va_end(args);

    if (size <= 0) {
        return string();
    }

    // 分配内存（+1 结尾符）
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        return string();
    }

    va_start(args, format);
    vsnprintf(buffer, size + 1, format, args);
    va_end(args);

    // 使用 string 构造结果并释放内存
    string result(buffer);
    free(buffer);

    return result;
}

