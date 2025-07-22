//
// Created by lxz on 2025/7/11.
//

#ifndef OVERT_ZFILE_H
#define OVERT_ZFILE_H

#include <sys/stat.h>

#include "config.h"

class zFile {
public:
    // 构造函数
    zFile();
    explicit zFile(const string& path);
    
    // 析构函数
    ~zFile();
    
    // 文件路径相关
    string getPath() const;
    string getFileName() const;
    bool pathEquals(string path) const;
    bool pathStartWith(string path) const;
    bool pathEndWith(string path) const;
    bool fileNameEquals(string fileName) const;
    bool fileNameStartWith(string fileName) const;
    bool fileNameEndWith(string fileName) const;

    string getFileExtension() const;
    string getDirectory() const;

    bool exists() const;



    // 文件读取操作
    string readAllText();
    vector<string> readAllLines();
    string readLine();
    vector<uint8_t> readBytes(long start_offset, size_t size);
    vector<uint8_t> readAllBytes();
    
    // 目录操作
    vector<string> listFiles() const;
    vector<string> listDirectories() const;
    vector<string> listAll() const;
    
    // 文件格式检查
    bool endsWith(const string& suffix) const;
    bool startsWith(const string& prefix) const;
    bool isTextFile();
    unsigned long getSum(long start_offset, size_t size);
    unsigned long getSum();

    void setAttribute();

    void setFd();

    long getFd() const{return m_fd;};
    bool isOpen() const{return m_fd>=0;};
    bool isDir() const{return st_ret==0 ? S_ISDIR(st.st_mode) : false;};
    bool isFile() const{return st_ret==0 ? !S_ISDIR(st.st_mode) : false;};
    unsigned long getDevice() const{return st_ret==0 ? st.st_dev : -1;};
    long getUid() const{return st_ret==0 ? st.st_uid : -1;};
    long getGid() const{return st_ret==0 ? st.st_gid : -1;};

    time_t getEarliestTime() const;
    string getEarliestTimeFormatted() const;

    // 文件大小
    long getFileSize();

private:

    string m_path;
    int m_fd = -1;
    struct stat st;
    int st_ret = -1;      // 读取成功则该值为 0
    long earliest_time = 0;

    string getLine(int fd) const;
    
    // UTF-8相关辅助方法
    bool isValidUTF8Sequence(const char* data, int remaining_bytes) const;
    int getUTF8CharLength(unsigned char first_byte) const;
};

#endif //OVERT_ZFILE_H
