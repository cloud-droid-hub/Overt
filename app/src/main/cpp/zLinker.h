//
// Created by lxz on 2025/6/13.
//

#ifndef OVERT_ZLINKER_H
#define OVERT_ZLINKER_H

#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <link.h>

#include "zElf.h"
#include "config.h"

/**
 * soinfo结构体
 * 表示Android动态链接器中的共享库信息
 * 包含共享库的基本加载信息和链表指针
 */
struct soinfo{
    const ElfW(Phdr) *phdr;    // 程序头表指针
    size_t phnum;               // 程序头表项数

    ElfW(Addr) base;            // 共享库在内存中的基地址
    size_t size;                // 共享库大小

    ElfW(Dyn) *dynamic;         // 动态链接表指针

    soinfo *next;               // 链表下一个节点指针
};

/**
 * 动态链接器检测类
 * 继承自zElf类，负责检测和分析Android动态链接器
 * 通过解析linker64文件获取soinfo链表，支持共享库检测和CRC校验
 */
class zLinker: public zElf {
private:
    // 私有构造函数，防止外部实例化
    zLinker();
    
    // 禁用拷贝构造函数
    zLinker(const zLinker&) = delete;
    
    // 禁用赋值操作符
    zLinker& operator=(const zLinker&) = delete;

    // 静态单例实例指针
    static zLinker* instance;

public:
    /**
     * 获取单例实例
     * 采用懒加载模式，首次调用时创建实例
     * @return zLinker单例指针
     */
    static zLinker* getInstance() {
        if (instance == nullptr) {
            instance = new zLinker();
        }
        return instance;
    }

    // soinfo链表头指针
    soinfo* soinfo_head;

    // 析构函数
    ~zLinker();

    // get_realpath函数指针，用于获取共享库的真实路径
    char*(*soinfo_get_realpath)(void*) = nullptr;

    /**
     * 查找共享库的内存基地址
     * 通过遍历soinfo链表查找指定名称的共享库
     * @param so_name 共享库名称（如"libart.so"）
     * @return 共享库的内存基地址，未找到时返回nullptr
     */
    char* find_lib_base(const char* so_name);

    /**
     * 查找共享库并返回zElf对象
     * 通过遍历soinfo链表查找指定名称的共享库，并创建对应的zElf对象
     * @param so_name 共享库名称（如"libart.so"）
     * @return zElf对象，包含文件路径和内存基地址信息
     */
    zElf find_lib(const char* so_name);

    /**
     * 获取所有已加载共享库的路径列表
     * 遍历soinfo链表，收集所有共享库的真实路径
     * @return 共享库路径列表
     */
    vector<string> get_libpath_list();

    /**
     * 检查共享库的CRC校验和
     * 比较共享库文件版本和内存版本的CRC校验和，检测是否被篡改
     * @param so_name 共享库名称（如"libart.so"）
     * @return 如果CRC不匹配返回true，表示库被篡改
     */
    static bool check_lib_crc(const char* so_name);
};

#endif //OVERT_ZLINKER_H
