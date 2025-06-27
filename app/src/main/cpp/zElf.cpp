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
#include <stdio.h>
#include <stdlib.h>
#include <bits/elf_arm64.h>
#include <elf.h>
#include <vector>
#include <link.h>

#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

#include "zElf.h"


zElf::zElf() {
    // 空实现，因为所有成员变量都已经在类定义中初始化
}


zElf::zElf(void *elf_mem_addr) {
    this->base_addr = (char *) elf_mem_addr;
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
        this->base_addr = get_maps_base(elf_file_name);
        parse_elf_head();
        parse_program_header_table();
//        parse_dynamic_table();
    } else {
        this->base_addr = parse_elf_file(elf_file_name);

        parse_elf_head();
        parse_program_header_table();
        parse_section_table();
//        parse_dynamic_table();
    }
}

void zElf::parse_elf_head() {
    elf_header = (Elf64_Ehdr *) this->base_addr;
    LOGE("elf_header->e_shoff %x", elf_header->e_shoff);
    LOGE("elf_header->e_shnum %x", elf_header->e_shnum);
    header_size = elf_header->e_ehsize;
    program_header_table_num = elf_header->e_phnum;
}

void zElf::parse_program_header_table() {
    Elf64_Phdr *program_header_table = ((Elf64_Phdr *) (base_addr + header_size));
    for (int i = 0; i < program_header_table_num; i++) {
        if (program_header_table->p_type == PT_LOAD and physical_address == -1) {
            physical_address = program_header_table->p_paddr;
            LOGE("physical_address %x", physical_address);
        }
        if (program_header_table->p_type == PT_DYNAMIC) {
            dynamic_table = base_addr + program_header_table->p_vaddr;
            dynamic_element_num = (program_header_table->p_memsz) / sizeof(Elf64_Dyn);
            LOGE("dynamic_table_offset %x", program_header_table->p_vaddr);
            LOGE("dynamic_table 0x%p", dynamic_table);
            LOGE("dynamic_element_num %d", dynamic_element_num);
        }
        program_header_table++;
    }
}

void zElf::parse_section_table() {

    LOGE("parse_section_table is called base_addr %p", base_addr);

    int session_table_offset = elf_header->e_shoff;
    LOGE("parse_section_table session_table_offset 0x%x", session_table_offset);

    char *section_table = base_addr + session_table_offset;
    LOGE("parse_section_table section_table %p", section_table);

    unsigned long section_table_value = *(unsigned long *) section_table;
    LOGE("section_table_value  %lu", section_table_value);

    int section_num = elf_header->e_shnum;
    LOGE("parse_section_table section_num %d", section_num);

    int section_string_section_id = elf_header->e_shstrndx;
    LOGE("parse_section_table section_string_section_id %d", section_string_section_id);

    Elf64_Shdr *section_element = (Elf64_Shdr *) section_table;
    LOGE("parse_section_table section_element %p", section_element);

    section_string_table = base_addr + (section_element + section_string_section_id)->sh_offset;
    LOGE("parse_section_table section_string_table %p", section_string_table);

    for (int i = 0; i < section_num; i++) {

        char *section_name = section_string_table + section_element->sh_name;
        if (strcmp(section_name, ".strtab") == 0) {
            LOGE(".strtab %x", section_element->sh_offset);
            string_table = base_addr + section_element->sh_offset;
        } else if (strcmp(section_name, ".dynsym") == 0) {
            LOGE("dynsym %x", section_element->sh_offset);
            dynamic_symbol_table = base_addr + section_element->sh_offset;
            dynamic_symbol_table_num = section_element->sh_size / sizeof(Elf64_Sym);
            LOGE("symbol_table_num %d", dynamic_symbol_table_num);

        } else if (strcmp(section_name, ".dynstr") == 0) {
            LOGE(".dynstr %x", section_element->sh_offset);
            dynamic_string_table = base_addr + section_element->sh_offset;
        } else if (strcmp(section_name, ".symtab") == 0) {

            symbol_table = (char *) (base_addr + section_element->sh_offset);

            unsigned long long section_symbol_table_size = section_element->sh_size;
            LOGE("section_symbol_table_size %x", section_symbol_table_size);

            unsigned long long symbol_table_element_size = section_element->sh_entsize;
            LOGE("section_symbol_element_size %x", symbol_table_element_size);

            section_symbol_num = section_symbol_table_size / symbol_table_element_size;
            LOGE("section_symbol_num %x", section_symbol_num);

        } else if (strcmp(section_name, ".rodata") == 0) {

        } else if (strcmp(section_name, ".rela.dyn") == 0) {

        }
        section_element++;
    }
    LOGE("parse_section_table succeed");
}

unsigned long long zElf::find_symbol(const char *symbol_name) {
    LOGE("find_symbol symbol_table %x \n", symbol_table - base_addr);
    LOGE("find_symbol section_symbol_num %d \n", section_symbol_num);

    LOGE("find_symbol dynamic_symbol_table %x \n", dynamic_symbol_table - base_addr);
    LOGE("find_symbol dynamic_symbol_table_num %d \n", dynamic_symbol_table_num);

    LOGE("find_symbol string_table %x \n", string_table - base_addr);
    LOGE("find_symbol string_table_num %d \n", string_table_num);

    LOGE("find_symbol dynamic_string_table %x \n", dynamic_string_table - base_addr);
    LOGE("find_symbol dynamic_string_table_num %d \n", dynamic_string_table_num);

    elf64_sym *symbol = (elf64_sym *) symbol_table;
    for (int j = 0; j < section_symbol_num; j++) {
        const char *name = string_table + symbol->st_name;
        if (strcmp(name, symbol_name) == 0) {
            LOGE("find_section_symbol [%d] %s %p\n", j, name, symbol->st_value);
            return symbol->st_value - physical_address;
        }
        symbol++;
//        LOGE("section_symbol %d %s", j, name);
//        sleep(0);// android studio 中如果打印太快会丢失一些 log 日志
    }

    elf64_sym *dynamic_symbol = (elf64_sym *) dynamic_symbol_table;
    for (int j = 0; j < dynamic_symbol_table_num; j++) {
        const char *name = dynamic_string_table + dynamic_symbol->st_name;

        if (strcmp(name, symbol_name) == 0) {
            LOGE("find_dynamic_symbol [%d] %s %p\n", j, name, dynamic_symbol->st_value);
            return dynamic_symbol->st_value - physical_address;
        }

        dynamic_symbol++;
//        LOGE("dynamic_symbol %d %s", j, name);
//        sleep(0);// android studio 中如果打印太快会丢失一些 log 日志
    }
    return 0;
}

char *zElf::parse_elf_file(char *elf_path) {

    LOGE("parse_elf_file %s", elf_path);

    // 打开文件，获取文件描述符
    file_fd = open(elf_path, O_RDONLY);
    if (file_fd == -1) {
        // 处理文件打开失败的情况
        LOGE("parse_elf_file 1");
        return nullptr;
    }
    LOGE("parse_elf_file 11");
    // 获取文件大小
    file_size = lseek(file_fd, 0, SEEK_END);
    if (file_size == -1) {
        // 处理获取文件大小失败的情况
        LOGE("parse_elf_file 2");
        close(file_fd);
        return nullptr;
    }
    LOGE("parse_elf_file 22");
    // 将文件映射到内存中
    file_ptr = (char *) mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
    if (file_ptr == MAP_FAILED) {
        // 处理映射失败的情况
        LOGE("parse_elf_file 3");
        close(file_fd);
        return nullptr;
    }
    LOGE("parse_elf_file 33");
    return file_ptr;
}

// 不回收内存的版本，仅用于测试
char *zElf::parse_elf_file_(char *elf_path) {

    LOGE("parse_elf_file %s", elf_path);

    // 打开文件，获取文件描述符
    int file_fd = open(elf_path, O_RDONLY);
    if (file_fd == -1) {
        // 处理文件打开失败的情况
        LOGE("parse_elf_file 1");
        return nullptr;
    }
    LOGE("parse_elf_file 11");
    // 获取文件大小
    size_t file_size = lseek(file_fd, 0, SEEK_END);
    if (file_size == -1) {
        // 处理获取文件大小失败的情况
        LOGE("parse_elf_file 2");
        close(file_fd);
        return nullptr;
    }
    LOGE("parse_elf_file 22");
    // 将文件映射到内存中
    char *file_ptr = (char *) mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
    if (file_ptr == MAP_FAILED) {
        // 处理映射失败的情况
        LOGE("parse_elf_file 3");
        close(file_fd);
        return nullptr;
    }
    LOGE("parse_elf_file 33");
    return file_ptr;
}


void zElf::parse_dynamic_table() {

    LOGE("parse_dynamic_table %p", dynamic_table);
    LOGE("physical_address %p", physical_address);




    Elf64_Dyn *dynamic_element = (Elf64_Dyn *) dynamic_table;
    char *gnu_hash_table = nullptr;
    for (int i = 0; i < dynamic_element_num; i++) {
        LOGE("dynamic_element->d_tag %x", dynamic_element->d_tag);
        if (dynamic_element->d_tag == DT_STRTAB) {
            LOGE("DT_STRTAB %x", dynamic_element->d_un.d_ptr - physical_address);
            dynamic_string_table = base_addr + dynamic_element->d_un.d_ptr - physical_address;
        } else if (dynamic_element->d_tag == DT_STRSZ) {
            LOGE("DT_STRSZ %x", dynamic_element->d_un.d_val);
            dynamic_string_table_size = dynamic_element->d_un.d_val;
        } else if (dynamic_element->d_tag == DT_SYMTAB) {
            LOGE("DT_SYMTAB %x", dynamic_element->d_un.d_ptr - physical_address);
            dynamic_symbol_table = base_addr + dynamic_element->d_un.d_ptr - physical_address;
        } else if (dynamic_element->d_tag == DT_SYMENT) {
            LOGE("DT_SYMENT %x", dynamic_element->d_un.d_ptr - physical_address);
            dynamic_symbol_element_size = dynamic_element->d_un.d_val;
        } else if (dynamic_element->d_tag == DT_SONAME) {
            LOGE("DT_SONAME %x", dynamic_element->d_un.d_ptr - physical_address);
            soname_offset = dynamic_element->d_un.d_ptr - physical_address;
            LOGE("soname_offset 0x%x", soname_offset);
        } else if (dynamic_element->d_tag == DT_GNU_HASH) {
            LOGE("DT_GNU_HASH %x", dynamic_element->d_un.d_ptr - physical_address);
            gnu_hash_table = base_addr + dynamic_element->d_un.d_ptr - physical_address;
            LOGE("soname_offset 0x%x", soname_offset);
        }
        dynamic_element++;
    }

    // 根据 gnu_hash_table 解析动态段符号数量
    if (gnu_hash_table && dynamic_symbol_table && dynamic_symbol_element_size > 0) {
        uint8_t *ptr = (uint8_t *) gnu_hash_table;

        uint32_t nbuckets = *(uint32_t *) (ptr + 0);
        uint32_t symoffset = *(uint32_t *) (ptr + 4);
        uint32_t bloom_size = *(uint32_t *) (ptr + 8);
        uint32_t bloom_shift = *(uint32_t *) (ptr + 12);

        // 16字节偏移后是 bloom 区
        uint64_t *bloom = (uint64_t *) (ptr + 16);

        // bloom 后是 bucket 表（nbuckets 个 uint32）
        uint32_t *buckets = (uint32_t *) (bloom + bloom_size);

        // bucket 表后是 chain 表（实际长度不固定）
        uint32_t *chains = buckets + nbuckets;

        uint32_t max_sym = 0;

        // 遍历每个 bucket，获取最大 symbol 索引
        for (uint32_t i = 0; i < nbuckets; ++i) {
            uint32_t b = buckets[i];
            if (b == 0 || b < symoffset) continue;

            uint32_t idx = b;
            while (true) {
                uint32_t hash_val = chains[idx - symoffset];
                idx++;

                // gnu_hash 的 chain 结束条件是 LSB 为 1
                if (hash_val & 1) break;
            }

            if (idx > max_sym)
                max_sym = idx;
        }

        size_t dynsym_count = max_sym;
        size_t dynsym_size = dynsym_count * dynamic_symbol_element_size;

        LOGE("gnu_hash info: nbuckets=%u, symoffset=%u, bloom_size=%u, bloom_shift=%u", nbuckets, symoffset, bloom_size, bloom_shift);
        LOGE("Final dynsym count (gnu.hash): %zu", dynsym_count);
        LOGE("Final dynsym size  (gnu.hash): %zu", dynsym_size);

        dynamic_symbol_table_num = max_sym;
    }

    // 一般来讲 string_table 都在后面，所以要在遍历结束在对 so_name 进行赋值
    so_name = dynamic_string_table + soname_offset;
    LOGE("soname %s", so_name);

    if (dynamic_string_table == 0 || dynamic_symbol_table == 0) {
        LOGE("parse_dynamic_table failed, try parse_section_table");
        return;
    }
    LOGE("parse_dynamic_table succeed");
}

zElf::~zElf() {
    if (file_ptr == nullptr) {
        return;
    }
    munmap(file_ptr, file_size);
    close(file_fd);
}


int zElf::is_link_view(uintptr_t base_addr) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *) base_addr;
    LOGE("[is_link_view] ELF magic: %02x %02x %02x %02x", ehdr->e_ident[0], ehdr->e_ident[1], ehdr->e_ident[2], ehdr->e_ident[3]);
    LOGE("[is_link_view] e_phoff: 0x%lx, e_phnum: %d", ehdr->e_phoff, ehdr->e_phnum);
    LOGE("[is_link_view] e_type: %d, e_entry: 0x%lx", ehdr->e_type, ehdr->e_entry);

    Elf64_Phdr *phdrs = (Elf64_Phdr *) (base_addr + ehdr->e_phoff);
    uintptr_t load_vaddr = 0;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            load_vaddr = phdrs[i].p_vaddr;
            LOGE("[is_link_view] First PT_LOAD vaddr: 0x%lx", load_vaddr);
            break;
        }
    }

    Elf64_Dyn *dynamic = NULL;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        LOGE("[is_link_view] phdr[%d]: type=%d, vaddr=0x%lx, offset=0x%lx", i, phdrs[i].p_type, phdrs[i].p_vaddr, phdrs[i].p_offset);
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dynamic = (Elf64_Dyn *) (base_addr + (phdrs[i].p_vaddr - load_vaddr));
            LOGE("[is_link_view] Found PT_DYNAMIC at 0x%lx (mem: %p)", phdrs[i].p_vaddr, dynamic);
            break;
        }
    }
    if (!dynamic) {
        LOGE("[is_link_view] No PT_DYNAMIC found");
        return -2;
    }

    // 打印 dynamic 段前10项内容
    for (int i = 0; i < 10; i++) {
        LOGE("[is_link_view] dynamic raw[%d]: tag=0x%lx, val=0x%lx", i, dynamic[i].d_tag, dynamic[i].d_un.d_val);
        if (dynamic[i].d_tag == DT_NULL) break;
    }

    // 记录 .rela.dyn, .rela.plt
    Elf64_Addr rela_dyn_addr = 0, rela_plt_addr = 0;
    Elf64_Xword rela_dyn_size = 0, rela_plt_size = 0;
    int found_rela_dyn = 0, found_rela_plt = 0;

    for (Elf64_Dyn *dyn = dynamic; dyn->d_tag != DT_NULL; dyn++) {
        LOGE("[is_link_view] dynamic tag: 0x%lx, val: 0x%lx", dyn->d_tag, dyn->d_un.d_val);
        if (dyn->d_tag == DT_RELA) {
            rela_dyn_addr = dyn->d_un.d_ptr;
            found_rela_dyn = 1;
        } else if (dyn->d_tag == DT_RELASZ) {
            rela_dyn_size = dyn->d_un.d_val;
        } else if (dyn->d_tag == DT_JMPREL) {
            rela_plt_addr = dyn->d_un.d_ptr;
            found_rela_plt = 1;
        } else if (dyn->d_tag == DT_PLTRELSZ) {
            rela_plt_size = dyn->d_un.d_val;
        }
    }
    LOGE("[is_link_view] .rela.dyn addr: 0x%lx, size: 0x%lx", rela_dyn_addr, rela_dyn_size);
    LOGE("[is_link_view] .rela.plt addr: 0x%lx, size: 0x%lx", rela_plt_addr, rela_plt_size);

    // 检查 .rela.dyn
    if (found_rela_dyn && rela_dyn_addr && rela_dyn_size) {
        Elf64_Rela *rela = (Elf64_Rela *) (base_addr + (rela_dyn_addr - load_vaddr));
        size_t count = rela_dyn_size / sizeof(Elf64_Rela);
        LOGE("[is_link_view] .rela.dyn count: %zu", count);
        for (size_t i = 0; i < count; i++) {
            uintptr_t reloc_addr = base_addr + (rela[i].r_offset - load_vaddr);
            uintptr_t actual_val = *(uintptr_t *) reloc_addr;
            uintptr_t addend = rela[i].r_addend;
            uint32_t type = ELF64_R_TYPE(rela[i].r_info);

            LOGE("[is_link_view] .rela.dyn[%zu]: type=%u, reloc_addr=0x%lx, actual_val=0x%lx, addend=0x%lx",
                 i, type, reloc_addr, actual_val, addend);

            // 只检查特定类型的重定位
            if (type == R_AARCH64_RELATIVE || type == R_AARCH64_GLOB_DAT) {
                if (actual_val != addend && actual_val != 0) {
                    // 检查值是否在合理的地址范围内
                    if (actual_val >= base_addr && actual_val < (base_addr + 0x1000000)) {
                        LOGE("[is_link_view] Found patched relocation in .rela.dyn");
                        return 1;
                    }
                }
            }
        }
    }

    // 检查 .rela.plt
    if (found_rela_plt && rela_plt_addr && rela_plt_size) {
        Elf64_Rela *rela = (Elf64_Rela *) (base_addr + (rela_plt_addr - load_vaddr));
        size_t count = rela_plt_size / sizeof(Elf64_Rela);
        LOGE("[is_link_view] .rela.plt count: %zu", count);

        // 获取 PLT 段的地址范围
        uintptr_t plt_start = 0, plt_end = 0;
        for (int i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type == PT_LOAD && (phdrs[i].p_flags & PF_X)) {
                plt_start = base_addr + (phdrs[i].p_vaddr - load_vaddr);
                plt_end = plt_start + phdrs[i].p_memsz;
                break;
            }
        }

        // 获取 PLT 解析器的地址
        uintptr_t plt_resolver = 0;
        for (int i = 0; i < ehdr->e_phnum; i++) {
            if (phdrs[i].p_type == PT_LOAD && (phdrs[i].p_flags & PF_X)) {
                // PLT 解析器通常在第一个可执行段的开始
                plt_resolver = base_addr + (phdrs[i].p_vaddr - load_vaddr);
                break;
            }
        }

        for (size_t i = 0; i < count; i++) {
            uintptr_t reloc_addr = base_addr + (rela[i].r_offset - load_vaddr);
            uintptr_t actual_val = *(uintptr_t *) reloc_addr;
            uintptr_t addend = rela[i].r_addend;
            uint32_t type = ELF64_R_TYPE(rela[i].r_info);

            LOGE("[is_link_view] .rela.plt[%zu]: type=%u, reloc_addr=0x%lx, actual_val=0x%lx, addend=0x%lx",
                 i, type, reloc_addr, actual_val, addend);

            // 只检查 JUMP_SLOT 类型的重定位
            if (type == R_AARCH64_JUMP_SLOT) {
                if (actual_val != addend && actual_val != 0) {
                    // 检查值是否在 PLT 段范围内
                    if (actual_val >= plt_start && actual_val < plt_end) {
                        // 检查是否指向 PLT 条目的开始
                        if ((actual_val - plt_start) % 16 == 0) {
                            // 检查是否指向 PLT 解析器
                            if (actual_val == plt_resolver) {
                                // 这是正常的延时加载情况
                                continue;
                            }
                            LOGE("[is_link_view] Found patched relocation in .rela.plt");
                            return 1;
                        }
                    }
                }
            }
        }
    }

    // 所有 reloc 都未 patch，说明不是 linker 映射
    LOGE("[is_link_view] No patched relocations found");
    return 0;
}

char *zElf::get_maps_base(char *so_name) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return nullptr;

    char line[1024];
    char *base_addr = nullptr;
//    dyn_base_addr 0x0x7fa7789180 = 0x0x7fa7756000 + 33180
    while (fgets(line, sizeof(line), fp)) {
        LOGE("line:%s", line); sleep(0);
        if (!strstr(line, "p 00000000 ")) continue;

        if (!strstr(line, so_name)) continue;

        LOGE("line2:%s", line); sleep(0);

        char* start = (char *) (strtoul(strtok(line, "-"), NULL, 16));

        if (memcmp(start, "\x7f""ELF", 4) != 0) continue;

        LOGE("line3:%s", line); sleep(0);
//        7fa775e000
//        7fa7783000
        base_addr = start;

    }
    LOGE("base_addr:%p", base_addr); sleep(0);
    fclose(fp);
    return base_addr;
}
