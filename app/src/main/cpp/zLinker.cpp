//
// Created by lxz on 2025/6/13.
//

#include "zLinker.h"
#include <android/log.h>
#include <regex>
#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

zLinker* zLinker::instance = nullptr;

zLinker::zLinker() {
    this->base_addr = parse_elf_file("/system/bin/linker64");
    LOGE("linker64 file_base_addr %p", this->base_addr);

    parse_elf_head();
    parse_program_header_table();
    parse_section_table();

    this->base_addr = get_maps_base("linker64");
    LOGE("linker64 mem_base_addr %p", this->base_addr);

    soinfo*(*solist_get_head)() = (soinfo*(*)())(this->find_symbol("__dl__Z15solist_get_headv"));

    LOGE("linker64 solist_get_head %p", solist_get_head);

    soinfo_head = solist_get_head();
    LOGE("soinfo_head %p", soinfo_head);

    soinfo_get_realpath =  (char*(*)(void*))(this->find_symbol("__dl__ZNK6soinfo12get_realpathEv"));
    LOGE("soinfo_get_realpath %p", soinfo_get_realpath);

//    soinfo* soinfo = soinfo_head;
//    while(soinfo->next != nullptr){
//        char* real_path = soinfo_get_realpath(soinfo);
//        LOGE("linker64 soinfo base:%p path:%s", soinfo->base, real_path);
//        sleep(0);
//        soinfo = soinfo->next;
//    }
}

/**
 * 判断字符串 str 是否以 suffix 结尾
 *
 * @param str     要检查的完整字符串
 * @param suffix  要判断的后缀字符串
 * @return        true 如果 str 以 suffix 结尾；否则 false
 */
bool ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }

    size_t len_str = strlen(str);
    size_t len_suffix = strlen(suffix);

    if (len_suffix > len_str) {
        return false;
    }

    return (strcmp(str + len_str - len_suffix, suffix) == 0);
}

char* zLinker::find_lib_base(const char* so_name){
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        char* real_path = soinfo_get_realpath(soinfo);
        LOGE("linker64 soinfo base:%p path:%s", soinfo->base, real_path);
        if(ends_with(real_path, so_name)){
            return (char*)soinfo->base;
        }
        soinfo = soinfo->next;
    }
    return nullptr;
}

zElf zLinker::find_lib(const char* so_name){
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        char* real_path = soinfo_get_realpath(soinfo);
//        LOGE("linker64 soinfo base:%p path:%s", soinfo->base, real_path);
        if(ends_with(real_path, so_name)){
            LOGE("linker64 fins succeed soinfo base:%p path:%s", soinfo->base, real_path);
            zElf elf_file = zElf(real_path);
            elf_file.base_addr = (char*)soinfo->base;
            return elf_file;
        }
        soinfo = soinfo->next;
    }
    return zElf();
}


std::vector<std::string> zLinker::get_libpath_list(){
    std::vector<std::string> libpath_list = std::vector<std::string>();
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        char* real_path = soinfo_get_realpath(soinfo);
        libpath_list.push_back(real_path);
        soinfo = soinfo->next;
    }
    return libpath_list;
}