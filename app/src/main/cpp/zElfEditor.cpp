//
// Created by lxz on 2024-04-12.
//
#include <iostream>
#include <regex>
#include <string>
#include "zElfEditor.h"
#include <android/log.h>

#include "util.h"
#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)


zElfEditor::zElfEditor() {


}

zElfEditor::zElfEditor(void* addr) {
    // 从内存中解析 elf 格式
    elf_addr = (char*)addr;

    parse_elf_head(elf_addr);

    parse_program_header_table();

    parse_dynamic_table();

}

zElfEditor::zElfEditor(std::string file_path) {

    if (strncmp(file_path.c_str(), "lib", 3) == 0) {
        lib_name = file_path;

        // 这里为了处理 libart 在不同版本中路径不同的问题
        std::string tmp_path1 = "/system/lib64/" + file_path;
        std::string tmp_path2 = "/apex/com.android.runtime/lib64/" + file_path;
        std::string tmp_path3 = "/apex/com.android.art/lib64/" + file_path;

        if (open(tmp_path1.c_str(), O_RDONLY) != -1) {
            lib_path = tmp_path1;
        } else if (open(tmp_path2.c_str(), O_RDONLY) != -1) {
            lib_path = tmp_path2;
        } else if (open(tmp_path3.c_str(), O_RDONLY) != -1) {
            lib_path = tmp_path3;
        }
    } else {
        lib_path = file_path;
        lib_name = file_path.substr(file_path.find_last_of("/") + 1);
    }

    if (strncmp(file_path.c_str(), "lib", 3) == 0 || strcmp(file_path.c_str(), "linker64") == 0) {
        // 从内存中解析 elf 格式
        lib_name = file_path;

        parse_maps_file(file_path);

        if (elf_addr == nullptr) return;

        parse_elf_head(elf_addr);

        parse_program_header_table();

        parse_dynamic_table();

    } else {
        // 从文件中解析 elf 格式
        parse_elf_file(file_path);
        if (elf_addr == nullptr) return;
        parse_elf_head(elf_addr);
        parse_program_header_table();
        parse_section_table();
    }
}

zElfEditor::~zElfEditor() {
    if (file_ptr == nullptr) {
        return;
    }
    munmap(file_ptr, file_size);
    close(fd);
}



void zElfEditor::parse_maps_file(std::string so_name) {
    uintptr_t start = 0;
    uintptr_t end = 0;
    int fd = open("/proc/self/maps", O_RDONLY);
    if (fd == -1) {
        LOGE("Error opening /proc/self/maps");
        return;
    }

    while (true) {
        std::string line = get_line(fd);
        if (line.size() == 0) {
            break;
        }
//         LOGE("get_line %s", line.c_str());
        std::regex pattern("(\\w+)-(\\w+) (\\S+) (\\S+) (\\S+) (\\S+)\\s+(.*)\\n");
        std::smatch matches;
        if (std::regex_match(line, matches, pattern) && matches.size() == 8) {
            if (matches[7].str().find(so_name) != std::string::npos) {
                start = strtoull(matches[1].str().c_str(), NULL, 16);
                end = strtoull(matches[2].str().c_str(), NULL, 16);
                 LOGE("start     %s", matches[1].str().c_str());
                 LOGE("end       %s", matches[2].str().c_str());
                 LOGE("rwx       %s", matches[3].str().c_str());
                 LOGE("offset    %s", matches[4].str().c_str());
                 LOGE("deviceid  %s", matches[5].str().c_str());
                 LOGE("nodeid    %s", matches[6].str().c_str());
                 LOGE("filename  %s", matches[7].str().c_str());
                 LOGE("line     %s", matches[0].str().c_str());
                if (elf_addr == nullptr) {
                    // LOGE("elf_addr == nullptr");
                    elf_addr = (char *) start;
                }
                if ((long) end_addr < (long) end) {
                    // LOGE("end_addr < (long)end");
                    end_addr = (char *) end;
                }
            }
        }
//        LOGE("find %s %p %p", so_name.c_str(), elf_addr, end_addr);
    }
    close(fd);
}

void zElfEditor::parse_elf_file(std::string elf_path) {

    LOGE("parse_elf_file %s", elf_path.c_str());

    // 打开文件，获取文件描述符
    fd = open(elf_path.c_str(), O_RDONLY);
    if (fd == -1) {
        // 处理文件打开失败的情况
        return;
    }

    // 获取文件大小
    file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        // 处理获取文件大小失败的情况
        close(fd);
        return;
    }

    // 将文件映射到内存中
    file_ptr = (char *) mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_ptr == MAP_FAILED) {
        // 处理映射失败的情况
        close(fd);
        return;
    }

    // 使用 file_ptr 指针访问文件内容
    elf_addr = file_ptr;
}

void zElfEditor::parse_elf_head(char *elf_addr) {
    zElfEditor::elf_addr = elf_addr;
    elf_header = (Elf64_Ehdr *) elf_addr;
    header_size = elf_header->e_ehsize;
    program_header_table_num = elf_header->e_phnum;
}

void zElfEditor::parse_program_header_table() {
    Elf64_Phdr *program_header_table = ((Elf64_Phdr *) (elf_addr + header_size));
    for (int i = 0; i < program_header_table_num; i++) {
        if (program_header_table->p_type == 1 and physical_address == -1) {
            physical_address = program_header_table->p_paddr;
            LOGE("physical_address %x", physical_address);
        }
        if (program_header_table->p_type == 2) {
            dynamic_table = elf_addr + program_header_table->p_offset;
            dynamic_element_num = (program_header_table->p_filesz) / sizeof(Elf64_Dyn);
            LOGE("dynamic_table_offset %x", program_header_table->p_offset);
            LOGE("dynamic_element_num %d", dynamic_element_num);
        }
        program_header_table++;
    }
}

void zElfEditor::parse_dynamic_table() {

    LOGE("parse_dynamic_table %p", dynamic_table);
    Elf64_Dyn *dynamic_element = (Elf64_Dyn *) dynamic_table;
    for (int i = 0; i < dynamic_element_num; i++) {
        if (dynamic_element->d_tag == DT_STRTAB) {
            LOGE("DT_STRTAB %x", dynamic_element->d_un.d_ptr - physical_address);
            string_table = elf_addr + dynamic_element->d_un.d_ptr - physical_address;
        } else if (dynamic_element->d_tag == DT_SYMTAB) {
            LOGE("DT_SYMTAB %x", dynamic_element->d_un.d_ptr - physical_address);
            symbol_table = elf_addr + dynamic_element->d_un.d_ptr - physical_address;
        } else if (dynamic_element->d_tag == DT_SYMENT) {
            LOGE("DT_SYMENT %x", dynamic_element->d_un.d_ptr - physical_address);
            symbol_element_size = dynamic_element->d_un.d_ptr - physical_address;
        } else if (dynamic_element->d_tag == DT_VERSYM) {
            LOGE("DT_VERSYM %x", dynamic_element->d_un.d_ptr - physical_address);
            version_symbol = elf_addr + (dynamic_element->d_un.d_ptr - physical_address);
            symbol_table_size = version_symbol - symbol_table;
            symbol_table_num = symbol_table_size / symbol_element_size;
            LOGE("symbol_table_num %d", symbol_table_num);
        } else if (dynamic_element->d_tag == DT_SONAME) {
            LOGE("DT_SONAME %x", dynamic_element->d_un.d_ptr - physical_address);
            soname_offset = dynamic_element->d_un.d_ptr - physical_address;
            LOGE("soname_offset 0x%x", soname_offset);
        }
        dynamic_element++;
    }

    // 一般来讲 string_table 都在后面，所以要在遍历结束在对 so_name 进行赋值
    so_name = string_table + soname_offset;
    LOGE("soname %s", so_name);

    if (string_table == 0 || symbol_table == 0) {
        LOGE("parse_dynamic_table failed, try parse_section_table");
    }
}
#define PAGE_START(x)  ((x) & PAGE_MASK)
void zElfEditor::parse_section_table() {

    LOGE("parse_section_table is called elf_addr %p", elf_addr);

    int session_table_offset = elf_header->e_shoff;
    LOGE("parse_section_table session_table_offset 0x%x", session_table_offset);

    char *section_table = elf_addr + session_table_offset;
    LOGE("parse_section_table section_table %p", section_table);

    int section_num = elf_header->e_shnum;
    LOGE("parse_section_table section_num %d", section_num);

    int section_string_section_id = elf_header->e_shstrndx;
    LOGE("parse_section_table section_string_section_id %d", section_string_section_id);

    Elf64_Shdr *section_element = (Elf64_Shdr *) section_table;
    LOGE("parse_section_table section_element %p", section_element);

    FILE *fp = fopen("/proc/self/maps", "r");

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "/apex/com.android.runtime/bin") != nullptr) {
            LOGE("maps2 %s", line);
        }
    }

    char* tmp = (char*)(section_element + section_string_section_id);
    LOGE("parse_section_table tmp %p", tmp);

//    // 设置内存读写执行权限
//    mprotect((void *) PAGE_START((long) tmp), PAGE_SIZE, PROT_WRITE | PROT_READ | PROT_EXEC);


    section_string_table = elf_addr + (section_element + section_string_section_id)->sh_offset;
    LOGE("parse_section_table section_string_table %p", section_string_table);


    for (int i = 0; i < section_num; i++) {

        char* section_name = section_string_table + section_element->sh_name;
        if (strcmp(section_name, ".strtab") == 0) {
            LOGE(".strtab %x", section_element->sh_offset);
            string_table = elf_addr + section_element->sh_offset;
        } else if (strcmp(section_name, ".dynsym") == 0) {
            LOGE("dynsym %x", section_element->sh_offset);
            dynamic_symbol_table = elf_addr + section_element->sh_offset;
            dynamic_symbol_table_num = section_element->sh_size / sizeof(Elf64_Sym);
            LOGE("symbol_table_num %d", dynamic_symbol_table_num);

        } else if (strcmp(section_name, ".dynstr") == 0) {
            LOGE(".dynstr %x", section_element->sh_offset);
            dynamic_string_table = elf_addr + section_element->sh_offset;
        }else if (strcmp(section_name, ".symtab") == 0) {

            symbol_table = (char*)(elf_addr + section_element->sh_offset);

            unsigned long long section_symbol_table_size = section_element->sh_size;
            LOGE("section_symbol_table_size %x", section_symbol_table_size);

            unsigned long long symbol_table_element_size = section_element->sh_entsize;
            LOGE("section_symbol_element_size %x", symbol_table_element_size);

            section_symbol_num = section_symbol_table_size / symbol_table_element_size;
            LOGE("section_symbol_num %x", section_symbol_num);


        }
        else if (strcmp(section_name, ".rodata") == 0) {

        }
        else if (strcmp(section_name, ".rela.dyn") == 0) {

        }
        section_element++;
    }

}


unsigned long long zElfEditor::find_symbol(const char *symbol_name) {
    LOGE("find_symbol symbol_table %x \n", symbol_table-elf_addr);
    LOGE("find_symbol section_symbol_num %d \n", section_symbol_num);

    LOGE("find_symbol dynamic_symbol_table %x \n", dynamic_symbol_table-elf_addr);
    LOGE("find_symbol dynamic_symbol_table_num %d \n", dynamic_symbol_table_num);

    LOGE("find_symbol string_table %x \n", string_table-elf_addr);
    LOGE("find_symbol string_table_num %d \n", string_table_num);

    LOGE("find_symbol dynamic_string_table %x \n", dynamic_string_table-elf_addr);
    LOGE("find_symbol dynamic_string_table_num %d \n", dynamic_string_table_num);

    elf64_sym* symbol = (elf64_sym*)symbol_table;
    for (int j = 0; j < section_symbol_num; j++) {
        const char* name = string_table + symbol->st_name;

        if (strcmp(name, symbol_name) == 0) {
            LOGE("find_symbol %s %p\n", name, symbol->st_value);
            return symbol->st_value - physical_address;
        }

        symbol++;
        LOGE("symbol %d %s", j, name);
        usleep(0);// android studio 中如果打印太快会丢失一些 log 日志
    }

    elf64_sym* dynamic_symbol = (elf64_sym*)dynamic_symbol_table;
    for (int j = 0; j < dynamic_symbol_table_num; j++) {
        const char* name = dynamic_string_table + dynamic_symbol->st_name;

        if (strcmp(name, symbol_name) == 0) {
            LOGE("find_dynamic_symbol %s %p\n", name, dynamic_symbol->st_value);
            return dynamic_symbol->st_value - physical_address;
        }

        dynamic_symbol++;
        LOGE("dynamic_symbol %d %s", j, name);
        usleep(0);// android studio 中如果打印太快会丢失一些 log 日志
    }
    return 0;
}