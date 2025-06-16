//
// Created by Administrator on 2024-04-12.
//

#ifndef FAKECLICK_ElfEditor_H
#define FAKECLICK_ElfEditor_H
#include "elf.h"
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "sys/mman.h"


//enum PARSE_TYPE{
//    memory_parse,
//    file_parse
//};



class zElfEditor {
public:
    std::string lib_name;
    std::string lib_path;

    char* elf_addr = nullptr;
    char* end_addr = nullptr;

    Elf64_Ehdr* elf_header;

    int program_header_table_num;

    int header_size;

    zElfEditor(std::string file_path);
    zElfEditor(void* addr);
    zElfEditor();

    ~zElfEditor();

    char* file_ptr = nullptr;
    off_t file_size;
    int fd;

    Elf64_Addr physical_address = -1;

    // 符号总有两张表，分别为 symbol_table 和 dynamic_symbol_table
    // symbol_table 位于 .symtab
    // dynamic_symbol_table 位于 .dynsym
    char* symbol_table = nullptr;
    int symbol_table_num = 0;

    char* dynamic_symbol_table = nullptr;
    int dynamic_symbol_table_num = 0;

    // 字符串有两张表，分别为 string_table 和 dynamic_string_table
    // string_table 位于 .strtab
    // dynsym_string_table 位于 .dynsym
    char* string_table = nullptr;
    int string_table_num = 0;

    char* dynamic_string_table = nullptr;
    int dynamic_string_table_num = 0;

    // 是有一个地方记录这个 so 的名称的
    int soname_offset = 0;
    char* so_name = nullptr;


    char* dynamic_table = nullptr;
    int dynamic_element_num = 0;



    int symbol_element_size = 0;
    int symbol_table_size = 0;
    char* version_symbol = nullptr;


    char* section_symbol_table = nullptr;
    char* section_string_table = nullptr;
    int section_symbol_num = 0;

    void parse_maps_file(std::string maps_path);

    void parse_elf_file(std::string elf_path);

    void parse_elf_head(char* addr);

    void parse_program_header_table();

    void parse_dynamic_table();

    void parse_section_table();

    void parse_section_table_by_map();

    Elf64_Addr find_symbol(const char* symbol_name);

};


#endif //FAKECLICK_ElfEditor_H