//
// Created by lxz on 2025/6/13.
//

#include <linux/elf.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <asm-generic/fcntl.h>
#include <fcntl.h>
#include <android/log.h>
#include <bits/elf_arm64.h>
#include <elf.h>
#include <vector>
#include <link.h>
#include "util.h"

#include "zLog.h"

#include "zElf.h"
#include "crc.h"

zElf::zElf() {
    // 空实现，因为所有成员变量都已经在类定义中初始化
}

zElf::zElf(void *elf_mem_addr) {
    link_view = LINK_VIEW::MEMORY_VIEW;
    this->elf_mem_ptr = (char *) elf_mem_addr;
    parse_elf_head();
    parse_program_header_table();
    parse_dynamic_table();
}

static ElfW(Dyn) *find_dyn_by_tag2(ElfW(Dyn) *dyn, ElfW(Sxword) tag) {
    while (dyn->d_tag != DT_NULL) {     // 遍历动态段表，直到遇到结束标记DT_NULL
    if (dyn->d_tag == tag) {         // 找到目标标签
    return dyn;                   // 返回该表项的指针
    }
    ++dyn;                           // 移动到下一个表项
    }
    return NULL;                         // 未找到返回NULL
}

zElf::zElf(char *elf_file_name) {
    if (strncmp(elf_file_name, "lib", 3) == 0) {
        link_view = LINK_VIEW::MEMORY_VIEW;
        this->elf_mem_ptr = get_maps_base(elf_file_name);
        parse_elf_head();
        parse_program_header_table();
        parse_dynamic_table();
    } else {
        link_view = LINK_VIEW::FILE_VIEW;
        this->real_path = string(elf_file_name);;
        this->elf_file_ptr = parse_elf_file(elf_file_name);
        parse_elf_head();
        parse_program_header_table();
        parse_dynamic_table();
        parse_section_table();
    }
}

void zElf::parse_elf_head() {
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    elf_header = (Elf64_Ehdr *) base_addr;
    LOGE("elf_header->e_shoff 0x%llx", elf_header->e_shoff);
    LOGE("elf_header->e_shnum %x", elf_header->e_shnum);
    elf_header_size = elf_header->e_ehsize;
    program_header_table = (Elf64_Phdr*)(base_addr + elf_header->e_phoff);
    program_header_table_num = elf_header->e_phnum;
}

void zElf::parse_program_header_table() {
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    bool found_load_segment = false;
    for (int i = 0; i < program_header_table_num; i++) {
        if (program_header_table[i].p_type== PT_LOAD && !found_load_segment) {
            found_load_segment = true;
            load_segment_virtual_offset = program_header_table[i].p_vaddr;
            load_segment_physical_offset = program_header_table[i].p_paddr;
            LOGE("load_segment_virtual_offset %llu", load_segment_virtual_offset);
        }
        if (program_header_table[i].p_type== PT_LOAD && program_header_table[i].p_flags == (PF_X | PF_R)) {
            loadable_rx_segment = &(program_header_table[i]);
            LOGE("loadable_rx_segment %p", loadable_rx_segment);
        }

        if (program_header_table[i].p_type == PT_DYNAMIC) {
            dynamic_table_offset = program_header_table[i].p_offset;
            dynamic_table = (Elf64_Dyn*)(base_addr + program_header_table[i].p_vaddr);
            dynamic_element_num = (program_header_table[i].p_memsz) / sizeof(Elf64_Dyn);
            LOGE("dynamic_table_offset 0x%llx", program_header_table[i].p_vaddr);
            LOGE("dynamic_table %p", dynamic_table);
            LOGE("dynamic_element_num %llu", dynamic_element_num);
        }
    }
}

void zElf::parse_dynamic_table() {
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    LOGE("parse_dynamic_table %p", dynamic_table);
    LOGE("load_segment_virtual_offset %llu", load_segment_virtual_offset);

    Elf64_Dyn *dynamic_element = dynamic_table;

    for (int i = 0; i < dynamic_element_num; i++) {
        if (dynamic_element->d_tag == DT_STRTAB) {
            LOGE("DT_STRTAB 0x%llx", dynamic_element->d_un.d_ptr);
            dynamic_string_table_offset = dynamic_element->d_un.d_ptr;
            dynamic_string_table = base_addr + dynamic_element->d_un.d_ptr + load_segment_virtual_offset;
        } else if (dynamic_element->d_tag == DT_STRSZ) {
            LOGE("DT_STRSZ 0x%llx", dynamic_element->d_un.d_val);
            dynamic_string_table_size = dynamic_element->d_un.d_val;
        } else if (dynamic_element->d_tag == DT_SYMTAB) {
            LOGE("DT_SYMTAB 0x%llx", dynamic_element->d_un.d_ptr);
            dynamic_symbol_table_offset = dynamic_element->d_un.d_ptr;
            dynamic_symbol_table = (Elf64_Sym*)(base_addr + dynamic_element->d_un.d_ptr + load_segment_virtual_offset);
        } else if (dynamic_element->d_tag == DT_SYMENT) {
            LOGE("DT_SYMENT 0x%llx", dynamic_element->d_un.d_ptr);
            dynamic_symbol_element_size = dynamic_element->d_un.d_val;
        } else if (dynamic_element->d_tag == DT_SONAME) {
            LOGE("DT_SONAME 0x%llx", dynamic_element->d_un.d_ptr);
            soname_offset = dynamic_element->d_un.d_ptr - load_segment_virtual_offset;
            LOGE("soname_offset 0x%llx", soname_offset);
        } else if (dynamic_element->d_tag == DT_GNU_HASH) {
            LOGE("DT_GNU_HASH 0x%llx", dynamic_element->d_un.d_ptr);
            gnu_hash_table_offset = dynamic_element->d_un.d_ptr;
            gnu_hash_table = base_addr + dynamic_element->d_un.d_ptr + load_segment_virtual_offset;
        }
        dynamic_element++;
    }

    // 一般来讲 string_table 都在后面，所以要在遍历结束再对 so_name 进行赋值
    so_name = dynamic_string_table + soname_offset;
    LOGE("soname %s", so_name);

    if (dynamic_string_table == nullptr || dynamic_symbol_table == nullptr) {
        LOGE("parse_dynamic_table failed, try parse_section_table");
        return;
    }
    LOGE("parse_dynamic_table succeed");
}

void zElf::parse_section_table() {
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    LOGE("parse_section_table is called elf_mem_ptr %p", base_addr);

    Elf64_Off session_table_offset = elf_header->e_shoff;
    LOGE("parse_section_table session_table_offset 0x%llx", session_table_offset);

    Elf64_Shdr* section_table = (Elf64_Shdr*)(base_addr + session_table_offset);
    LOGE("parse_section_table section_table %p", section_table);

    unsigned long section_table_value = *(unsigned long *) section_table;
    LOGE("section_table_value  0x%lx", section_table_value);

    int section_num = elf_header->e_shnum;
    LOGE("parse_section_table section_num %d", section_num);

    int section_string_section_id = elf_header->e_shstrndx;
    LOGE("parse_section_table section_string_section_id %d", section_string_section_id);

    Elf64_Shdr *section_element = section_table;
    LOGE("parse_section_table section_element %p", section_element);

    section_string_table = base_addr + (section_element + section_string_section_id)->sh_offset;
    LOGE("parse_section_table section_string_table %p", section_string_table);

    for (int i = 0; i < section_num; i++) {

        char *section_name = section_string_table + section_element->sh_name;
        if (strcmp(section_name, ".strtab") == 0) {
            LOGE("strtab %llx", section_element->sh_offset);
            string_table = base_addr + section_element->sh_offset;
        } else if (strcmp(section_name, ".dynsym") == 0) {
            LOGE("dynsym %llx", section_element->sh_offset);
            dynamic_symbol_table = (Elf64_Sym*)(base_addr + section_element->sh_offset);
            dynamic_symbol_table_num = section_element->sh_size / sizeof(Elf64_Sym);
            LOGE("symbol_table_num %llu", dynamic_symbol_table_num);

        } else if (strcmp(section_name, ".dynstr") == 0) {
            LOGE("dynstr %llx", section_element->sh_offset);
            dynamic_string_table = base_addr + section_element->sh_offset;
        } else if (strcmp(section_name, ".symtab") == 0) {

            symbol_table = (Elf64_Sym*) (base_addr + section_element->sh_offset);

            unsigned long long section_symbol_table_size = section_element->sh_size;
            LOGE("section_symbol_table_size %llx", section_symbol_table_size);

            unsigned long long symbol_table_element_size = section_element->sh_entsize;
            LOGE("section_symbol_element_size %llx", symbol_table_element_size);

            section_symbol_num = section_symbol_table_size / symbol_table_element_size;
            LOGE("section_symbol_num %llx", section_symbol_num);

        } else if (strcmp(section_name, ".rodata") == 0) {

        } else if (strcmp(section_name, ".rela.dyn") == 0) {

        }
        section_element++;
    }
    LOGE("parse_section_table succeed");
}


Elf64_Addr zElf::find_symbol_offset_by_dynamic(const char *symbol_name) {

    LOGE("find_symbol_by_dynamic dynamic_symbol_table_offset 0x%llx", dynamic_symbol_table_offset);
    LOGE("find_symbol_by_dynamic dynamic_symbol_table_num %llu", dynamic_symbol_table_num);

    LOGE("find_symbol_by_dynamic string_table_offset 0x%llx", string_table_offset);
    LOGE("find_symbol_by_dynamic string_table_num %d", string_table_num);

    LOGE("find_symbol_by_dynamic dynamic_string_table_offset 0x%llx", dynamic_string_table_offset);
    LOGE("find_symbol_by_dynamic dynamic_string_table_num %d", dynamic_string_table_num);

    // 确保字符串的范围在字符串表的范围内
    Elf64_Sym* dynamic_symbol = dynamic_symbol_table;
    for (int i = 0; dynamic_symbol->st_name >= 0 && dynamic_symbol->st_name <= dynamic_string_table_offset +dynamic_string_table_size; i++) {
        const char *name = dynamic_string_table + dynamic_symbol->st_name;
        if (strcmp(name, symbol_name) == 0) {
            LOGE("find_dynamic_symbol [%d] %s 0x%x", i, name, dynamic_symbol->st_name);
            return dynamic_symbol->st_value - load_segment_virtual_offset;
        }
        dynamic_symbol++;
//        LOGE("find_dynamic_symbol %d %s 0x%x", i, name, dynamic_symbol->st_name);
//        sleep(0);// android studio 中如果打印太快会丢失一些 log 日志
    }
    return 0;
}

Elf64_Addr zElf::find_symbol_offset_by_section(const char *symbol_name) {

    Elf64_Sym *symbol = symbol_table;
    for (int j = 0; j < section_symbol_num; j++) {
        const char *name = string_table + symbol->st_name;
        if (strcmp(name, symbol_name) == 0) {
            LOGE("find_dynamic_symbol [%d] %s 0x%x", j, name, symbol->st_name);
            return symbol->st_value - physical_address;
        }
        symbol++;
//        LOGE("section_symbol %d %s", j, name, symbol->st_value);
//        sleep(0);// android studio 中如果打印太快会丢失一些 log 日志
    }

    return 0;
}

unsigned long long zElf::find_symbol_offset(const char *symbol_name) {
    Elf64_Addr symbol_offset = 0;
    symbol_offset = find_symbol_offset_by_dynamic(symbol_name);
    symbol_offset = symbol_offset == 0 ? find_symbol_offset_by_section(symbol_name) : symbol_offset;
    return symbol_offset;
}

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
    LOGE("find_symbol %s 0x%llx", symbol_name, symbol_offset);

    return elf_mem_ptr + symbol_offset;
}

char *zElf::parse_elf_file(char *elf_path) {

    LOGE("parse_elf_file %s", elf_path);

    // 打开文件，获取文件描述符
    elf_file_fd = open(elf_path, O_RDONLY);
    if (elf_file_fd == -1) {
        // 处理文件打开失败的情况
        LOGE("parse_elf_file 1");
        return nullptr;
    }
    LOGE("parse_elf_file 11");
    // 获取文件大小
    elf_file_size = lseek(elf_file_fd, 0, SEEK_END);
    if (elf_file_size == -1) {
        // 处理获取文件大小失败的情况
        LOGE("parse_elf_file 2");
        close(elf_file_fd);
        return nullptr;
    }
    LOGE("parse_elf_file 22");
    // 将文件映射到内存中
    elf_file_ptr = (char *) mmap(NULL, elf_file_size, PROT_READ, MAP_PRIVATE, elf_file_fd, 0);
    if (elf_file_ptr == MAP_FAILED) {
        // 处理映射失败的情况
        LOGE("parse_elf_file 3");
        close(elf_file_fd);
        return nullptr;
    }
    LOGE("parse_elf_file 33");
    return elf_file_ptr;
}

// 不回收内存的版本，仅用于测试
char *zElf::parse_elf_file_(char *elf_path) {

    LOGE("parse_elf_file %s", elf_path);

    // 打开文件，获取文件描述符
    int elf_file_fd = open(elf_path, O_RDONLY);
    if (elf_file_fd == -1) {
        // 处理文件打开失败的情况
        LOGE("parse_elf_file 1");
        return nullptr;
    }
    LOGE("parse_elf_file 11");
    // 获取文件大小
    size_t elf_file_size = lseek(elf_file_fd, 0, SEEK_END);
    if (elf_file_size == -1) {
        // 处理获取文件大小失败的情况
        LOGE("parse_elf_file 2");
        close(elf_file_fd);
        return nullptr;
    }
    LOGE("parse_elf_file 22");
    // 将文件映射到内存中
    char *elf_file_ptr = (char *) mmap(NULL, elf_file_size, PROT_READ, MAP_PRIVATE, elf_file_fd, 0);
    if (elf_file_ptr == MAP_FAILED) {
        // 处理映射失败的情况
        LOGE("parse_elf_file 3");
        close(elf_file_fd);
        return nullptr;
    }
    LOGE("parse_elf_file 33");
    return elf_file_ptr;
}

uint64_t zElf::get_text_segment_crc(){
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    void* code_mem_ptr = (void*)(base_addr + loadable_rx_segment->p_vaddr);
    Elf64_Xword code_mem_size = loadable_rx_segment->p_memsz;
    LOGE("check_text_segment offset:%llx code_mem_ptr: %p, code_mem_size: %llx", loadable_rx_segment->p_vaddr, code_mem_ptr, code_mem_size);

    // 初始化累加和变量
    uint64_t crc = crc32c_fold(code_mem_ptr, code_mem_size);
    LOGE("check_text_segment code_mem_ptr: %p, code_mem_size: %llx", code_mem_ptr, code_mem_size);
    LOGE("check_text_segment crc: %lu", crc);
    return crc;
}

uint64_t zElf::get_elf_header_crc(){
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    void* code_mem_ptr = (void*)(base_addr);
    Elf64_Xword code_mem_size = elf_header_size;
    LOGE("get_elf_header_crc offset:%llx code_mem_ptr: %p, code_mem_size: %llx", 0, code_mem_ptr, code_mem_size);

    // 初始化累加和变量
    uint64_t crc = crc32c_fold(code_mem_ptr, code_mem_size);
    LOGE("get_elf_header_crc code_mem_ptr: %p, code_mem_size: %llx", code_mem_ptr, code_mem_size);
    LOGE("get_elf_header_crc crc: %lu", crc);
    return crc;
}

uint64_t zElf::get_program_header_crc(){
    char* base_addr = link_view == LINK_VIEW::MEMORY_VIEW ? elf_mem_ptr : elf_file_ptr;

    void* code_mem_ptr = (void*)(base_addr + elf_header_size);
    Elf64_Xword code_mem_size = sizeof(Elf64_Phdr) * program_header_table_num;
    LOGE("get_program_header_crc offset:%llx code_mem_ptr: %p, code_mem_size: %llx", 0, code_mem_ptr, code_mem_size);

    // 初始化累加和变量
    uint64_t crc = crc32c_fold(code_mem_ptr, code_mem_size);
    LOGE("get_program_header_crc code_mem_ptr: %p, code_mem_size: %llx", code_mem_ptr, code_mem_size);
    LOGE("get_program_header_crc crc: %lu", crc);
    return crc;
}

zElf::~zElf() {
    if (elf_file_ptr == nullptr) {
        return;
    }
    munmap(elf_file_ptr, elf_file_size);
    close(elf_file_fd);
}


char *zElf::get_maps_base(const char *so_name) {
    char *elf_mem_ptr = nullptr;

    vector<string> lines = get_file_lines("/proc/self/maps");
    for(int i = 0; i < lines.size(); i++){

        if (!strstr(lines[i].c_str(), "p 00000000 ")) continue;

        if (!strstr(lines[i].c_str(), so_name)) continue;

        vector<string> split_line = split_str(lines[i], "-");

        if(split_line.empty()){
            LOGE("get_maps_base split_line is empty");
            return nullptr;
        }

        char* start = (char *) (strtoul(split_line[0].c_str(), nullptr, 16));

        if (memcmp(start, "\x7f""ELF", 4) != 0) continue;

        elf_mem_ptr = start;

    }

    LOGE("elf_mem_ptr:%p", elf_mem_ptr); sleep(0);
    return elf_mem_ptr;

}
