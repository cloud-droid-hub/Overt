//
// Created by lxz on 2025/6/13.
//

#ifndef OVERT_ZELF_H
#define OVERT_ZELF_H


#include <linux/elf.h>
#include <stddef.h>

class zElf {
public:
    char* base_addr = nullptr;

    zElf();
    zElf(char* elf_file_name);
    zElf(void* elf_mem_addr);
    ~zElf();

    Elf64_Ehdr* elf_header = nullptr;
    Elf64_Phdr* program_header_table;
    Elf64_Half program_header_table_num = 0;
    Elf64_Half header_size = 0;
    void parse_elf_head();

    Elf64_Addr load_segment_virtual_offset = 0;
    Elf64_Addr load_segment_physical_offset = 0;
    Elf64_Addr physical_address = 0;
    Elf64_Addr dynamic_table_offset = 0;
    Elf64_Dyn* dynamic_table = nullptr;
    Elf64_Xword dynamic_element_num = 0;
    void parse_program_header_table();

    char* section_string_table = nullptr;
    Elf64_Xword section_symbol_num = 0;
    Elf64_Addr string_table_offset = 0;
    char* string_table = nullptr;
    int string_table_num = 0;
    Elf64_Sym* symbol_table = nullptr;
    int symbol_table_num = 0;
    void parse_section_table();

    Elf64_Addr dynamic_symbol_table_offset = 0;
    Elf64_Sym* dynamic_symbol_table = nullptr;
    Elf64_Xword dynamic_symbol_table_num = 0;
    Elf64_Xword dynamic_symbol_element_size = 0;
    Elf64_Addr dynamic_string_table_offset = 0;
    char* dynamic_string_table = nullptr;
    int dynamic_string_table_num = 0;
    unsigned long long dynamic_string_table_size = 0;

    // 是有一个地方记录这个 so 的名称的
    Elf64_Addr soname_offset = 0;
    char* so_name = nullptr;
    Elf64_Addr gnu_hash_table_offset = 0;
    char* gnu_hash_table = nullptr;
    void parse_dynamic_table();

    char* file_ptr = nullptr;
    size_t file_size = 0;
    int file_fd = 0;
    char* parse_elf_file(char* elf_path);

    static char* parse_elf_file_(char* elf_path);

    char* find_symbol(const char *symbol_name);
    unsigned long long find_symbol_offset(const char *symbol_name);
    unsigned long long find_symbol_offset_by_dynamic(const char *symbol_name);
    unsigned long long find_symbol_offset_by_section(const char *symbol_name);

    static char* get_maps_base(char* so_name);

    static int is_link_view(uintptr_t base_addr);
};


#endif //OVERT_ZELF_H
