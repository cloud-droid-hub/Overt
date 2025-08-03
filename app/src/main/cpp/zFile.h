//
// Created by lxz on 2025/7/11.
//

#ifndef OVERT_ZFILE_H
#define OVERT_ZFILE_H


#include <sys/stat.h>
#include "config.h"

/**
 * 文件操作工具类
 * 提供文件读取、目录遍历、文件属性查询等功能
 * 支持文本文件和二进制文件的读取操作
 */
class zFile {
public:
    // 构造函数
    zFile();
    explicit zFile(const string& path);
    
    // 析构函数
    ~zFile();
    
    // 文件路径相关操作
    /**
     * 获取文件路径
     * @return 文件完整路径
     */
    string getPath() const;
    
    /**
     * 获取文件名
     * @return 文件名（不含路径）
     */
    string getFileName() const;
    
    /**
     * 检查路径是否相等
     * @param path 待比较的路径
     * @return 是否相等
     */
    bool pathEquals(string path) const;
    
    /**
     * 检查路径是否以指定字符串开头
     * @param path 前缀字符串
     * @return 是否以指定字符串开头
     */
    bool pathStartWith(string path) const;
    
    /**
     * 检查路径是否以指定字符串结尾
     * @param path 后缀字符串
     * @return 是否以指定字符串结尾
     */
    bool pathEndWith(string path) const;
    
    /**
     * 检查文件名是否相等
     * @param fileName 待比较的文件名
     * @return 是否相等
     */
    bool fileNameEquals(string fileName) const;
    
    /**
     * 检查文件名是否以指定字符串开头
     * @param fileName 前缀字符串
     * @return 是否以指定字符串开头
     */
    bool fileNameStartWith(string fileName) const;
    
    /**
     * 检查文件名是否以指定字符串结尾
     * @param fileName 后缀字符串
     * @return 是否以指定字符串结尾
     */
    bool fileNameEndWith(string fileName) const;

    /**
     * 获取文件扩展名
     * @return 文件扩展名（不含点号）
     */
    string getFileExtension() const;
    
    /**
     * 获取文件所在目录
     * @return 目录路径
     */
    string getDirectory() const;

    /**
     * 检查文件是否存在
     * @return 文件是否存在
     */
    bool exists() const;

    // 文件读取操作
    /**
     * 读取文件的所有文本内容
     * @return 文件文本内容
     */
    string readAllText();
    
    /**
     * 读取文件的所有行
     * @return 文件的所有行内容
     */
    vector<string> readAllLines();
    
    /**
     * 读取文件的一行
     * @return 一行内容
     */
    string readLine();
    
    /**
     * 读取文件的指定字节范围
     * @param start_offset 起始偏移量
     * @param size 读取字节数
     * @return 读取的字节数据
     */
    vector<uint8_t> readBytes(long start_offset, size_t size);
    
    /**
     * 读取文件的所有字节
     * @return 文件的所有字节数据
     */
    vector<uint8_t> readAllBytes();
    
    // 目录操作
    /**
     * 列出目录中的所有文件
     * @return 文件名列表
     */
    vector<string> listFiles() const;
    
    /**
     * 列出目录中的所有子目录
     * @return 子目录名列表
     */
    vector<string> listDirectories() const;
    
    /**
     * 列出目录中的所有内容（文件和子目录）
     * @return 所有内容名称列表
     */
    vector<string> listAll() const;
    
    // 文件格式检查
    /**
     * 检查文件是否以指定后缀结尾
     * @param suffix 后缀字符串
     * @return 是否以指定后缀结尾
     */
    bool endsWith(const string& suffix) const;
    
    /**
     * 检查文件是否以指定前缀开头
     * @param prefix 前缀字符串
     * @return 是否以指定前缀开头
     */
    bool startsWith(const string& prefix) const;
    
    /**
     * 检查文件是否为文本文件
     * @return 是否为文本文件
     */
    bool isTextFile();
    
    /**
     * 计算文件指定范围的校验和
     * @param start_offset 起始偏移量
     * @param size 计算字节数
     * @return 校验和
     */
    unsigned long getSum(long start_offset, size_t size);
    
    /**
     * 计算整个文件的校验和
     * @return 文件校验和
     */
    unsigned long getSum();

    /**
     * 设置文件属性
     */
    void setAttribute();

    /**
     * 设置文件描述符
     */
    void setFd();

    // 文件属性查询
    /**
     * 获取文件描述符
     * @return 文件描述符
     */
    long getFd() const{return m_fd;};
    
    /**
     * 检查文件是否已打开
     * @return 文件是否已打开
     */
    bool isOpen() const{return m_fd>=0;};
    
    /**
     * 检查是否为目录
     * @return 是否为目录
     */
    bool isDir() const{return st_ret==0 ? S_ISDIR(st.st_mode) : false;};
    
    /**
     * 检查是否为普通文件
     * @return 是否为普通文件
     */
    bool isFile() const{return st_ret==0 ? !S_ISDIR(st.st_mode) : false;};
    
    /**
     * 获取设备号
     * @return 设备号
     */
    unsigned long getDevice() const{return st_ret==0 ? st.st_dev : -1;};
    
    /**
     * 获取用户ID
     * @return 用户ID
     */
    long getUid() const{return st_ret==0 ? st.st_uid : -1;};
    
    /**
     * 获取组ID
     * @return 组ID
     */
    long getGid() const{return st_ret==0 ? st.st_gid : -1;};

    /**
     * 获取文件最早时间
     * @return 最早时间戳
     */
    time_t getEarliestTime() const;
    
    /**
     * 获取格式化的最早时间字符串
     * @return 格式化的时间字符串
     */
    string getEarliestTimeFormatted() const;

    /**
     * 获取文件大小
     * @return 文件大小（字节）
     */
    long getFileSize();

private:
    // 文件路径
    string m_path;
    
    // 文件描述符
    int m_fd = -1;
    
    // 文件状态信息
    struct stat st;
    
    // 文件状态读取结果（0表示成功）
    int st_ret = -1;
    
    // 文件最早时间
    long earliest_time = 0;

    /**
     * 从文件描述符读取一行
     * @param fd 文件描述符
     * @return 读取的行内容
     */
    string getLine(int fd) const;
    
    // UTF-8相关辅助方法
    /**
     * 检查UTF-8序列是否有效
     * @param data 数据指针
     * @param remaining_bytes 剩余字节数
     * @return 是否有效
     */
    bool isValidUTF8Sequence(const char* data, int remaining_bytes) const;
    
    /**
     * 获取UTF-8字符长度
     * @param first_byte 首字节
     * @return 字符长度
     */
    int getUTF8CharLength(unsigned char first_byte) const;
};

#endif //OVERT_ZFILE_H
