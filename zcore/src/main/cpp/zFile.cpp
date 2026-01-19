//
// Created by lxz on 2025/7/11.
//

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cctype>
#include <errno.h>
#include <fcntl.h>

#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zFile.h"

/**
 * 默认构造函数
 * 初始化空的文件对象
 */
zFile::zFile() : m_path(""), m_fd(-1), earliest_time(0), st_ret(-1) {
    LOGD("Default constructor called");
}

/**
 * 带字符串路径的构造函数
 * 根据指定路径初始化文件对象，并设置文件属性
 * @param string 文件或目录的路径
 */
zFile::zFile(const char *string) : m_path(string) {
    LOGD("Default constructor called");
    if(m_path.empty()) {
        LOGD("Constructed with empty path");
        return;
    }

    // 设置文件属性（文件/目录）
    setAttribute();

    // 设置文件描述符
    setFd();
}

/**
 * 带路径的构造函数
 * 根据指定路径初始化文件对象，并设置文件属性
 * @param path 文件或目录的路径
 */
zFile::zFile(const string& path) : m_path(path) {
    LOGD("Constructor called with path: %s", path.c_str());
    if(m_path.empty()) {
        LOGD("Constructed with empty path");
        return;
    }

    // 设置文件属性（文件/目录）
    setAttribute();

    // 设置文件描述符
    setFd();

    // 验证文件是否成功打开
    if (m_fd < 0) {
        LOGW("Failed to open file: %s", m_path.c_str());
    }
}

/**
 * 带数据向量的构造函数
 * 根据字节数据初始化文件对象，数据存储在内存中
 * @param data 文件数据字节向量
 */
zFile::zFile(vector<uint8_t> data) : m_path(""), m_fd(-1), earliest_time(0), st_ret(-1), m_data(data) {
    LOGD("Constructor called with data vector, size: %zu", data.size());
}

/**
 * 设置文件路径
 * 设置新的文件路径，会先清理之前的文件描述符
 * @param path 新的文件路径
 */
void zFile::setPath(const string& path) {
    LOGD("setPath called with path: %s", path.c_str());

    // 如果路径没有变化，直接返回
    if (m_path == path) {
        LOGD("Path unchanged, skipping update");
        return;
    }

    // 清理之前的文件描述符
    if (m_fd > 2) {
        LOGD("Closing previous fd: %d", m_fd);
        if (close(m_fd) != 0) {
            LOGW("Failed to close fd %d: %s", m_fd, strerror(errno));
        }
        m_fd = -1;
    } else if (m_fd >= 0) {
        LOGE("setPath: WARNING - Skipping close of standard file descriptor %d (stdin/stdout/stderr)", m_fd);
        LOGE("setPath: This prevents conflict with Android's unique_fd management");
        m_fd = -1;
    }

    // 设置新路径
    m_path = path;

    // 如果新路径为空，重置其他状态
    if (m_path.empty()) {
        LOGD("setPath: Path set to empty, resetting state");
        earliest_time = 0;
        st_ret = -1;
        return;
    }

    // 设置文件属性（文件/目录）
    setAttribute();

    // 设置文件描述符
    setFd();

    // 验证文件是否成功打开
    if (m_fd < 0) {
        LOGW("setPath: Failed to open file: %s", m_path.c_str());
    } else {
        // 检查文件描述符是否在标准范围内（0-2）
        if (m_fd <= 2) {
            LOGE("setPath: WARNING - File descriptor %d is in standard range (0-2) for %s", m_fd, m_path.c_str());
            LOGE("setPath: This may cause issues with Android's unique_fd management");
        }
        LOGI("setPath: File opened successfully with fd %d", m_fd);
    }
}

/**
 * 设置文件属性
 * 获取文件的stat信息，包括文件大小、时间戳等
 */
void zFile::setAttribute(){
    LOGD("setAttribute called for path: %s", m_path.c_str());

    // 获取文件状态信息
    st_ret = stat(m_path.c_str(), &st);
    if (st_ret != 0) {
        LOGD("stat failed for path: %s: %s", m_path.c_str(), strerror(errno));
        earliest_time = 0;  // 重置时间戳
    }else{
        // 计算最早时间戳（修改时间、创建时间、访问时间中的最小值）
        earliest_time = st.st_mtim.tv_sec < st.st_ctim.tv_sec ? st.st_mtim.tv_sec : st.st_ctim.tv_sec;
        earliest_time = st.st_atim.tv_sec < earliest_time ? st.st_atim.tv_sec : earliest_time;
    }

    stfs_ret = statfs64(m_path.c_str(), &stfs);
    if (stfs_ret != 0) {
        LOGD("stfs_ret failed for path: %s: %s", m_path.c_str(), strerror(errno));
    }

}


/**
 * 设置文件描述符
 * 根据文件类型（文件或目录）打开相应的文件描述符
 */
void zFile::setFd(){
    LOGD("setFd called for path: %s", m_path.c_str());

    // 如果已有文件描述符，先关闭
    if (m_fd > 2) {
        LOGD("Closing previous fd: %d", m_fd);
        if (close(m_fd) != 0) {
            LOGW("Failed to close fd %d: %s", m_fd, strerror(errno));
        }
        m_fd = -1;
    } else if (m_fd >= 0) {
        LOGE("setFd: WARNING - Skipping close of standard file descriptor %d (stdin/stdout/stderr)", m_fd);
        LOGE("setFd: This prevents conflict with Android's unique_fd management");
        m_fd = -1;
    }

    if (m_path.empty()) {
        LOGD("setFd: path is empty");
        return;
    }

    // 根据文件类型设置打开标志
    int flag = isDir() ? (O_RDONLY | O_DIRECTORY) : O_RDONLY;

    // 打开文件或目录
    m_fd = open(m_path.c_str(), flag, O_CREAT);

    if (m_fd < 0) {
        LOGW("setFd failed for %s: %s (errno=%d)", m_path.c_str(), isDir() ? "directory" : "file", errno);
    } else {
        // 检查文件描述符是否在标准范围内（0-2）
        if (m_fd <= 2) {
            LOGE("setFd: WARNING - File descriptor %d is in standard range (0-2) for %s", m_fd, m_path.c_str());
            LOGE("setFd: This may cause issues with Android's unique_fd management");
        }
        LOGI("setFd succeed for %s, fd=%d", m_path.c_str(), m_fd);
    }
}

/**
 * 获取最早时间戳
 * @return 文件的最早时间戳（秒）
 */
long zFile::getEarliestTime() const {
    LOGD("getEarliestTime called for path: %s", m_path.c_str());
    return earliest_time;
}

/**
 * 析构函数
 * 清理资源，关闭文件描述符
 */
zFile::~zFile() {
    LOGD("Destructor called for path: %s", m_path.c_str());
    if (m_fd > 2) {
        LOGD("Closing fd in destructor: %d", m_fd);
        if (close(m_fd) != 0) {
            LOGW("Failed to close fd %d in destructor: %s", m_fd, strerror(errno));
        }
        m_fd = -1;
    } else if (m_fd >= 0) {
        LOGE("zFile::~zFile: WARNING - Skipping close of standard file descriptor %d (stdin/stdout/stderr)", m_fd);
        LOGE("zFile::~zFile: This prevents conflict with Android's unique_fd management");
        m_fd = -1;
    }
}

/**
 * 获取文件路径
 * @return 文件的完整路径
 */
string zFile::getPath() const {
    LOGD("getPath called");
    return m_path;
}

/**
 * 获取文件名
 * 从完整路径中提取文件名部分
 * @return 文件名（不包含路径）
 */
string zFile::getFileName() const {
    LOGD("getFileName called for path: %s", m_path.c_str());
    if (m_path.empty()) return "";

    // 查找最后一个斜杠位置
    size_t lastSlash = m_path.find_last_of("/\\");
    if (lastSlash == string::npos) {
        return m_path;
    }
    return m_path.substr(lastSlash + 1);
}

/**
 * 获取文件扩展名
 * 从文件名中提取扩展名部分
 * @return 文件扩展名（不包含点号）
 */
string zFile::getFileExtension() const {
    string fileName = getFileName();
    if (fileName.empty()) return "";

    // 查找最后一个点号位置
    size_t lastDot = fileName.find_last_of('.');
    if (lastDot == string::npos) {
        return "";
    }
    return fileName.substr(lastDot + 1);
}

/**
 * 获取目录路径
 * 从完整路径中提取目录部分
 * @return 目录路径（不包含文件名）
 */
string zFile::getDirectory() const {
    if (m_path.empty()) return "";

    // 查找最后一个斜杠位置
    size_t lastSlash = m_path.find_last_of("/\\");
    if (lastSlash == string::npos) {
        return "";
    }
    return m_path.substr(0, lastSlash);
}

/**
 * 路径相等比较
 * @param path 要比较的路径
 * @return 如果路径相等返回true
 */
bool zFile::pathEquals(string path) const {
    return m_path == path;
}

/**
 * 路径前缀比较
 * @param path 要比较的前缀路径
 * @return 如果当前路径以指定路径开头返回true
 */
bool zFile::pathStartWith(string path) const {
    if (path.empty()) {
        return true;
    }
    if (m_path.length() < path.length()) {
        return false;
    }
    return m_path.substr(0, path.length()) == path;
}

/**
 * 路径后缀比较
 * @param path 要比较的后缀路径
 * @return 如果当前路径以指定路径结尾返回true
 */
bool zFile::pathEndWith(string path) const {
    if (path.empty()) {
        return true;
    }
    if (m_path.length() < path.length()) {
        return false;
    }
    return m_path.substr(m_path.length() - path.length()) == path;
}

/**
 * 文件名相等比较
 * @param fileName 要比较的文件名
 * @return 如果文件名相等返回true
 */
bool zFile::fileNameEquals(string fileName) const {
    return getFileName() == fileName;
}

/**
 * 文件名前缀比较
 * @param fileName 要比较的文件名前缀
 * @return 如果当前文件名以指定前缀开头返回true
 */
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

/**
 * 文件名后缀比较
 * @param fileName 要比较的文件名后缀
 * @return 如果当前文件名以指定后缀结尾返回true
 */
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

/**
 * 文件存在性检查
 * 检查文件或目录是否存在
 * @return 如果文件存在返回true
 */
bool zFile::exists() const {
    // 如果文件描述符有效，说明文件存在
    if(m_fd >= 0){
        return true;
    }

    // 使用access函数检查文件是否存在
    if (access(m_path.c_str(), F_OK) != -1) {
        return true;
    }
    return false;
}

/**
 * 获取文件大小
 * 重新获取文件属性并返回文件大小
 * @return 文件大小（字节），目录或失败时返回-1
 */
long zFile::getFileSize(){
    // 重新获取文件属性
    setAttribute();
    // 重新设置文件描述符
    setFd();

    if (isDir() || m_fd < 0) {
        return -1;
    }

    return st.st_size;
}

/**
 * 获取格式化的最早时间戳
 * 将时间戳转换为可读的日期时间格式
 * @return 格式化的时间字符串（YYYY-MM-DD HH:MM:SS）
 */
string zFile::getEarliestTimeFormatted() const {
    long timestamp = getEarliestTime();

    // 将时间戳转换为本地时间
    struct tm* timeinfo = localtime(&timestamp);
    if (!timeinfo) {
        return "Invalid";
    }

    // 格式化时间字符串
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return string(buffer);
}

/**
 * 读取文件全部文本内容
 * 将文件内容作为文本读取，自动处理编码
 * @return 文件内容字符串，失败时返回空字符串
 */
string zFile::readAllText() {
    LOGD("readAllText %d %d", isDir(), m_fd);
    if (isDir() || m_fd < 0) {
        LOGD("readAllText isDir or m_fd<0");
        return "";
    }

    // 检查是否为文本文件
    if (!isTextFile()) {
        LOGW("readAllText: 文件不是文本文件: %s", m_path.c_str());
        return "";
    }

    // 读取整个文件到内存
    vector<uint8_t> data = readBytes(0, getFileSize());

    // 将 vector<uint8_t> 转换为 string
    return string(data.begin(), data.end());
}

/**
 * 读取文件所有行
 * 将文件内容按行分割并返回行列表
 * @return 文件行列表
 */
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

/**
 * 读取一行
 * 从当前文件位置读取一行内容
 * @return 一行内容，失败时返回空字符串
 */
string zFile::readLine() {
    if (isDir() || m_fd < 0) {
        return "";
    }
    return getLine(m_fd);
}

/**
 * 读取指定字节数据
 * 从指定偏移位置读取指定大小的字节数据
 * @param start_offset 起始偏移位置
 * @param size 要读取的字节数，0表示读取全部
 * @return 读取的字节数据
 */
vector<uint8_t> zFile::readBytes(long start_offset, size_t size) {
    LOGD("readBytes called with start_offset: %ld, size: %zu", start_offset, size);
    vector<uint8_t> data;

    if (isDir() || m_fd < 0) {
        LOGD("readBytes: file is directory or fd invalid");
        return data;
    }

    // 保存当前位置
    off_t current_pos = lseek(m_fd, 0, SEEK_CUR);
    if (current_pos == -1) {
        LOGW("Failed to get current position: %s", strerror(errno));
        return data;
    }

    // 移动到指定偏移位置
    if (start_offset > 0) {
        if (lseek(m_fd, start_offset, SEEK_SET) == -1) {
            LOGW("Failed to seek to offset %ld: %s", start_offset, strerror(errno));
            lseek(m_fd, current_pos, SEEK_SET);  // 恢复位置
            return data;
        }
    }

    if (size == 0) {
        // size 为 0，读取全部
        char buffer[4096];
        ssize_t total_read = 0;

        while (true) {
            ssize_t bytesRead = read(m_fd, buffer, sizeof(buffer));
            if (bytesRead < 0) {
                LOGW("Read error: %s", strerror(errno));
                break;
            } else if (bytesRead == 0) {
                // EOF
                break;
            }

            data.insert(data.end(), buffer, buffer + bytesRead);
            total_read += bytesRead;
        }

        if (total_read == 0) {
            LOGD("readBytes: no data read");
        } else {
            LOGI("readBytes: read %zu bytes successfully", data.size());
        }

    } else {
        // size > 0，读取固定长度
        data.resize(size);
        ssize_t bytesRead = read(m_fd, data.data(), size);
        if (bytesRead < 0) {
            LOGW("Read error: %s", strerror(errno));
            data.clear();
        } else if ((size_t)bytesRead < size) {
            data.resize(bytesRead);  // 只读到部分内容，缩小
            LOGD("Partial read: actual read %zd bytes", bytesRead);
        } else {
            LOGI("readBytes: read %zu bytes successfully", size);
        }
    }

    // 恢复原始偏移
    if (lseek(m_fd, current_pos, SEEK_SET) == -1) {
        LOGW("Failed to restore position: %s", strerror(errno));
    }

    return data;
}

/**
 * 读取文件全部字节
 * @return 文件的全部字节数据
 */
vector<uint8_t> zFile::readAllBytes() {
    return readBytes(0, getFileSize());
}

/**
 * 列出目录中的所有文件
 * 获取目录中所有文件的完整路径
 * @return 文件路径列表
 */
vector<string> zFile::listFiles() const {
    LOGD("listFiles called for path: %s", m_path.c_str());
    vector<string> files;

    if (!isDir()) {
        LOGD("listFiles: %s is not a directory", m_path.c_str());
        return files;
    }

    // 打开目录
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGW("listFiles: failed to open directory %s (errno: %d)", m_path.c_str(), errno);
        return files;
    }

    char link_real_path[PATH_MAX] = {0};
    struct dirent* entry;

    // 遍历目录条目
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        string fullPath = m_path + "/" + entry->d_name;

        // 处理符号链接
        if(entry->d_type == DT_LNK){
            ssize_t len =readlink(fullPath.c_str(), link_real_path, sizeof(link_real_path)-1);
            if (len > 0) {
                link_real_path[len] = '\0';
                files.emplace_back(link_real_path);
            }
        }
            // 处理普通文件
        else if (entry->d_type == DT_REG){
            files.push_back(fullPath);
        }
    }

    closedir(dir);
    LOGI("listFiles: found %zu files in %s", files.size(), m_path.c_str());
    return files;
}

/**
 * 列出目录中的所有子目录
 * 获取目录中所有子目录的名称
 * @return 子目录名称列表
 */
vector<string> zFile::listDirectories() const {
    LOGD("listDirectories called for path: %s", m_path.c_str());
    vector<string> dirs;

    if (!isDir()) {
        LOGD("listDirectories: %s is not a directory", m_path.c_str());
        return dirs;
    }

    // 打开目录
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGW("listDirectories: failed to open directory %s (errno: %d)", m_path.c_str(), errno);
        return dirs;
    }

    struct dirent* entry;

    // 遍历目录条目
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
    LOGI("listDirectories: found %zu directories in %s", dirs.size(), m_path.c_str());
    return dirs;
}

/**
 * 列出目录中的所有条目
 * 获取目录中所有条目的名称（包括文件、目录、符号链接等）
 * @return 所有条目名称列表
 */
vector<string> zFile::listAll() const {
    LOGD("listAll called for path: %s", m_path.c_str());
    vector<string> all;

    if (!isDir()) {
        LOGD("listAll: %s is not a directory", m_path.c_str());
        return all;
    }

    // 打开目录
    DIR* dir = opendir(m_path.c_str());
    if (!dir) {
        LOGW("listAll: failed to open directory %s (errno: %d)", m_path.c_str(), errno);
        return all;
    }

    struct dirent* entry;

    // 遍历目录条目
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 添加所有条目（文件、目录、符号链接等）
        all.push_back(entry->d_name);
    }

    closedir(dir);
    LOGI("listAll: found %zu items in %s", all.size(), m_path.c_str());
    return all;
}

/**
 * 检查路径是否以指定后缀结尾
 * @param suffix 要检查的后缀
 * @return 如果路径以指定后缀结尾返回true
 */
bool zFile::endsWith(const string& suffix) const {
    if (suffix.length() > m_path.length()) {
        return false;
    }
    return m_path.compare(m_path.length() - suffix.length(), suffix.length(), suffix) == 0;
}

/**
 * 检查路径是否以指定前缀开头
 * @param prefix 要检查的前缀
 * @return 如果路径以指定前缀开头返回true
 */
bool zFile::startsWith(const string& prefix) const {
    if (prefix.length() > m_path.length()) {
        return false;
    }
    return m_path.compare(0, prefix.length(), prefix) == 0;
}

/**
 * 从文件描述符读取一行
 * 读取直到遇到换行符或文件结束
 * @param fd 文件描述符
 * @return 读取的一行内容
 */
string zFile::getLine(int fd) const {
    char buffer;
    string line = "";
    int read_count = 0;
    const int max_read = 8192; // 最大读取8KB，防止无限循环

    // 逐字节读取直到遇到换行符
    while (read_count < max_read) {
        ssize_t bytes_read = read(fd, &buffer, sizeof(buffer));
        if (bytes_read <= 0) break;

        line += buffer;
        read_count++;

        if (buffer == '\n') break;
    }

    return line;
}

/**
 * 检查UTF-8序列是否有效
 * 验证多字节UTF-8字符的编码是否正确
 * @param data 数据指针
 * @param remaining_bytes 剩余字节数
 * @return 如果UTF-8序列有效返回true
 */
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

/**
 * 获取UTF-8字符长度
 * 根据UTF-8字符的第一个字节确定字符长度
 * @param first_byte UTF-8字符的第一个字节
 * @return UTF-8字符的字节数，无效字符返回-1
 */
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

/**
 * 检查文件是否为文本文件
 * 通过分析文件内容的前1KB来判断文件类型
 * @return 如果文件是文本文件返回true
 */
bool zFile::isTextFile() {
    // 这个函数实现起来有问题
    return true;

    if (m_fd < 0) {
        return false;
    }

    // 复用 readBytes 读取文件前1KB来检查
    vector<uint8_t> data = readBytes(0, 1024);

    LOGD("isTextFile: 读取了 %zu 字节", data.size());

    if (data.empty()) {
        LOGD("isTextFile: 文件为空");
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
            LOGD("isTextFile: 发现非ASCII可打印字符: %02X", c);
            return false;
        }
    }
    return true;
}

/**
 * 计算文件指定范围的字节和
 * 读取指定范围的字节并计算其总和
 * @param start_offset 起始偏移位置
 * @param size 要计算的字节数
 * @return 字节和
 */
unsigned long zFile::getSum(long start_offset, size_t size){
    LOGD("getSum called with start_offset: %ld, size: %zu", start_offset, size);
    unsigned long sum = 0;

    // 读取指定范围的字节数据
    vector<uint8_t> data = readBytes(start_offset, size);
    LOGD("getSum: read %zu bytes", data.size());

    // 计算字节和
    for(int i = 0; i < data.size(); i++){
        sum += data[i];
    }
    LOGI("getSum: calculated sum = %lu", sum);
    return sum;
}

/**
 * 计算整个文件的字节和
 * @return 整个文件的字节和
 */
unsigned long zFile::getSum(){
    LOGD("getSum called for entire file");
    return getSum(0, getFileSize());
}

/**
 * 保存文件
 * 将内存中的数据保存到指定路径
 * @param path 保存路径
 * @return 是否成功
 */
bool zFile::saveByData(string path) {
    LOGD("saveByData called with path: %s", path.c_str());
    
    // 检查内存数据是否为空
    if (m_data.empty()) {
        LOGW("saveByData: m_data is empty");
        return false;
    }
    
    // 检查目标路径的目录是否存在
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != string::npos) {
        string dir = path.substr(0, lastSlash);
        if (!dir.empty() && access(dir.c_str(), F_OK) != 0) {
            LOGW("saveByData: target directory does not exist: %s", dir.c_str());
            // 注意：这里不返回错误，让 open 函数处理，因为某些系统可能允许创建文件
        }
    }
    
    // 打开目标文件进行写入
    int target_fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (target_fd < 0) {
        LOGW("saveByData: failed to open target file %s: %s", path.c_str(), strerror(errno));
        return false;
    }
    
    // 写入数据
    ssize_t bytes_written = write(target_fd, m_data.data(), m_data.size());
    if (bytes_written < 0 || (size_t)bytes_written != m_data.size()) {
        LOGW("saveByData: failed to write data to %s: %s", path.c_str(), strerror(errno));
        close(target_fd);
        return false;
    }
    
    // 确保数据写入磁盘
    if (fsync(target_fd) != 0) {
        LOGW("saveByData: failed to sync file %s: %s", path.c_str(), strerror(errno));
    }
    
    close(target_fd);
    LOGI("saveByData: successfully saved %zu bytes to %s", m_data.size(), path.c_str());
    return true;
}