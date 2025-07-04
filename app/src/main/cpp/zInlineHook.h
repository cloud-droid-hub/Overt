#include <string>
#include <unistd.h>
#include "android/log.h"
#include "sys/mman.h"
#include "pthread.h"
#include "fcntl.h"
#include "elf.h"
#include "sys/stat.h"

extern "C"
void register_hook(void* hook_addr, void* func_on_enter);

extern "C"
void register_hook_with_leave(void* hook_addr, void* func_on_enter, void* func_on_leave);

extern "C"
void unregister_hook_pass_(void* hook_addr);

