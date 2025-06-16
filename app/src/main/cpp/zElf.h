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
    int program_header_table_num = 0;
    int header_size = 0;
    void parse_elf_head();

    Elf64_Addr physical_address = -1;
    char* dynamic_table = nullptr;
    int dynamic_element_num = 0;
    void parse_program_header_table();

    char* section_string_table = nullptr;
    int section_symbol_num = 0;
    char* string_table = nullptr;
    int string_table_num = 0;
    char* dynamic_symbol_table = nullptr;
    int dynamic_symbol_table_num = 0;
    char* dynamic_string_table = nullptr;
    int dynamic_string_table_num = 0;
    char* symbol_table = nullptr;
    int symbol_table_num = 0;
    void parse_section_table();

    unsigned long long dynamic_string_table_size = 0;
    int dynamic_symbol_element_size = 0;
    int dynamic_symbol_table_size = 0;
    char* dynamic_version_symbol = nullptr;
    // 是有一个地方记录这个 so 的名称的
    int soname_offset = 0;
    char* so_name = nullptr;
    void parse_dynamic_table();

    char* file_ptr = nullptr;
    size_t file_size = 0;
    int file_fd = 0;
    char* parse_elf_file(char* elf_path);

    static char* parse_elf_file_(char* elf_path);

    unsigned long long find_symbol(const char *symbol_name);

    static char* get_maps_base(char* so_name);

    static int is_link_view(uintptr_t base_addr);
};


#endif //OVERT_ZELF_H
