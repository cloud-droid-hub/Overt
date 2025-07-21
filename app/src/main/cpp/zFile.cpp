//
// Created by lxz on 2025/7/11.
//

#include "zFile.h"
#include "zLog.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>


// 构造函数
zFile::zFile() : m_path("") {
    LOGE("zFile()");
}

zFile::zFile(const string& path) : m_path(path) {
    LOGE("zFile(const string& path)");
    if(m_path.empty()) return;

    // 设置属性（文件/目录）
    setAttribute();

    // 设置文件描述符
    setFd();

}

void zFile::setAttribute(){

    st_ret = stat(m_path.c_str(), &st);

    if (st_ret != 0) return;

    earliest_time = st.st_mtim.tv_sec < st.st_ctim.tv_sec ? st.st_mtim.tv_sec : st.st_ctim.tv_sec;

    earliest_time = st.st_atim.tv_sec < earliest_time ? st.st_atim.tv_sec : earliest_time;

}

void zFile::setFd(){
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }

    if (m_path.empty()) return;

    int flag = isDir() ? (O_RDONLY | O_DIRECTORY) : O_RDONLY;

    m_fd = open(m_path.c_str(), flag);

    if (m_fd < 0) {
        LOGE("Failed to setFd %s: %s (errno=%d, %s)", isDir() ? "directory" : "file", m_path.c_str(), errno, strerror(errno));
    }else{
        LOGE("setFd succeed %s  %s %d", m_path.c_str(), isDir() ? "directory" : "file", m_fd);
    }
}


long zFile::getEarliestTime() const {
    return earliest_time;
}

// 析构函数
zFile::~zFile() {
    if (m_fd >= 0) {
        close(m_fd);
    }
}

string zFile::getPath() const {
    return m_path;
}

string zFile::getFileName() const {
    if (m_path.empty()) return "";

    size_t lastSlash = m_path.find_last_of("/\\");
    if (lastSlash == string::npos) {
        return m_path;
    }
    return m_path.substr(lastSlash + 1);
}

string zFile::getFileExtension() const {
    string fileName = getFileName();
    if (fileName.empty()) return "";

    size_t lastDot = fileName.find_last_of('.');
    if (lastDot == string::npos) {
        return "";
    }
    return fileName.substr(lastDot + 1);
}

string zFile::getDirectory() const {
    if (m_path.empty()) return "";

    size_t lastSlash = m_path.find_last_of("/\\");
    if (lastSlash == string::npos) {
        return "";
    }
    return m_path.substr(0, lastSlash);
}

bool zFile::pathEquals(string path) const {
    return m_path == path;
}

bool zFile::pathStartWith(string path) const {
    if (path.empty()) {
        return true;
    }
    if (m_path.length() < path.length()) {
        return false;
    }
    return m_path.substr(0, path.length()) == path;
}

bool zFile::pathEndWith(string path) const {
    if (path.empty()) {
        return true;
    }
    if (m_path.length() < path.length()) {
        return false;
    }
    return m_path.substr(m_path.length() - path.length()) == path;
}

bool zFile::fileNameEquals(string fileName) const {
    return getFileName() == fileName;
}

bool zFile::fileNameStartWith(string fileName) const {
    string currentFileName = getFileName();
    if (fileName.empty()) {
        return true;
    }
    if (currentFileName.length() < fileName.length()) {
        return false;
    }
    return currentFileName.substr(0, fileName.length()) == fileName;
}

bool zFile::fileNameEndWith(string fileName) const {
    string currentFileName = getFileName();
    if (fileName.empty()) {
        return true;
    }
    if (currentFileName.length() < fileName.length()) {
        return false;
    }
    return currentFileName.substr(currentFileName.length() - fileName.length()) == fileName;
}

// 文件存在性检查
bool zFile::exists() const {
    if(m_fd >= 0){
        return true;
    }
    if (access(m_path.c_str(), F_OK) != -1) {
        return true;
    }
    return false;
}

// 文件属性
long zFile::getFileSize(){

    setAttribute();

    setFd();

    if (isDir() || m_fd < 0) {
        return -1;
    }

    return st.st_size;
}

string zFile::getEarliestTimeFormatted() const {

    long timestamp = getEarliestTime();

    struct tm* timeinfo = localtime(&timestamp);
    if (!timeinfo) {
        return "Invalid";
    }

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return string(buffer);
}

// 文件读取操作
string zFile::readAllText() {
    LOGE("readAllText %d %d", isDir(), m_fd);
    if (isDir() || m_fd < 0) {
        LOGE("readAllText3");
        return "";
    }

    LOGE("readAllText2");
    // 检查是否为文本文件
    if (!isTextFile()) {
        LOGE("readAllText: 文件不是文本文件: %s", m_path.c_str());
        return "";
    }

    // 读取整个文件到内存
    vector<uint8_t> data = readBytes(0, getFileSize());

    // 将 vector<uint8_t> 转换为 string
    return string(data.begin(), data.end());
}

vector<string> zFile::readAllLines() {
    vector<string> lines;

    // 复用 readAllText 获取文件内容
    string content = readAllText();

    if (content.empty()) {
        return lines;
    }

    // 按行分割
    string current_line;
    for (char c : content) {
        if (c == '\n') {
            lines.push_back(current_line);
            current_line.clear();
        } else if (c == '\r') {
            // 忽略 \r，只处理 \n
            continue;
        } else {
            current_line += c;
        }
    }

    // 添加最后一行（如果没有换行符结尾）
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }

    return lines;
}

string zFile::readLine() {
    if (isDir() || m_fd < 0) {
        return "";
    }
    return getLine(m_fd);
}
vector<uint8_t> zFile::readBytes(long start_offset, size_t size) {
    vector<uint8_t> data;
    LOGE("readBytes start_offset: %ld, size: %zu", start_offset, size);
    if (isDir() || m_fd < 0) {
        return data;
    }

    // 保存当前位置
    off_t current_pos = lseek(m_fd, 0, SEEK_CUR);
    LOGE("当前偏移: %ld", current_pos);

    // 移动到指定偏移位置
    if (start_offset > 0) {
        lseek(m_fd, start_offset, SEEK_SET);
    }

    if (size == 0) {
        // size 为 0，读取全部
        LOGE("size 为 0，执行全量读取");

        char buffer[4096];
        ssize_t total_read = 0;

        while (true) {
            ssize_t bytesRead = read(m_fd, buffer, sizeof(buffer));
            if (bytesRead <= 0) break;

            data.insert(data.end(), buffer, buffer + bytesRead);
            total_read += bytesRead;

            LOGE("循环读取: 本次读取 %ld 字节, 总计 %ld 字节", bytesRead, total_read);
        }

        if (total_read == 0) {
            LOGE("读取失败或无数据");
        } else {
            LOGE("全量读取完成: 总计 %zu 字节", data.size());
        }

    } else {
        // size > 0，读取固定长度
        LOGE("读取指定大小: %zu 字节", size);
        data.resize(size);
        ssize_t bytesRead = read(m_fd, data.data(), size);
        if (bytesRead < 0) {
            perror("read");
            data.clear();
        } else if ((size_t)bytesRead < size) {
            data.resize(bytesRead);  // 只读到部分内容，缩小
            LOGE("部分读取: 实际读取 %zd 字节", bytesRead);
        } else {
            LOGE("成功读取 %zu 字节", size);
        }
    }

    // 恢复原始偏移
    lseek(m_fd, current_pos, SEEK_SET);
    return data;
}


vector<uint8_t> zFile::readAllBytes() {
    return readBytes(0, getFileSize());
}

// 目录操作
vector<string> zFile::listFiles() const {
    vector<string> files;
    
    if (!isDir()) {
        LOGE("listFiles: %s 不是目录", m_path.c_str());
        return files;
    }
    
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGE("listFiles: 无法打开目录 %s (errno: %d)", m_path.c_str(), errno);
        return files;
    }

    char link_real_path[PATH_MAX] = {0};
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        string fullPath = m_path + "/" + entry->d_name;

        if(entry->d_type == DT_LNK){
            ssize_t len =readlink(fullPath.c_str(), link_real_path, sizeof(link_real_path)-1);
            if (len > 0) {
                link_real_path[len] = '\0';
                files.emplace_back(link_real_path);
            }
        }else if (entry->d_type == DT_REG){\
            files.push_back(fullPath);
        }

    }
    
    closedir(dir);
    LOGE("listFiles: 在 %s 中找到 %zu 个文件", m_path.c_str(), files.size());
    return files;
}

vector<string> zFile::listDirectories() const {
    vector<string> dirs;
    
    if (!isDir()) {
        LOGE("listDirectories: %s 不是目录", m_path.c_str());
        return dirs;
    }
    
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGE("listDirectories: 无法打开目录 %s (errno: %d)", m_path.c_str(), errno);
        return dirs;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 只添加目录
        if (entry->d_type == DT_DIR) {
            dirs.push_back(entry->d_name);
        }
    }
    
    closedir(dir);
    LOGE("listDirectories: 在 %s 中找到 %zu 个目录", m_path.c_str(), dirs.size());
    return dirs;
}

vector<string> zFile::listAll() const {
    vector<string> all;
    
    if (!isDir()) {
        LOGE("listAll: %s 不是目录", m_path.c_str());
        return all;
    }
    
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGE("listAll: 无法打开目录 %s (errno: %d)", m_path.c_str(), errno);
        return all;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 添加所有条目（文件、目录、符号链接等）
        all.push_back(entry->d_name);
    }
    
    closedir(dir);
    LOGE("listAll: 在 %s 中找到 %zu 个条目", m_path.c_str(), all.size());
    return all;
}

// 文件格式检查
bool zFile::endsWith(const string& suffix) const {
    if (suffix.length() > m_path.length()) {
        return false;
    }
    return m_path.compare(m_path.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool zFile::startsWith(const string& prefix) const {
    if (prefix.length() > m_path.length()) {
        return false;
    }
    return m_path.compare(0, prefix.length(), prefix) == 0;
}

string zFile::getLine(int fd) const {
    char buffer;
    string line = "";
    int read_count = 0;
    const int max_read = 8192; // 最大读取8KB，防止无限循环

    while (read_count < max_read) {
        ssize_t bytes_read = read(fd, &buffer, sizeof(buffer));
        if (bytes_read <= 0) break;

        line += buffer;
        read_count++;

        if (buffer == '\n') break;
    }

    return line;
}

// UTF-8相关辅助方法
bool zFile::isValidUTF8Sequence(const char* data, int remaining_bytes) const {
    if (remaining_bytes <= 0) {
        return false;
    }
    
    unsigned char first_byte = static_cast<unsigned char>(data[0]);
    
    // 单字节UTF-8字符 (0xxxxxxx)
    if ((first_byte & 0x80) == 0) {
        return true;
    }
    
    // 多字节UTF-8字符
    int expected_length = getUTF8CharLength(first_byte);
    if (expected_length <= 0 || expected_length > remaining_bytes) {
        return false;
    }
    
    // 检查后续字节是否为10xxxxxx格式
    for (int i = 1; i < expected_length; i++) {
        unsigned char byte = static_cast<unsigned char>(data[i]);
        if ((byte & 0xC0) != 0x80) {
            return false;
        }
    }
    
    return true;
}

int zFile::getUTF8CharLength(unsigned char first_byte) const {
    // 单字节字符 (0xxxxxxx)
    if ((first_byte & 0x80) == 0) {
        return 1;
    }
    
    // 双字节字符 (110xxxxx)
    if ((first_byte & 0xE0) == 0xC0) {
        return 2;
    }
    
    // 三字节字符 (1110xxxx)
    if ((first_byte & 0xF0) == 0xE0) {
        return 3;
    }
    
    // 四字节字符 (11110xxx)
    if ((first_byte & 0xF8) == 0xF0) {
        return 4;
    }
    
    // 无效的UTF-8起始字节
    return -1;
}

bool zFile::isTextFile() {
    return true;
    
    if (m_fd < 0) {
        return false;
    }

    // 复用 readBytes 读取文件前1KB来检查
    vector<uint8_t> data = readBytes(0, 1024);
    
    LOGE("isTextFile: 读取了 %zu 字节", data.size());

    if (data.empty()) {
        LOGE("isTextFile: 文件为空");
        return false;
    }

    // 检查是否为ASCII或UTF-8文本
    for (size_t i = 0; i < data.size(); i++) {
        unsigned char c = data[i];
        
        // 检查是否为ASCII可打印字符、制表符、换行符、回车符
        if (c >= 0x20 && c <= 0x7E) {
            // ASCII可打印字符
            continue;
        } else if (c == 0x09 || c == 0x0A || c == 0x0D) {
            // 制表符(\t)、换行符(\n)、回车符(\r)
            continue;
        } else if (c == 0x00) {
            // 遇到null字符，认为是二进制文件
            return false;
        } else if (c >= 0x80) {
            // 可能是UTF-8字符，检查UTF-8编码
            if (!isValidUTF8Sequence(reinterpret_cast<const char*>(&data[i]), data.size() - i)) {
                return false;
            }
            // 跳过UTF-8字符的其他字节
            int utf8_len = getUTF8CharLength(c);
            if (utf8_len > 0 && i + utf8_len <= data.size()) {
                i += utf8_len - 1; // -1是因为循环会自增
            } else {
                return false; // UTF-8字符不完整
            }
        } else {
            // 其他控制字符，认为是二进制文件
            LOGE("isTextFile: 发现非ASCII可打印字符: %02X", c);
            return false;
        }
    }
    return true;
}

unsigned long zFile::getSum(long start_offset, size_t size){
    unsigned long sum = 0;
    LOGE("111111111111 %d", m_fd);
    vector<uint8_t> data = readBytes(start_offset, size);
    LOGE("222222222222 %d", data.size());
    for(int i = 0; i < data.size(); i++){
        sum += data[i];
    }
    return sum;
}

unsigned long zFile::getSum(){
    return getSum(0, getFileSize());
}