//
// Created by lxz on 2025/6/13.
//

#include <android/log.h>

#include "zStd.h"
#include "zStdUtil.h"
#include "zLog.h"
#include "zFile.h"
#include "zLinker.h"

// 静态单例实例指针
zLinker* zLinker::instance = nullptr;

/**
 * zLinker构造函数
 * 初始化动态链接器检测器，解析linker64文件并获取soinfo链表
 * 通过ELF解析技术获取linker64的内存布局和符号信息
 */
zLinker::zLinker() {
    LOGD("Constructor called");
    
    // 解析linker64文件的ELF结构
    this->elf_file_ptr = parse_elf_file("/system/bin/linker64");
    LOGI("linker64 elf_file_ptr %p", this->elf_file_ptr);

    // 解析ELF头部信息
    parse_elf_head();
    // 解析程序头表
    parse_program_header_table();
    // 解析节头表
    parse_section_table();

    // 获取linker64在内存中的基地址
    this->elf_mem_ptr = get_maps_base("linker64");
    LOGI("linker64 elf_mem_ptr %p", this->elf_mem_ptr);

    // 获取solist_get_head函数指针，用于获取soinfo链表头
    soinfo*(*solist_get_head)() = (soinfo*(*)())(this->find_symbol("__dl__Z15solist_get_headv"));

    LOGI("linker64 solist_get_head %p", solist_get_head);

    // 获取soinfo链表头指针
    soinfo_head = solist_get_head();
    LOGI("soinfo_head %p", soinfo_head);

    // 获取get_realpath函数指针，用于获取共享库的真实路径
    soinfo_get_realpath =  (char*(*)(void*))(this->find_symbol("__dl__ZNK6soinfo12get_realpathEv"));
    LOGI("soinfo_get_realpath %p", soinfo_get_realpath);

    // 遍历所有已加载的共享库
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        char* real_path = soinfo_get_realpath(soinfo);
        LOGV("linker64 soinfo base:%p path:%s", soinfo->base, real_path);
        soinfo = soinfo->next;
    }
}

/**
 * 查找共享库的内存基地址
 * 通过遍历soinfo链表查找指定名称的共享库
 * @param so_name 共享库名称（如"libart.so"）
 * @return 共享库的内存基地址，未找到时返回nullptr
 */
char* zLinker::find_lib_base(const char* so_name){
    LOGD("find_lib_base called with so_name: %s", so_name);
    
    // 从soinfo链表头开始遍历
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        // 获取共享库的真实路径
        char* real_path = soinfo_get_realpath(soinfo);
        LOGD("linker64 soinfo base:%p path:%s", soinfo->base, real_path);
        
        // 检查路径是否以指定名称结尾
        if(string_end_with(real_path, so_name)){
            LOGI("find_lib_base: found %s at base %p", so_name, soinfo->base);
            return (char*)soinfo->base;
        }
        soinfo = soinfo->next;
    }
    LOGD("find_lib_base: %s not found", so_name);
    return nullptr;
}

/**
 * 查找共享库并返回zElf对象
 * 通过遍历soinfo链表查找指定名称的共享库，并创建对应的zElf对象
 * @param so_name 共享库名称（如"libart.so"）
 * @return zElf对象，包含文件路径和内存基地址信息
 */
zElf zLinker::find_lib(const char* so_name){
    LOGD("find_lib called with so_name: %s", so_name);
    
    // 从soinfo链表头开始遍历
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        // 获取共享库的真实路径
        char* real_path = soinfo_get_realpath(soinfo);
        
        // 检查路径是否以指定名称结尾
        if(string_end_with(real_path, so_name)){
            LOGI("find_lib: found %s at base %p path:%s", so_name, soinfo->base, real_path);
            
            // 创建zElf对象，包含文件路径
            zElf elf_file = zElf(real_path);
            // 设置内存基地址
            elf_file.elf_mem_ptr = (char*)soinfo->base;
            return elf_file;
        }
        soinfo = soinfo->next;
    }
    LOGD("find_lib: %s not found", so_name);
    return zElf();
}

/**
 * 获取所有已加载共享库的路径列表
 * 遍历soinfo链表，收集所有共享库的真实路径
 * @return 共享库路径列表
 */
vector<string> zLinker::get_libpath_list(){
    LOGD("get_libpath_list called");
    vector<string> libpath_list = vector<string>();
    
    // 从soinfo链表头开始遍历
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        // 获取共享库的真实路径
        char* real_path = soinfo_get_realpath(soinfo);
        // 添加到路径列表
        libpath_list.push_back(real_path);
        soinfo = soinfo->next;
    }
    LOGI("get_libpath_list: found %zu libraries", libpath_list.size());
    return libpath_list;
}

/**
 * 检查共享库的CRC校验和
 * 比较共享库文件版本和内存版本的CRC校验和，检测是否被篡改
 * @param so_name 共享库名称（如"libart.so"）
 * @return 如果CRC不匹配返回true，表示库被篡改
 */
bool zLinker::check_lib_crc(const char* so_name){
    LOGD("check_lib_crc called with so_name: %s", so_name);
    LOGI("check_lib_hash so_name: %s", so_name);
    
    // 获取共享库的zElf对象（文件版本）
    zElf elf_lib_file = zLinker::getInstance()->find_lib(so_name);
    // 计算文件版本的CRC校验和（ELF头 + 程序头 + 代码段）
    uint64_t elf_lib_file_crc = elf_lib_file.get_elf_header_crc() + 
                                elf_lib_file.get_program_header_crc() + 
                                elf_lib_file.get_text_segment_crc();

    // 获取共享库的内存版本zElf对象
    zElf elf_lib_mem = zElf((void*)zLinker::get_maps_base(so_name));
    // 计算内存版本的CRC校验和
    uint64_t elf_lib_mem_crc = elf_lib_mem.get_elf_header_crc() + 
                               elf_lib_mem.get_program_header_crc() + 
                               elf_lib_mem.get_text_segment_crc();

    LOGI("check_lib_hash elf_lib_file: %p crc: %lu", elf_lib_file.elf_file_ptr, elf_lib_file_crc);
    LOGI("check_lib_hash elf_lib_mem: %p crc: %lu", elf_lib_mem.elf_mem_ptr, elf_lib_mem_crc);

    // 比较文件版本和内存版本的CRC校验和
    bool crc_mismatch = elf_lib_file_crc != elf_lib_mem_crc;
    if (crc_mismatch) {
        LOGW("check_lib_crc: CRC mismatch detected for %s", so_name);
    } else {
        LOGI("check_lib_crc: CRC match for %s", so_name);
    }
    return crc_mismatch;
}