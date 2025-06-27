#include <stdint.h>    // 提供固定大小的整数类型定义
#include <string.h>    // 提供字符串操作函数
#include <stdbool.h>   // 提供布尔类型定义
#include <stdlib.h>    // 提供内存分配函数
#include <dlfcn.h>     // 提供动态链接函数
#include <unistd.h>
#include "plt.h"       // 包含PLT相关的结构体和宏定义
#include "android/log.h"
#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

/*
 * PLT(过程链接表)解析实现
 * 
 * 本文件实现了动态链接中的符号解析功能，主要用于解析PLT表项
 * 支持两种符号查找方式：
 * 1. GNU哈希表(新方法，更快)
 * 2. 传统ELF哈希表(旧方法，用于兼容)
 * 
 * 主要功能：
 * 1. 符号哈希计算
 * 2. 动态段解析
 * 3. 符号表遍历
 * 4. PLT表项定位
 */

/*
 * GNU风格的哈希函数，用于符号查找
 * reference: https://android.googlesource.com/platform/bionic/+/master/linker/linker_soinfo.cpp
 * 
 * 这是GNU链接器使用的特殊哈希函数，采用DJB算法
 * 相比传统ELF哈希，具有更好的分布特性，查找更快
 * 
 * 参数：
 * - name: 要计算哈希的符号名（字符串）
 * 返回：
 * - 符号名的哈希值
 */
static uint32_t gnu_hash(const uint8_t *name) {
    uint32_t h = 5381;    // DJB哈希的初始值，这个魔数经过实践证明有很好的分布性

    while (*name) {       // 遍历字符串的每个字符
        h += (h << 5) + *name++;    // h * 33 + c，DJB哈希的核心算法
                                    // h << 5 等价于 h * 32，再加上h就是h * 33
    }
    return h;
}

/*
 * 传统的ELF哈希函数
 * 在较老的ELF文件中使用，或当GNU哈希不可用时使用
 * 这是最初的ELF符号哈希算法
 * 
 * 参数：
 * - name: 要计算哈希的符号名（字符串）
 * 返回：
 * - 符号名的哈希值
 */
static uint32_t elf_hash(const uint8_t *name) {
    uint32_t h = 0, g;    // h是哈希值，g用于临时存储高位

    while (*name) {       // 遍历字符串的每个字符
        h = (h << 4) + *name++;     // h * 16 + c，将h左移4位并加上当前字符
        g = h & 0xf0000000;         // 取出h的高4位
        h ^= g;                     // 将高4位异或到结果中
        h ^= g >> 24;               // 将高4位右移24位后再次异或，增加散列效果
    }

    return h;
}

/*
 * 辅助函数：通过标签查找特定的动态段表项
 * 用于定位重要的段，如符号表、字符串表等
 * 
 * 参数：
 * - dyn: 动态段表的起始地址
 * - tag: 要查找的段类型标签
 * 返回：
 * - 找到则返回对应表项的指针，否则返回NULL
 */
static ElfW(Dyn) *find_dyn_by_tag(ElfW(Dyn) *dyn, ElfW(Sxword) tag) {
    while (dyn->d_tag != DT_NULL) {     // 遍历动态段表，直到遇到结束标记DT_NULL
        if (dyn->d_tag == tag) {         // 找到目标标签
            return dyn;                   // 返回该表项的指针
        }
        ++dyn;                           // 移动到下一个表项
    }
    return NULL;                         // 未找到返回NULL
}

/*
 * 检查符号是否为全局符号（可用于动态链接）
 * 全局符号包括：
 * 1. STB_GLOBAL绑定的符号
 * 2. 已定义的弱符号（非未定义）
 * 
 * 参数：
 * - sym: 要检查的符号表项
 * 返回：
 * - true表示是全局符号，false表示不是
 */
static inline bool is_global(ElfW(Sym) *sym) {
    unsigned char stb = ELF_ST_BIND(sym->st_info);    // 获取符号的绑定类型
    if (stb == STB_GLOBAL || stb == STB_WEAK) {      // 如果是全局符号或弱符号
        return sym->st_shndx != SHN_UNDEF;           // 检查是否已定义（不是未定义符号）
    } else {
        return false;                                 // 其他类型都不是全局符号
    }
}

/*
 * 主要的符号查找函数
 * 尝试在动态符号表中查找符号，支持GNU哈希和ELF哈希两种方式
 * 
 * 参数说明：
 * - info: 包含已加载库的信息
 * - base_addr: 动态段的基地址
 * - symbol: 要查找的符号名
 * 
 * 查找策略：
 * 1. 优先使用GNU哈希表（更快）
 * 2. 如果失败，回退到ELF哈希表
 */
static ElfW(Addr) *
find_symbol(struct dl_phdr_info *info, ElfW(Dyn) *base_addr, const char *symbol) {
    ElfW(Dyn) *dyn;

    // 获取动态符号表
    dyn = find_dyn_by_tag(base_addr, DT_SYMTAB);                             // 查找动态符号表
    ElfW(Sym) *dynsym = (ElfW(Sym) *) (info->dlpi_addr + dyn->d_un.d_ptr);  // 计算符号表的实际地址

    // 获取动态字符串表
    dyn = find_dyn_by_tag(base_addr, DT_STRTAB);                            // 查找动态字符串表
    char *dynstr = (char *) (info->dlpi_addr + dyn->d_un.d_ptr);            // 计算字符串表的实际地址

    // 首先尝试使用GNU哈希表（现代方法）
    dyn = find_dyn_by_tag(base_addr, DT_GNU_HASH);                          // 查找GNU哈希表
    if (dyn != NULL) {
        // 解析GNU哈希表结构
        ElfW(Word) *dt_gnu_hash = (ElfW(Word) *) (info->dlpi_addr + dyn->d_un.d_ptr);  // GNU哈希表的实际地址
        size_t gnu_nbucket_ = dt_gnu_hash[0];         // 哈希桶数量，存储在表的第一个字段
        uint32_t gnu_maskwords_ = dt_gnu_hash[2];     // Bloom过滤器的字数，存储在表的第三个字段
        uint32_t gnu_shift2_ = dt_gnu_hash[3];        // Bloom过滤器的移位值，存储在表的第四个字段
        
        // 计算各个表的位置
        ElfW(Addr) *gnu_bloom_filter_ = (ElfW(Addr) *) (dt_gnu_hash + 4);   // Bloom过滤器数组紧跟在头部之后
        uint32_t *gnu_bucket_ = (uint32_t *) (gnu_bloom_filter_ + gnu_maskwords_);  // 哈希桶在Bloom过滤器之后
        uint32_t *gnu_chain_ = gnu_bucket_ + gnu_nbucket_ - dt_gnu_hash[1];  // 哈希链在哈希桶之后

        --gnu_maskwords_;   // 调整Bloom过滤器字数（因为是从0开始计数）

        // 计算哈希值和Bloom过滤器参数
        uint32_t hash = gnu_hash((uint8_t *) symbol);     // 计算符号的哈希值
        uint32_t h2 = hash >> gnu_shift2_;               // 计算第二个哈希值用于Bloom过滤器

        // 首先检查Bloom过滤器（快速判断符号是否存在）
        uint32_t bloom_mask_bits = sizeof(ElfW(Addr)) * 8;    // 计算每个Bloom过滤器字的位数
        uint32_t word_num = (hash / bloom_mask_bits) & gnu_maskwords_;   // 计算使用哪个Bloom过滤器字
        ElfW(Addr) bloom_word = gnu_bloom_filter_[word_num];             // 获取对应的Bloom过滤器字

        // Bloom过滤器测试
        // 检查两个位是否都被设置：
        // 1. hash值对应的位
        // 2. h2值对应的位
        if ((1 & (bloom_word >> (hash % bloom_mask_bits)) &
             (bloom_word >> (h2 % bloom_mask_bits))) == 0) {
            return NULL;  // 如果任一位未设置，符号肯定不存在
        }

        // 在哈希桶中查找
        uint32_t n = gnu_bucket_[hash % gnu_nbucket_];    // 获取哈希值对应的桶中的第一个链表项

        if (n == 0) {    // 如果桶是空的
            return NULL;  // 符号不存在
        }

        // 遍历链直到找到符号或到达链尾
        do {
            ElfW(Sym) *sym = dynsym + n;    // 获取符号表项
            // 检查：
            // 1. 哈希值是否匹配
            // 2. 是否是全局符号
            // 3. 符号名是否相同

            LOGE("found symbol: %s %s", symbol, dynstr + sym->st_name);

            if (((gnu_chain_[n] ^ hash) >> 1) == 0
                && is_global(sym)
                && strcmp(dynstr + sym->st_name, symbol) == 0) {
                // 找到符号，计算其实际地址
                ElfW(Addr) *symbol_sym = (ElfW(Addr) *) (info->dlpi_addr + sym->st_value);
#ifdef DEBUG_PLT
                LOGI("found %s(gnu+%u) in %s, %p", symbol, n, info->dlpi_name, symbol_sym);
#endif
                return symbol_sym;
            }
        } while ((gnu_chain_[n++] & 1) == 0);  // 继续直到链尾（最低位为1标记结束）

        return NULL;  // 未找到符号
    }

    // 回退到传统ELF哈希表
    dyn = find_dyn_by_tag(base_addr, DT_HASH);    // 查找ELF哈希表
    if (dyn != NULL) {
        ElfW(Word) *dt_hash = (ElfW(Word) *) (info->dlpi_addr + dyn->d_un.d_ptr);  // ELF哈希表的实际地址
        size_t nbucket_ = dt_hash[0];           // 桶数量，存储在表的第一个字段
        uint32_t *bucket_ = dt_hash + 2;        // 哈希桶数组，跳过头部两个字段
        uint32_t *chain_ = bucket_ + nbucket_;  // 哈希链数组，在桶数组之后

        // 计算哈希并在桶中查找
        uint32_t hash = elf_hash((uint8_t *) (symbol));    // 计算符号的哈希值
        // 遍历哈希链
        for (uint32_t n = bucket_[hash % nbucket_]; n != 0; n = chain_[n]) {
            ElfW(Sym) *sym = dynsym + n;    // 获取符号表项
            // 检查：
            // 1. 是否是全局符号
            // 2. 符号名是否相同

            LOGE("found %s(elf+%u) in %s", symbol, n, info->dlpi_name);

            if (is_global(sym) &&
                strcmp(dynstr + sym->st_name, symbol) == 0) {
                // 找到符号，计算其实际地址
                ElfW(Addr) *symbol_sym = (ElfW(Addr) *) (info->dlpi_addr + sym->st_value);
#ifdef DEBUG_PLT
                LOGI("found %s(elf+%u) in %s, %p", symbol, n, info->dlpi_name, symbol_sym);
#endif
                return symbol_sym;
            }
        }

        return NULL;  // 未找到符号
    }

    return NULL;  // 未找到哈希表
}

#if defined(__LP64__)
#define Elf_Rela ElfW(Rela)
#define ELF_R_SYM ELF64_R_SYM
#else
#define Elf_Rela ElfW(Rel)
#define ELF_R_SYM ELF32_R_SYM
#endif

#ifdef DEBUG_PLT
#if defined(__x86_64__)
#define R_JUMP_SLOT R_X86_64_JUMP_SLOT
#define ELF_R_TYPE  ELF64_R_TYPE
#elif defined(__i386__)
#define R_JUMP_SLOT R_386_JMP_SLOT
#define ELF_R_TYPE  ELF32_R_TYPE
#elif defined(__arm__)
#define R_JUMP_SLOT R_ARM_JUMP_SLOT
#define ELF_R_TYPE  ELF32_R_TYPE
#elif defined(__aarch64__)
#define R_JUMP_SLOT R_AARCH64_JUMP_SLOT
#define ELF_R_TYPE  ELF64_R_TYPE
#else
#error unsupported OS
#endif
#endif

/*
 * 在PLT(过程链接表)中查找符号
 * PLT用于实现共享库函数的延迟绑定机制
 * 
 * 参数说明：
 * - info: 已加载库的信息
 * - base_addr: 动态段基地址
 * - symbol: 要查找的符号名
 * 
 * 返回：找到则返回PLT表项地址，否则返回NULL
 */
static ElfW(Addr) *find_plt(struct dl_phdr_info *info, ElfW(Dyn) *base_addr, const char *symbol) {
    // 查找跳转重定位表（PLT重定位）
    ElfW(Dyn) *dyn = find_dyn_by_tag(base_addr, DT_JMPREL);    // 查找.rel.plt或.rela.plt段
    if (dyn == NULL) {
        return NULL;    // 未找到PLT重定位表
    }
    // 计算PLT重定位表的实际地址
    Elf_Rela *dynplt = (Elf_Rela *) (info->dlpi_addr + dyn->d_un.d_ptr);

    // 获取符号解析所需的表
    dyn = find_dyn_by_tag(base_addr, DT_SYMTAB);    // 查找动态符号表

    LOGE("base:%p dyn_table222: %x", base_addr, dyn);
    ElfW(Sym) *dynsym = (ElfW(Sym) *) (info->dlpi_addr + dyn->d_un.d_ptr);

    dyn = find_dyn_by_tag(base_addr, DT_STRTAB);    // 查找动态字符串表
    char *dynstr = (char *) (info->dlpi_addr + dyn->d_un.d_ptr);

    // 获取PLT重定位表大小
    dyn = find_dyn_by_tag(base_addr, DT_PLTRELSZ);
    if (dyn == NULL) {
        return NULL;    // 未找到PLT重定位表大小
    }
    // 计算PLT表项数量
    size_t count = dyn->d_un.d_val / sizeof(Elf_Rela);
    LOGE("lib:%s plt_count:%lu", info->dlpi_name, count);

    // 遍历PLT表项
    for (size_t i = 0; i < count; ++i) {
        Elf_Rela *plt = dynplt + i;    // 获取当前PLT重定位项
#ifdef DEBUG_PLT
        // 检查重定位类型是否正确
        if (ELF_R_TYPE(plt->r_info) != R_JUMP_SLOT) {
            LOGW("invalid type for plt+%zu in %s", i, info->dlpi_name);
            continue;
        }
#endif
        // 从动态符号表获取符号名
        size_t idx = ELF_R_SYM(plt->r_info);    // 获取符号表索引
        idx = dynsym[idx].st_name;              // 获取符号名在字符串表中的偏移
        // 比较符号名

//        LOGE("found2 %s(plt+%zu) in %s", dynstr + idx, i, info->dlpi_name);
//        sleep(0);
        if (strcmp(dynstr + idx, symbol) == 0) {
            // 找到符号，计算PLT表项的实际地址
            ElfW(Addr) *symbol_plt = (ElfW(Addr) *) (info->dlpi_addr + plt->r_offset);
#ifdef DEBUG_PLT
            ElfW(Addr) *symbol_plt_value = (ElfW(Addr) *) *symbol_plt;
            LOGI("found %s(plt+%zu) in %s, %p -> %p", symbol, i, info->dlpi_name, symbol_plt,
                 symbol_plt_value);
#endif
            return symbol_plt;
        }
    }

    return NULL;    // 未找到符号
}

/*
 * 检查文件是否为so库文件
 * 通过检查文件名后缀判断
 * 
 * 参数：
 * - str: 要检查的文件路径
 * 返回：
 * - true表示是so文件，false表示不是
 */
static inline bool isso(const char *str) {
    if (str == NULL) {                // 空指针检查
        return false;
    }
    const char *dot = strrchr(str, '.');   // 查找最后一个点号位置
    return dot != NULL                      // 存在扩展名
           && *++dot == 's'                 // 第一个字符是's'
           && *++dot == 'o'                 // 第二个字符是'o'
           && (*++dot == '\0' || *dot == '\r' || *dot == '\n');  // 结尾是空字符或换行
}

/*
 * 检查路径是否为系统库路径(/system/)
 * 
 * 参数：
 * - str: 要检查的路径
 * 返回：
 * - true表示是系统库路径，false表示不是
 */
static inline bool isSystem(const char *str) {
    return str != NULL                  // 非空检查
           && *str == '/'              // 以'/'开头
           && *++str == 's'            // 后续字符依次匹配"system/"
           && *++str == 'y'
           && *++str == 's'
           && *++str == 't'
           && *++str == 'e'
           && *++str == 'm'
           && *++str == '/';
}

/*
 * 检查路径是否为厂商库路径(/vendor/)
 * 
 * 参数：
 * - str: 要检查的路径
 * 返回：
 * - true表示是厂商库路径，false表示不是
 */
static inline bool isVendor(const char *str) {
    return str != NULL                  // 非空检查
           && *str == '/'              // 以'/'开头
           && *++str == 'v'            // 后续字符依次匹配"vendor/"
           && *++str == 'e'
           && *++str == 'n'
           && *++str == 'd'
           && *++str == 'o'
           && *++str == 'r'
           && *++str == '/';
}

/*
 * 检查路径是否为OEM库路径(/oem/)
 * 
 * 参数：
 * - str: 要检查的路径
 * 返回：
 * - true表示是OEM库路径，false表示不是
 */
static inline bool isOem(const char *str) {
    return str != NULL                  // 非空检查
           && *str == '/'              // 以'/'开头
           && *++str == 'o'            // 后续字符依次匹配"oem/"
           && *++str == 'e'
           && *++str == 'm'
           && *++str == '/';
}

/*
 * 检查是否为第三方库路径
 * 不在系统、厂商和OEM路径下的库被认为是第三方库
 * 
 * 参数：
 * - str: 要检查的路径
 * 返回：
 * - true表示是第三方库路径，false表示不是
 */
static inline bool isThirdParty(const char *str) {
    if (isSystem(str) || isVendor(str) || isOem(str)) {   // 检查是否是系统、厂商或OEM路径
        return false;                                      // 如果是，则不是第三方库
    } else {
        return true;                                       // 否则是第三方库
    }
}

/*
 * 检查是否需要处理PLT表项
 * 根据符号类型和库类型决定是否需要进行PLT查找
 * 
 * 参数：
 * - symbol: 符号信息结构体
 * - info: 共享对象信息
 * 返回：
 * - true表示需要处理PLT，false表示不需要
 */
static inline bool should_check_plt(Symbol *symbol, struct dl_phdr_info *info) {
    const char *path = info->dlpi_name;                    // 获取库路径
    if (symbol->check & PLT_CHECK_PLT_ALL) {              // 如果设置了检查所有PLT的标志
        return true;                                       // 总是需要处理
    } else if (symbol->check & PLT_CHECK_PLT_APP) {       // 如果设置了只检查应用PLT的标志
        return *path != '/' || isThirdParty(path);        // 检查是否是应用自身或第三方库的PLT
    } else {
        return false;                                      // 其他情况不需要处理PLT
    }
}

/*
 * dl_iterate_phdr的回调函数
 * 遍历已加载的共享对象，查找目标符号
 * 
 * 参数说明：
 * - info: 共享对象的信息
 * - size: info结构体的大小（未使用）
 * - data: 用户数据（这里是Symbol结构体）
 * 
 * 返回：
 * - 0: 继续遍历
 * - 1: 停止遍历
 */
static int callback(struct dl_phdr_info *info, __unused size_t size, void *data) {
    if (!isso(info->dlpi_name)) {                         // 检查是否是so文件
//#ifdef DEBUG_PLT
        LOGE("ignore non-so: %s", info->dlpi_name);
//#endif
        return 0;                                         // 不是so文件则跳过
    }
    Symbol *symbol = (Symbol *) data;                     // 转换用户数据为Symbol结构体
//#if 0
    LOGE("Name: \"%s\" (%d segments)", info->dlpi_name, info->dlpi_phnum);
//#endif
    ++symbol->total;                                      // 增加处理的库计数

    // 遍历程序头表
    for (ElfW(Half) phdr_idx = 0; phdr_idx < info->dlpi_phnum; ++phdr_idx) {
        ElfW(Phdr) phdr = info->dlpi_phdr[phdr_idx];     // 获取程序头表项
        if (phdr.p_type != PT_DYNAMIC) {                  // 只处理动态段
            continue;
        }
        // 计算动态段的基地址
        ElfW(Dyn) *base_addr = (ElfW(Dyn) *) (info->dlpi_addr + phdr.p_vaddr);
        ElfW(Addr) *addr;
        LOGE("dyn_base_addr 0x%p = 0x%p + %x", base_addr, info->dlpi_addr, phdr.p_vaddr);
        // 根据需要决定是否在PLT中查找

        // dyn_base_addr 0x0x7fa7789180 = 0x0x7fa7756000 + 33180
        // 0x0x7fa7791180
        addr = should_check_plt(symbol, info) ? find_plt(info, base_addr, symbol->symbol_name) : NULL;
        addr = true ? find_plt(info, base_addr, symbol->symbol_name) : NULL;
        if (addr != NULL) {                               // 如果在PLT中找到了符号
            if (symbol->symbol_plt != NULL) {             // 如果已经有PLT记录
                // 检查PLT值是否匹配
                ElfW(Addr) *addr_value = (ElfW(Addr) *) *addr;
                ElfW(Addr) *symbol_plt_value = (ElfW(Addr) *) *symbol->symbol_plt;
                if (addr_value != symbol_plt_value) {     // 如果值不匹配
#ifdef DEBUG_PLT
                    LOGW("%s, plt %p -> %p != %p", symbol->symbol_name, addr, addr_value,
                         symbol_plt_value);
#endif
                    return 1;                             // 停止遍历
                }
            }
            symbol->symbol_plt = addr;                    // 记录PLT地址

            // 如果需要记录库名
            if (symbol->check & PLT_CHECK_NAME) {
                if (symbol->size == 0) {                  // 第一次记录
                    symbol->size = 1;
                    symbol->names = calloc(1, sizeof(char *));
                } else {                                  // 追加记录
                    ++symbol->size;
                    symbol->names = realloc(symbol->names, symbol->size * sizeof(char *));
                }
//#ifdef DEBUG_PLT
                LOGE("[%d]: %s", symbol->size - 1, info->dlpi_name);
//#endif
                symbol->names[symbol->size - 1] = strdup(info->dlpi_name);  // 保存库名的副本
            }
        }

        // 在符号表中查找
        addr = find_symbol(info, base_addr, symbol->symbol_name);
        if (addr != NULL) {                              // 如果找到了符号
            symbol->symbol_sym = addr;                    // 记录符号地址
            if (symbol->check == PLT_CHECK_SYM_ONE) {    // 如果只需要找一个
                return PLT_CHECK_SYM_ONE;                // 立即返回
            }
        }

        // 如果同时找到了PLT和符号表中的符号
        if (symbol->symbol_plt != NULL && symbol->symbol_sym != NULL) {
            ElfW(Addr) *symbol_plt_value = (ElfW(Addr) *) *symbol->symbol_plt;
            // 检查PLT值是否与符号值匹配
            if (symbol_plt_value != symbol->symbol_sym) {
//#ifdef DEBUG_PLT
                LOGE("%s, plt: %p -> %p != %p", symbol->symbol_name, symbol->symbol_plt,
                     symbol_plt_value, symbol->symbol_sym);
//#endif
                return 1;                                // 如果不匹配，停止遍历
            }
        }
    }
    return 0;                                           // 继续遍历
}

/*
 * 通过dlsym查找符号
 * 用于快速查找符号的实际地址
 * 
 * 参数：
 * - name: 要查找的符号名
 * - total: 用于返回处理的库总数（可选）
 * 返回：
 * - 找到的符号地址，未找到返回NULL
 */
void *plt_dlsym(const char *name, size_t *total) {
    Symbol symbol;
    memset(&symbol, 0, sizeof(Symbol));                  // 初始化Symbol结构体
    if (total == NULL) {                                 // 如果不需要返回总数
        symbol.check = PLT_CHECK_SYM_ONE;                // 设置只查找一个的标志
    }
    symbol.symbol_name = name;                           // 设置要查找的符号名
    dl_iterate_phdr_symbol(&symbol);                     // 开始查找
    if (total != NULL) {                                 // 如果需要返回总数
        *total = symbol.total;                           // 设置处理的库总数
    }
    return symbol.symbol_sym;                            // 返回找到的符号地址
}

/*
 * 主要的符号迭代查找函数
 * 使用dl_iterate_phdr遍历所有已加载的共享对象
 * 
 * 参数：
 * - symbol: 要查找的符号信息
 * 
 * 返回：遍历的结果
 */
int dl_iterate_phdr_symbol(Symbol *symbol) {
    int result;
#ifdef DEBUG_PLT
    LOGI("start dl_iterate_phdr: %s", symbol->symbol_name);
#endif

#if __ANDROID_API__ >= 21 || !defined(__arm__)
    result = dl_iterate_phdr(callback, symbol);          // 在高版本Android或非ARM平台直接调用
#else
    // 在低版本Android的ARM平台上需要动态获取函数
    int (*dl_iterate_phdr)(int (*)(struct dl_phdr_info *, size_t, void *), void *);
    dl_iterate_phdr = dlsym(RTLD_NEXT, "dl_iterate_phdr");
    if (dl_iterate_phdr != NULL) {
        result = dl_iterate_phdr(callback, symbol);      // 如果找到函数则调用
    } else {
        result = 0;
        void *handle = dlopen("libdl.so", RTLD_NOW);    // 尝试从libdl.so中获取函数
        dl_iterate_phdr = dlsym(handle, "dl_iterate_phdr");
        if (dl_iterate_phdr != NULL) {
            result = dl_iterate_phdr(callback, symbol);  // 如果找到函数则调用
        } else {
            LOGW("cannot dlsym dl_iterate_phdr");       // 无法找到函数
        }
        dlclose(handle);                                // 关闭库句柄
    }
#endif

#ifdef DEBUG_PLT
    LOGI("complete dl_iterate_phdr: %s", symbol->symbol_name);
#endif
    return result;                                      // 返回遍历结果
}
