//
// Created by lxz on 2025/6/13.
//

#include <android/log.h>

#include "util.h"
#include "zLog.h"
#include "zFile.h"
#include "zLinker.h"

zLinker* zLinker::instance = nullptr;

zLinker::zLinker() {
    this->elf_file_ptr = parse_elf_file("/system/bin/linker64");
    LOGE("linker64 elf_file_ptr %p", this->elf_file_ptr);

    parse_elf_head();
    parse_program_header_table();
    parse_section_table();

    this->elf_mem_ptr = get_maps_base("linker64");
    LOGE("linker64 elf_mem_ptr %p", this->elf_mem_ptr);

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


char* zLinker::find_lib_base(const char* so_name){
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        char* real_path = soinfo_get_realpath(soinfo);
        LOGE("linker64 soinfo base:%p path:%s", soinfo->base, real_path);
        if(string_end_with(real_path, so_name)){
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
        if(string_end_with(real_path, so_name)){
            LOGE("linker64 fins succeed soinfo base:%p path:%s", soinfo->base, real_path);
            zElf elf_file = zElf(real_path);
            elf_file.elf_mem_ptr = (char*)soinfo->base;
            return elf_file;
        }
        soinfo = soinfo->next;
    }
    return zElf();
}


vector<string> zLinker::get_libpath_list(){
    vector<string> libpath_list = vector<string>();
    soinfo* soinfo = soinfo_head;
    while(soinfo->next != nullptr){
        char* real_path = soinfo_get_realpath(soinfo);
        libpath_list.push_back(real_path);
        soinfo = soinfo->next;
    }
    return libpath_list;
}

bool zLinker::check_lib_crc(const char* so_name){
    LOGE("check_lib_hash so_name: %s", so_name);
    zElf elf_lib_file = zLinker::getInstance()->find_lib(so_name);
    uint64_t elf_lib_file_crc = elf_lib_file.get_elf_header_crc() + elf_lib_file.get_program_header_crc() + elf_lib_file.get_text_segment_crc();

    zElf elf_lib_mem = zElf((void*)zLinker::get_maps_base(so_name));
    uint64_t elf_lib_mem_crc = elf_lib_mem.get_elf_header_crc() + elf_lib_mem.get_program_header_crc() + elf_lib_mem.get_text_segment_crc();

    LOGE("check_lib_hash elf_lib_file: %p crc: %lu", elf_lib_file.elf_file_ptr, elf_lib_file_crc);
    LOGE("check_lib_hash elf_lib_mem: %p crc: %lu", elf_lib_mem.elf_mem_ptr, elf_lib_mem_crc);

    return elf_lib_file_crc != elf_lib_mem_crc;
}