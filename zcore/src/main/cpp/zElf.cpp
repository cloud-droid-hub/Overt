//
// Created by lxz on 2025/6/13.
//

#include <linux/elf.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <elf.h>
#include <link.h>
#include <errno.h>


#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zCrc32.h"
#include "zElf.h"

/**
 * 默认构造函数
 * 初始化空的ELF对象
 */
zElf::zElf() : zFile() {
    LOGD("Default constructor called");
    // 空实现，因为所有成员变量都已经在类定义中初始化
}

/**
 * 内存视图构造函数
 * 根据内存地址初始化ELF对象，用于分析已加载到内存的ELF文件
 * @param elf_mem_addr 内存中的ELF文件基地址
 */
zElf::zElf(void *elf_mem_addr) : zFile() {
    LOGD("Constructor called with elf_mem_addr: %p", elf_mem_addr);
    link_view = LINK_VIEW::MEMORY_VIEW;
    this->elf_mem_ptr = (char *) elf_mem_addr;
    parse_elf_head();
    parse_program_header_table();
    parse_dynamic_table();
}

/**
 * 根据标签查找动态表项
 * 遍历动态段表，查找指定标签的表项
 * @param dyn 动态表指针
 * @param tag 要查找的标签
 * @return 找到的表项指针，未找到返回NULL
 */
static ElfW(Dyn) *find_dyn_by_tag2(ElfW(Dyn) *dyn, ElfW(Sxword) tag) {
    while (dyn->d_tag != DT_NULL) {     // 遍历动态段表，直到遇到结束标记DT_NULL
        if (dyn->d_tag == tag) {         // 找到目标标签
            return dyn;                   // 返回该表项的指针
        }
        ++dyn;                           // 移动到下一个表项
    }
    return NULL;                         // 未找到返回NULL
}

/**
 * 文件路径构造函数
 * 根据文件路径初始化ELF对象，支持内存视图和文件视图
 * @param elf_file_name ELF文件路径或库名
 */
zElf::zElf(char *elf_file_name) : zFile(elf_file_name) {
    LOGD("Constructor called with elf_file_name: %s", elf_file_name);
    
    // 检查是否为库名（以"lib"开头）
    if (strncmp(elf_file_name, "lib", 3) == 0) {
        // 内存视图：从内存映射中获取库的基地址
        link_view = LINK_VIEW::MEMORY_VIEW;
        this->elf_mem_ptr = get_maps_base(elf_file_name);
        if (this->elf_mem_ptr == nullptr) {
            LOGW("Failed to get maps base for %s", elf_file_name);
            return;
        }
        parse_elf_head();
        parse_program_header_table();
        parse_dynamic_table();
    } else {
        // 文件视图：直接解析ELF文件
        link_view = LINK_VIEW::FILE_VIEW;
        this->elf_file_ptr = parse_elf_file(elf_file_name);
        if (this->elf_file_ptr == nullptr) {
            LOGW("Failed to parse elf file: %s", elf_file_name);
            return;
        }
        parse_elf_head();
        parse_program_header_table();
        parse_dynamic_table();
        parse_section_table();
    }
}

/**
 * 解析ELF头部
 * 解析ELF文件的头部信息，包括节头表偏移、程序头表等
 */
void zElf::parse_elf_head() {
    LOGD("parse_elf_head called");
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    // 设置ELF头部指针
    elf_header = (Elf64_Ehdr *) base_addr;
    LOGD("elf_header->e_shoff 0x%llx", elf_header->e_shoff);
    LOGD("elf_header->e_shnum %x", elf_header->e_shnum);
    
    // 获取ELF头部大小
    elf_header_size = elf_header->e_ehsize;
    
    // 设置程序头表指针和数量
    program_header_table = (Elf64_Phdr*)(base_addr + elf_header->e_phoff);
    program_header_table_num = elf_header->e_phnum;
}

/**
 * 解析程序头表
 * 解析ELF文件的程序头表，查找加载段、动态段等关键段
 */
void zElf::parse_program_header_table() {
    LOGD("parse_program_header_table called");
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    bool found_load_segment = false;
    
    // 遍历所有程序头表项
    for (int i = 0; i < program_header_table_num; i++) {
        // 查找第一个加载段，获取虚拟偏移和物理偏移
        if (program_header_table[i].p_type== PT_LOAD && !found_load_segment) {
            found_load_segment = true;
            load_segment_virtual_offset = program_header_table[i].p_vaddr;
            load_segment_physical_offset = program_header_table[i].p_paddr;
            LOGD("load_segment_virtual_offset %llu", load_segment_virtual_offset);
        }
        
        // 查找可执行且可读的加载段（代码段） 有的版本代码段是 PF_X | PF_R  有的只是  PF_X
        if (program_header_table[i].p_type== PT_LOAD && (program_header_table[i].p_flags == (PF_X | PF_R) || program_header_table[i].p_flags == PF_X)) {
            loadable_rx_segment = &(program_header_table[i]);
            LOGD("loadable_rx_segment %p", loadable_rx_segment);
        }

        // 查找动态段
        if (program_header_table[i].p_type == PT_DYNAMIC) {
            dynamic_table_offset = program_header_table[i].p_offset;
            dynamic_table = (Elf64_Dyn*)(base_addr + program_header_table[i].p_vaddr);
            dynamic_element_num = (program_header_table[i].p_memsz) / sizeof(Elf64_Dyn);
            LOGD("dynamic_table_offset 0x%llx", program_header_table[i].p_vaddr);
            LOGD("dynamic_table %p", dynamic_table);
            LOGD("dynamic_element_num %llu", dynamic_element_num);
        }
    }
}

/**
 * 解析动态表
 * 解析ELF文件的动态表，获取字符串表、符号表等关键信息
 */
void zElf::parse_dynamic_table() {
    LOGD("parse_dynamic_table called");
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    LOGD("parse_dynamic_table %p", dynamic_table);
    LOGD("load_segment_virtual_offset %llu", load_segment_virtual_offset);

    Elf64_Dyn *dynamic_element = dynamic_table;

    // 遍历所有动态表项
    for (int i = 0; i < dynamic_element_num; i++) {
        if (dynamic_element->d_tag == DT_STRTAB) {
            // 动态字符串表
            LOGD("DT_STRTAB 0x%llx", dynamic_element->d_un.d_ptr);
            dynamic_string_table_offset = dynamic_element->d_un.d_ptr;
            dynamic_string_table = base_addr + dynamic_element->d_un.d_ptr + load_segment_virtual_offset;
        } else if (dynamic_element->d_tag == DT_STRSZ) {
            // 动态字符串表大小
            LOGD("DT_STRSZ 0x%llx", dynamic_element->d_un.d_val);
            dynamic_string_table_size = dynamic_element->d_un.d_val;
        } else if (dynamic_element->d_tag == DT_SYMTAB) {
            // 动态符号表
            LOGD("DT_SYMTAB 0x%llx", dynamic_element->d_un.d_ptr);
            dynamic_symbol_table_offset = dynamic_element->d_un.d_ptr;
            dynamic_symbol_table = (Elf64_Sym*)(base_addr + dynamic_element->d_un.d_ptr + load_segment_virtual_offset);
        } else if (dynamic_element->d_tag == DT_SYMENT) {
            // 动态符号表项大小
            LOGD("DT_SYMENT 0x%llx", dynamic_element->d_un.d_ptr);
            dynamic_symbol_element_size = dynamic_element->d_un.d_val;
        } else if (dynamic_element->d_tag == DT_SONAME) {
            // 共享库名称
            LOGD("DT_SONAME 0x%llx", dynamic_element->d_un.d_ptr);
            soname_offset = dynamic_element->d_un.d_ptr - load_segment_virtual_offset;
            LOGD("soname_offset 0x%llx", soname_offset);
        } else if (dynamic_element->d_tag == DT_GNU_HASH) {
            // GNU哈希表
            LOGD("DT_GNU_HASH 0x%llx", dynamic_element->d_un.d_ptr);
            gnu_hash_table_offset = dynamic_element->d_un.d_ptr;
            gnu_hash_table = base_addr + dynamic_element->d_un.d_ptr + load_segment_virtual_offset;
        }
        dynamic_element++;
    }

    // 设置共享库名称
    so_name = dynamic_string_table + soname_offset;
    LOGD("soname %s", so_name);

    // 检查关键表是否解析成功
    if (dynamic_string_table == nullptr || dynamic_symbol_table == nullptr) {
        LOGW("parse_dynamic_table failed, try parse_section_table");
        return;
    }
    LOGI("parse_dynamic_table succeed");
}

/**
 * 解析节头表
 * 解析ELF文件的节头表，获取字符串表、符号表等节信息
 */
void zElf::parse_section_table() {
    LOGD("parse_section_table called");
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    LOGD("parse_section_table is called elf_mem_ptr %p", base_addr);

    // 获取节头表偏移
    Elf64_Off session_table_offset = elf_header->e_shoff;
    LOGD("parse_section_table session_table_offset 0x%llx", session_table_offset);

    // 设置节头表指针
    Elf64_Shdr* section_table = (Elf64_Shdr*)(base_addr + session_table_offset);
    LOGD("parse_section_table section_table %p", section_table);

    unsigned long section_table_value = *(unsigned long *) section_table;
    LOGD("section_table_value  0x%lx", section_table_value);

    // 获取节数量和节字符串表索引
    int section_num = elf_header->e_shnum;
    LOGD("parse_section_table section_num %d", section_num);

    int section_string_section_id = elf_header->e_shstrndx;
    LOGD("parse_section_table section_string_section_id %d", section_string_section_id);

    Elf64_Shdr *section_element = section_table;
    LOGD("parse_section_table section_element %p", section_element);

    // 设置节字符串表
    section_string_table = base_addr + (section_element + section_string_section_id)->sh_offset;
    LOGD("parse_section_table section_string_table %p", section_string_table);

    // 遍历所有节
    for (int i = 0; i < section_num; i++) {
        char *section_name = section_string_table + section_element->sh_name;
        
        if (strcmp(section_name, ".strtab") == 0) {
            // 字符串表
            LOGD("strtab %llx", section_element->sh_offset);
            string_table = base_addr + section_element->sh_offset;
        } else if (strcmp(section_name, ".dynsym") == 0) {
            // 动态符号表
            LOGD("dynsym %llx", section_element->sh_offset);
            dynamic_symbol_table = (Elf64_Sym*)(base_addr + section_element->sh_offset);
            dynamic_symbol_table_num = section_element->sh_size / sizeof(Elf64_Sym);
            LOGD("symbol_table_num %llu", dynamic_symbol_table_num);

        } else if (strcmp(section_name, ".dynstr") == 0) {
            // 动态字符串表
            LOGD("dynstr %llx", section_element->sh_offset);
            dynamic_string_table = base_addr + section_element->sh_offset;
        } else if (strcmp(section_name, ".symtab") == 0) {
            // 符号表
            symbol_table = (Elf64_Sym*) (base_addr + section_element->sh_offset);

            unsigned long long section_symbol_table_size = section_element->sh_size;
            LOGD("section_symbol_table_size %llx", section_symbol_table_size);

            unsigned long long symbol_table_element_size = section_element->sh_entsize;
            LOGD("section_symbol_element_size %llx", symbol_table_element_size);

            section_symbol_num = section_symbol_table_size / symbol_table_element_size;
            LOGD("section_symbol_num %llx", section_symbol_num);

        } else if (strcmp(section_name, ".rodata") == 0) {
            // 只读数据段（未处理）
        } else if (strcmp(section_name, ".rela.dyn") == 0) {
            // 重定位段（未处理）
        }
        section_element++;
    }
    LOGI("parse_section_table succeed");
}

/**
 * 通过动态表查找符号偏移
 * 在动态符号表中查找指定符号的偏移地址
 * @param symbol_name 符号名称
 * @return 符号的偏移地址，未找到返回0
 */
Elf64_Addr zElf::find_symbol_offset_by_dynamic(const char *symbol_name) {
    LOGD("find_symbol_by_dynamic dynamic_symbol_table_offset 0x%llx", dynamic_symbol_table_offset);
    LOGD("find_symbol_by_dynamic dynamic_symbol_table_num %llu", dynamic_symbol_table_num);

    LOGD("find_symbol_by_dynamic string_table_offset 0x%llx", string_table_offset);
    LOGD("find_symbol_by_dynamic string_table_num %d", string_table_num);

    LOGD("find_symbol_by_dynamic dynamic_string_table_offset 0x%llx", dynamic_string_table_offset);
    LOGD("find_symbol_by_dynamic dynamic_string_table_num %d", dynamic_string_table_num);

    // 确保字符串的范围在字符串表的范围内
    Elf64_Sym* dynamic_symbol = dynamic_symbol_table;
    for (int i = 0; dynamic_symbol->st_name >= 0 && dynamic_symbol->st_name <= dynamic_string_table_offset +dynamic_string_table_size; i++) {
        const char *name = dynamic_string_table + dynamic_symbol->st_name;
        if (strcmp(name, symbol_name) == 0) {
            LOGD("find_dynamic_symbol [%d] %s 0x%x", i, name, dynamic_symbol->st_name);
            return dynamic_symbol->st_value - load_segment_virtual_offset;
        }
        dynamic_symbol++;
        // LOGE("find_dynamic_symbol %d %s 0x%x", i, name, dynamic_symbol->st_name);
        // sleep(0);// android studio 中如果打印太快会丢失一些 log 日志
    }
    return 0;
}

/**
 * 通过节头表查找符号偏移
 * 在节头表的符号表中查找指定符号的偏移地址
 * @param symbol_name 符号名称
 * @return 符号的偏移地址，未找到返回0
 */
Elf64_Addr zElf::find_symbol_offset_by_section(const char *symbol_name) {
    Elf64_Sym *symbol = symbol_table;
    for (int j = 0; j < section_symbol_num; j++) {
        const char *name = string_table + symbol->st_name;
        if (strcmp(name, symbol_name) == 0) {
            LOGD("find_symbol_offset_by_section [%d] %s 0x%x 0x%x", j, name, symbol->st_value, symbol->st_value - physical_address);
            return symbol->st_value - physical_address;
        }
        symbol++;
        // LOGE("section_symbol %d %s", j, name, symbol->st_value);
    }

    return 0;
}

/**
 * 查找符号偏移
 * 先在动态表中查找，如果未找到则在节头表中查找
 * @param symbol_name 符号名称
 * @return 符号的偏移地址，未找到返回0
 */
unsigned long long zElf::find_symbol_offset(const char *symbol_name) {
    Elf64_Addr symbol_offset = 0;
    symbol_offset = find_symbol_offset_by_dynamic(symbol_name);
    symbol_offset = symbol_offset == 0 ? find_symbol_offset_by_section(symbol_name) : symbol_offset;
    return symbol_offset;
}

/**
 * 查找符号地址
 * 根据符号名称查找符号在内存中的实际地址
 * @param symbol_name 符号名称
 * @return 符号的内存地址，未找到返回nullptr
 */
char* zElf::find_symbol(const char *symbol_name) {
    if (elf_mem_ptr == nullptr) {
        LOGE("find_symbol elf_mem_ptr == nullptr");
        return nullptr;
    }

    Elf64_Addr symbol_offset = 0;
    symbol_offset = find_symbol_offset_by_dynamic(symbol_name);
    symbol_offset = symbol_offset == 0 ? find_symbol_offset_by_section(symbol_name) : symbol_offset;

    if (symbol_offset == 0) {
        LOGE("find_symbol %s failed", symbol_name);
        return nullptr;
    }
    LOGI("find_symbol %s 0x%llx", symbol_name, symbol_offset);

    return elf_mem_ptr + symbol_offset;
}

/**
 * 解析ELF文件
 * 将ELF文件映射到内存中，返回文件指针
 * @param elf_path ELF文件路径
 * @return 映射后的文件指针，失败返回nullptr
 */
char *zElf::parse_elf_file(char *elf_path) {
    LOGI("parse_elf_file %s", elf_path);

    // 使用父类的文件描述符管理
    if (!exists()) {
        LOGE("parse_elf_file: file does not exist: %s", elf_path);
        return nullptr;
    }

    // 获取文件大小
    long file_size = getFileSize();
    if (file_size <= 0) {
        LOGE("parse_elf_file: failed to get file size");
        return nullptr;
    }
    LOGD("parse_elf_file: file size obtained: %ld", file_size);
    
    // 将文件映射到内存中
    elf_file_ptr = (char *) mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, getFd(), 0);
    if (elf_file_ptr == MAP_FAILED) {
        // 处理映射失败的情况
        LOGE("parse_elf_file: failed to mmap file");
        return nullptr;
    }
    LOGI("parse_elf_file: file mapped successfully");
    return elf_file_ptr;
}

/**
 * 解析ELF文件（测试版本）
 * 不回收内存的版本，仅用于测试
 * @param elf_path ELF文件路径
 * @return 映射后的文件指针，失败返回nullptr
 */
char *zElf::parse_elf_file_(char *elf_path) {
    LOGI("parse_elf_file_ %s", elf_path);

    // 打开文件，获取文件描述符
    int elf_file_fd = open(elf_path, O_RDONLY);
    if (elf_file_fd == -1) {
        // 处理文件打开失败的情况
        LOGE("parse_elf_file_: failed to open file");
        return nullptr;
    }
    
    // 检查文件描述符是否在标准范围内（0-2）
    if (elf_file_fd <= 2) {
        LOGE("parse_elf_file_: WARNING - File descriptor %d is in standard range (0-2) for file %s", elf_file_fd, elf_path);
        LOGE("parse_elf_file_: This may cause issues with Android's unique_fd management");
    }
    
    LOGD("parse_elf_file_: file opened successfully with fd %d", elf_file_fd);
    
    // 获取文件大小
    size_t elf_file_size = lseek(elf_file_fd, 0, SEEK_END);
    if (elf_file_size == -1) {
        // 处理获取文件大小失败的情况
        LOGE("parse_elf_file_: failed to get file size");
        close(elf_file_fd);
        return nullptr;
    }
    LOGD("parse_elf_file_: file size obtained");
    
    // 将文件映射到内存中
    char *elf_file_ptr = (char *) mmap(NULL, elf_file_size, PROT_READ, MAP_PRIVATE, elf_file_fd, 0);
    if (elf_file_ptr == MAP_FAILED) {
        // 处理映射失败的情况
        LOGE("parse_elf_file_: failed to mmap file");
        close(elf_file_fd);
        return nullptr;
    }
    LOGI("parse_elf_file_: file mapped successfully");
    return elf_file_ptr;
}

/**
 * 获取代码段CRC校验和
 * 计算可执行代码段的CRC32校验和
 * @return 代码段的CRC32校验和
 */
uint64_t zElf::get_text_segment_crc(){
    LOGI("loadable_rx_segment is called");
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    if(loadable_rx_segment == nullptr){
        LOGE("loadable_rx_segment == nullptr");
        return 0;
    }

    void* code_mem_ptr = (void*)(base_addr + loadable_rx_segment->p_vaddr);
    Elf64_Xword code_mem_size = loadable_rx_segment->p_memsz;
    LOGD("check_text_segment offset:%llx code_mem_ptr: %p, code_mem_size: %llx", loadable_rx_segment->p_vaddr, code_mem_ptr, code_mem_size);

    // 计算代码段的CRC32校验和
    uint64_t crc = crc32c_fold(code_mem_ptr, code_mem_size);
    LOGD("check_text_segment code_mem_ptr: %p, code_mem_size: %llx", code_mem_ptr, code_mem_size);
    LOGD("check_text_segment crc: %lu", crc);
    return crc;
}

/**
 * 获取ELF头部CRC校验和
 * 计算ELF文件头部的CRC32校验和
 * @return ELF头部的CRC32校验和
 */
uint64_t zElf::get_elf_header_crc(){
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    void* code_mem_ptr = (void*)(base_addr);
    Elf64_Xword code_mem_size = elf_header_size;
    LOGD("get_elf_header_crc offset:%llx code_mem_ptr: %p, code_mem_size: %llx", 0, code_mem_ptr, code_mem_size);

    // 计算ELF头部的CRC32校验和
    uint64_t crc = crc32c_fold(code_mem_ptr, code_mem_size);
    LOGD("get_elf_header_crc code_mem_ptr: %p, code_mem_size: %llx", code_mem_ptr, code_mem_size);
    LOGD("get_elf_header_crc crc: %lu", crc);
    return crc;
}

/**
 * 获取程序头表CRC校验和
 * 计算ELF文件程序头表的CRC32校验和
 * @return 程序头表的CRC32校验和
 */
uint64_t zElf::get_program_header_crc(){

    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    void* code_mem_ptr = (void*)(base_addr + elf_header_size);
    Elf64_Xword code_mem_size = sizeof(Elf64_Phdr) * program_header_table_num;
    LOGD("get_program_header_crc offset:%llx code_mem_ptr: %p, code_mem_size: %llx", 0, code_mem_ptr, code_mem_size);

    // 计算程序头表的CRC32校验和
    uint64_t crc = crc32c_fold(code_mem_ptr, code_mem_size);
    LOGD("get_program_header_crc code_mem_ptr: %p, code_mem_size: %llx", code_mem_ptr, code_mem_size);
    LOGD("get_program_header_crc crc: %lu", crc);
    return crc;
}

/**
 * 析构函数
 * 清理资源，取消内存映射
 */
zElf::~zElf() {
    // 清理文件视图资源
    if (link_view == LINK_VIEW::FILE_VIEW && elf_file_ptr != nullptr) {
        if (munmap(elf_file_ptr, getFileSize()) != 0) {
            LOGW("Failed to munmap: %s", strerror(errno));
        }
        elf_file_ptr = nullptr;
    }
    // 父类析构函数会自动处理文件描述符
}

/**
 * 获取共享库在内存中的基地址
 * 通过解析/proc/self/maps文件查找指定共享库的内存基地址
 * @param so_name 共享库名称
 * @return 共享库在内存中的基地址，未找到返回nullptr
 */
char *zElf::get_maps_base(const char *so_name) {
    LOGI("get_maps_base so_name:%s", so_name);
    char *elf_mem_ptr = nullptr;
    LOGD("elf_mem_ptr:%p", elf_mem_ptr);
    
    // 读取/proc/self/maps文件
    vector<string> lines = get_file_lines("/proc/self/maps");
    LOGD("get_maps_base lines size:%zu", lines.size());
    
    // 遍历每一行
    for(int i = 0; i < lines.size(); i++){
        // 检查是否为私有映射且包含指定库名
        if (!strstr(lines[i].c_str(), "p 00000000 ")) continue;
        if (!strstr(lines[i].c_str(), so_name)) continue;
        
        // 分割地址范围
        vector<string> split_line = split_str(lines[i], "-");
        if(split_line.empty()){
            LOGW("get_maps_base split_line is empty");
            return nullptr;
        }
        LOGD("get_maps_base split_line[0]:%s", split_line[0].c_str());
        
        // 转换地址
        char* start = (char *) (strtoul(split_line[0].c_str(), nullptr, 16));
        LOGD("get_maps_base start:%p", start);
        
        // 验证ELF魔数
        if (start == nullptr || memcmp(start, "\x7f""ELF", 4) != 0) continue;
        LOGD("get_maps_base start:%p", start);
        // elf_mem_ptr = (elf_mem_ptr !=0 && elf_mem_ptr < start) ? elf_mem_ptr : start;
        elf_mem_ptr = start;
    }

    LOGI("elf_mem_ptr:%p", elf_mem_ptr);
    return elf_mem_ptr;
}

/**
 * 重写父类的 exists() 方法
 * 根据视图类型检查文件是否存在
 * @return 如果文件存在返回true
 */
bool zElf::exists() const {
    if (link_view == LINK_VIEW::MEMORY_VIEW) {
        // 内存视图：检查内存指针是否有效
        return elf_mem_ptr != nullptr;
    } else if (link_view == LINK_VIEW::FILE_VIEW) {
        // 文件视图：调用父类方法检查文件是否存在
        return zFile::exists();
    }
    return false;
}
