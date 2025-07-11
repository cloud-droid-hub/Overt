//
// Created by lxz on 2025/7/11.
//

#ifndef OVERT_ZFILE_H
#define OVERT_ZFILE_H

#include <string>
#include <vector>
#include <map>

class zFile {
public:
    // 构造函数
    zFile();
    explicit zFile(const std::string& path);
    
    // 析构函数
    ~zFile();
    
    // 文件路径相关
    void setPath(const std::string& path);
    std::string getPath() const;
    std::string getFileName() const;
    std::string getFileExtension() const;
    std::string getDirectory() const;
    
    // 文件存在性检查
    bool exists() const;
    bool isFile() const;
    bool isDirectory() const;
    bool isOpen() const;
    
    // 文件属性
    long getFileSize() const;
    time_t getLastModifiedTime() const;
    time_t getEarliestTime() const;
    std::string getEarliestTimeFormatted() const;
    
    // 文件读取操作
    std::string readAllText();
    std::vector<std::string> readAllLines();
    std::string readLine();
    std::vector<uint8_t> readBytes(long start_offset, size_t size);
    std::vector<uint8_t> readAllBytes();

    // 目录操作
    std::vector<std::string> listFiles() const;
    std::vector<std::string> listDirectories() const;
    std::vector<std::string> listAll() const;
    
    // 文件格式检查
    bool endsWith(const std::string& suffix) const;
    bool startsWith(const std::string& prefix) const;
    bool isTextFile();
    unsigned long getSum(long start_offset, size_t size);
    unsigned long getSum();
    unsigned long getDevice();

private:
    std::string m_path;
    int m_fd;
    bool m_is_directory;
    long earliest_time;
    
    // 私有辅助方法
    void openFile();
    void openDirectory();
    void closeFile();
    std::string getLine(int fd) const;
    bool checkAccess(int mode) const;
    bool checkIsDirectory() const;
    
    // UTF-8相关辅助方法
    bool isValidUTF8Sequence(const char* data, int remaining_bytes) const;
    int getUTF8CharLength(unsigned char first_byte) const;
};

#endif //OVERT_ZFILE_H
