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
zFile::zFile() : m_path(""), m_fd(-1), m_is_directory(false) {
}

zFile::zFile(const std::string& path) : m_path(path), m_fd(-1), m_is_directory(false) {
    if (!m_path.empty()) {
        // 先检查是否为目录
        m_is_directory = checkIsDirectory();
        if (m_is_directory) {
            LOGE("路径是目录: %s", m_path.c_str());
            openDirectory();
        } else {
            LOGE("路径是文件: %s", m_path.c_str());
            openFile();
        }

        earliest_time = getEarliestTime();
    }
}

// 析构函数
zFile::~zFile() {
    if (m_fd >= 0) {
        close(m_fd);
    }
}

// 文件路径相关
void zFile::setPath(const std::string& path) {
    // 关闭当前文件
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    
    m_path = path;
    m_is_directory = false;
    
    // 检查新路径
    if (!m_path.empty()) {
        m_is_directory = checkIsDirectory();
        
        if (m_is_directory) {
            LOGE("路径是目录: %s", m_path.c_str());
            openDirectory();
        } else {
            openFile();
        }
    }
}

std::string zFile::getPath() const {
    return m_path;
}

std::string zFile::getFileName() const {
    if (m_path.empty()) return "";

    size_t lastSlash = m_path.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return m_path;
    }
    return m_path.substr(lastSlash + 1);
}

std::string zFile::getFileExtension() const {
    std::string fileName = getFileName();
    if (fileName.empty()) return "";

    size_t lastDot = fileName.find_last_of('.');
    if (lastDot == std::string::npos) {
        return "";
    }
    return fileName.substr(lastDot + 1);
}

std::string zFile::getDirectory() const {
    if (m_path.empty()) return "";

    size_t lastSlash = m_path.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return "";
    }
    return m_path.substr(0, lastSlash);
}

// 文件存在性检查
bool zFile::exists() const {
    if (m_is_directory) {
        return true; // 目录存在
    }
    return m_fd >= 0; // 文件存在
}

bool zFile::isFile() const {
    return !m_is_directory && m_fd >= 0;
}

bool zFile::isDirectory() const {
    return m_is_directory;
}

bool zFile::isOpen() const {
    return m_is_directory || m_fd >= 0;
}

// 文件属性
long zFile::getFileSize() const {
    if (m_is_directory || m_fd < 0) {
        return -1;
    }
    
    struct stat st;
    if (fstat(m_fd, &st) != 0) {
        LOGE("getFileSize: fstat failed, errno=%d", errno);
        return -1;
    }
    LOGE("getFileSize: 文件大小=%ld", st.st_size);
    return st.st_size;
}

time_t zFile::getLastModifiedTime() const {
    if (m_is_directory || m_fd < 0) {
        return 0;
    }
    
    struct stat st;
    if (fstat(m_fd, &st) != 0) {
        return 0;
    }
    return st.st_mtime;
}

long zFile::getEarliestTime() const{
    long earliest_time = 0;

    if (m_path.empty()) {
        LOGE("getEarliestTime: 路径为空");
        return earliest_time;
    }
    
    // 对于目录，使用 stat() 而不是 fstat()
    struct stat stx;
    int stat_result;
    
    if (m_is_directory) {
        stat_result = stat(m_path.c_str(), &stx);
    } else if (m_fd >= 0) {
        stat_result = fstat(m_fd, &stx);
    } else {
        LOGE("getEarliestTime: 文件未打开且不是目录 %s", m_path.c_str());
        return 0;
    }
    
    if (stat_result != 0) {
        LOGE("getEarliestTime: stat failed, errno=%d, path=%s", errno, m_path.c_str());
        return 0;
    }

    earliest_time = stx.st_mtim.tv_sec < stx.st_ctim.tv_sec ? stx.st_mtim.tv_sec : stx.st_ctim.tv_sec;

    earliest_time = stx.st_atim.tv_sec < earliest_time ? stx.st_atim.tv_sec : earliest_time;

    return earliest_time;
}

std::string zFile::getEarliestTimeFormatted() const {

    long timestamp = getEarliestTime();
    
    struct tm* timeinfo = localtime(&timestamp);
    if (!timeinfo) {
        return "Invalid";
    }
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

// 文件读取操作
std::string zFile::readAllText() {
    if (m_is_directory || m_fd < 0) {
        return "";
    }

    // 检查是否为文本文件
    if (!isTextFile()) {
        LOGE("readAllText: 文件不是文本文件: %s", m_path.c_str());
        return "";
    }

    // 读取整个文件到内存
    std::vector<uint8_t> data = readBytes(0, getFileSize());
    
    // 将 vector<uint8_t> 转换为 string
    return std::string(data.begin(), data.end());
}

std::vector<std::string> zFile::readAllLines() {
    std::vector<std::string> lines;
    
    // 复用 readAllText 获取文件内容
    std::string content = readAllText();
    
    if (content.empty()) {
        return lines;
    }

    // 按行分割
    std::string current_line;
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

std::string zFile::readLine() {
    if (m_is_directory || m_fd < 0) {
        return "";
    }
    return getLine(m_fd);
}

std::vector<uint8_t> zFile::readBytes(long start_offset, size_t size){
    std::vector<uint8_t> data;

    if (m_is_directory || m_fd < 0) {
        return data;
    }

    // 保存当前位置
    off_t current_pos = lseek(m_fd, 0, SEEK_CUR);

    if(getDevice() == 4){
        LOGE("设备为 4 伪内核文件, 需要特殊处理");

        // 移动到指定偏移位置
        if (start_offset > 0) {
            lseek(m_fd, start_offset, SEEK_SET);
        }

        char buffer[4096];
        ssize_t total_read = 0;
        size_t max_read = (size == 0) ? SIZE_MAX : size; // 如果size为0，读取所有数据
        
        while (total_read < max_read) {
            size_t read_size = std::min(sizeof(buffer), max_read - total_read);
            ssize_t bytesRead = read(m_fd, buffer, read_size);
            
            if (bytesRead <= 0) {
                break;
            }
            
            // 将读取的数据添加到返回vector中
            data.insert(data.end(), buffer, buffer + bytesRead);
            total_read += bytesRead;
            
            LOGE("伪内核文件读取: 本次读取 %ld 字节, 总计 %ld 字节", bytesRead, total_read);
        }

        if (total_read == 0) {
            LOGE("伪内核文件读取失败或无数据");
        } else {
            LOGE("伪内核文件读取完成: 总计 %zu 字节", data.size());
        }

    }else{
        // 移动到文件开头
        lseek(m_fd, start_offset, SEEK_SET);

        struct stat st;
        if (fstat(m_fd, &st) == 0) {
            size = st.st_size < size ? st.st_size : size;
            data.resize(size);
            ssize_t bytesRead = read(m_fd, data.data(), size);
            if (bytesRead != size) {
                data.clear();
            }
        }
    }

    // 恢复位置
    lseek(m_fd, current_pos, SEEK_SET);

    return data;
}


std::vector<uint8_t> zFile::readAllBytes() {
    return readBytes(0, getFileSize());
}

std::vector<std::string> zFile::listFiles() const {
    std::vector<std::string> files;
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGE("Failed to open directory: %s (errno: %d)", m_path.c_str(), errno);
        return files;
    }

    struct dirent* entry;
    int total_entries = 0;
    int file_entries = 0;
    while ((entry = readdir(dir)) != nullptr) {
        total_entries++;
        LOGE("Found entry: %s (type: %d)", entry->d_name, entry->d_type);
        if (entry->d_type == DT_REG) { // 普通文件
            files.push_back(entry->d_name);
            file_entries++;
        }
    }
    LOGE("Total entries: %d, Files: %d", total_entries, file_entries);
    closedir(dir);
    return files;
}

std::vector<std::string> zFile::listDirectories() const {
    std::vector<std::string> dirs;
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGE("Failed to open directory: %s (errno: %d)", m_path.c_str(), errno);
        return dirs;
    }

    struct dirent* entry;
    int dir_entries = 0;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR && 
            strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0) {
            dirs.push_back(entry->d_name);
            dir_entries++;
        }
    }
    LOGE("Directory entries: %d", dir_entries);
    closedir(dir);
    return dirs;
}

std::vector<std::string> zFile::listAll() const {
    std::vector<std::string> all;
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGE("Failed to open directory: %s (errno: %d)", m_path.c_str(), errno);
        return all;
    }

    struct dirent* entry;
    int all_entries = 0;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            all.push_back(entry->d_name);
            all_entries++;
        }
    }
    LOGE("All entries (excluding . and ..): %d", all_entries);
    closedir(dir);
    return all;
}



// 文件格式检查
bool zFile::endsWith(const std::string& suffix) const {
    if (suffix.length() > m_path.length()) {
        return false;
    }
    return m_path.compare(m_path.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool zFile::startsWith(const std::string& prefix) const {
    if (prefix.length() > m_path.length()) {
        return false;
    }
    return m_path.compare(0, prefix.length(), prefix) == 0;
}


// 私有辅助方法
void zFile::openFile() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    
    if (m_path.empty()) {
        return;
    }
    
    m_fd = open(m_path.c_str(), O_RDONLY);
    if (m_fd < 0) {
        LOGE("Failed to open file: %s", m_path.c_str());
    }
}

// 私有辅助方法
void zFile::openDirectory() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }

    if (m_path.empty()) {
        return;
    }

    m_fd = open(m_path.c_str(), O_RDONLY | O_DIRECTORY);
    if (m_fd < 0) {
        LOGE("Failed to openDirectory file: %s", m_path.c_str());
    }
}

void zFile::closeFile() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
}

std::string zFile::getLine(int fd) const {
    char buffer;
    std::string line = "";
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

bool zFile::checkAccess(int mode) const {
    return ::access(m_path.c_str(), mode) == 0;
}

bool zFile::checkIsDirectory() const {
    struct stat st;
    if (stat(m_path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
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
    if (m_fd < 0) {
        return false;
    }

    // 复用 readBytes 读取文件前1KB来检查
    std::vector<uint8_t> data = readBytes(0, 1024);
    
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
    std::vector<uint8_t> data = readBytes(start_offset, size);
    for(int i = 0; i < data.size(); i++){
        sum += data[i];
    }
    return sum;
}

unsigned long zFile::getSum(){
    return getSum(0, getFileSize());
}

unsigned long zFile::getDevice(){
    struct stat st;
    if (stat(m_path.c_str(), &st) == 0) {
        LOGE("getDevice: Device=%lx, Mode=%o, Size=%ld, Path=%s", 
             (unsigned long)st.st_dev, st.st_mode, st.st_size, m_path.c_str());
        return st.st_dev;
    }
    LOGE("getDevice: stat failed for path=%s, errno=%d", m_path.c_str(), errno);
    return 0;
}