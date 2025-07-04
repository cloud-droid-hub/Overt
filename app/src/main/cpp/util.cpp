//
// Created by lxz on 2025/6/6.
//

#include "util.h"

std::string get_line(int fd) {
    char buffer;
    std::string line = "";
    while (true) {
        ssize_t bytes_read = nonstd::read(fd, &buffer, sizeof(buffer));
        if (bytes_read == 0) break;
        line += buffer;
        if (buffer == '\n') break;
    }
    return line;
}

std::vector<std::string> get_file_lines(std::string path){
    std::vector<std::string> file_lines = std::vector<std::string>();
    int fd = nonstd::open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return file_lines;
    }
    while (true) {
        std::string line = get_line(fd);
        if (line.size() == 0) {
            break;
        }
        file_lines.push_back(line);
    }
    nonstd::close(fd);
    return file_lines;
}

// 分割字符串，返回字符串数组，分割算法用 c 来实现，不利用 api
std::vector<std::string> split_str(const std::string& str, const std::string& split) {
    std::vector<std::string> result;

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
        while (j <= str_len - split_len && std::memcmp(s + j, split.c_str(), split_len) != 0) {
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

std::vector<std::string> split_str(const std::string& str, char delim) {
    std::vector<std::string> result;
    const char* s = str.c_str();
    size_t start = 0;
    size_t len = str.length();

    for (size_t i = 0; i <= len; ++i) {
        if (s[i] == delim || s[i] == '\0') {
            result.emplace_back(s + start, i - start);
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