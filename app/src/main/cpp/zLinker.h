//
// Created by lxz on 2025/6/13.
//

#ifndef OVERT_ZLINKER_H
#define OVERT_ZLINKER_H

#include <elf.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <link.h>
#include "zElf.h"

struct soinfo{
    const ElfW(Phdr) *phdr;
    size_t phnum;

    ElfW(Addr) base;
    size_t size;

    ElfW(Dyn) *dynamic;

    soinfo *next;
};

class zLinker: public zElf {
private:
    // Private constructor to prevent direct instantiation
    zLinker();
    
    // Delete copy constructor and assignment operator
    zLinker(const zLinker&) = delete;
    zLinker& operator=(const zLinker&) = delete;

    // Static instance
    static zLinker* instance;

public:

    // Static method to get the singleton instance
    static zLinker* getInstance() {
        if (instance == nullptr) {
            instance = new zLinker();
        }
        return instance;
    }

    soinfo* soinfo_head;

    ~zLinker();

    char*(*soinfo_get_realpath)(void*) = nullptr;

    char* find_lib_base(const char* so_name);

    zElf find_lib(const char* so_name);

    std::vector<std::string> get_libpath_list();

    static bool check_lib_hash(const char* so_name);
};


#endif //OVERT_ZLINKER_H
