// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)
#include "zThreadPool.h"
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <sys/mman.h>

static std::atomic<bool> done{false};
static std::atomic<int> ran{0};
static std::atomic<bool> armed{false};
static bool poolCase = false;
static pthread_t caller;
static bool allocTask = false;
static void* taskMem = nullptr;
static size_t pageSize;
static std::atomic<bool> freed{false};

void* newReal(size_t) asm("__real__Znwm");
void* newWrap(size_t) asm("__wrap__Znwm");
void* newWrap(size_t size) {
    if (!allocTask || size != sizeof(zTask)) return newReal(size);
    allocTask = false;
    taskMem = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (taskMem == MAP_FAILED) exit(6);
    return taskMem;
}

void freeReal(void*) asm("__real__ZdlPv");
void freeWrap(void*) asm("__wrap__ZdlPv");
void freeWrap(void* ptr) {
    if (ptr != taskMem) {
        freeReal(ptr);
        return;
    }
    if (mprotect(taskMem, pageSize, PROT_NONE) != 0) exit(7);
    freed.store(true);
}

void sizedReal(void*, size_t) asm("__real__ZdlPvm");
void sizedWrap(void*, size_t) asm("__wrap__ZdlPvm");
void sizedWrap(void* ptr, size_t size) {
    if (ptr == taskMem) freeWrap(ptr);
    else sizedReal(ptr, size);
}

static void onFault(int sig, siginfo_t* info, void* ctx) {
    auto addr = reinterpret_cast<uintptr_t>(info->si_addr);
    auto start = reinterpret_cast<uintptr_t>(taskMem);
    if (freed.load() && addr >= start && addr - start < pageSize) {
        const char msg[] = "task-after-free\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _Exit(91);
    }
    _Exit(92);
}

void zLogPrint(int level, const char* tag, const char* file, const char* func,
               int line, const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (poolCase && strcmp(fmt, "zThread: taskCompletionCallback is call") == 0) {
        done.store(true);
    }
}

static void waitDone() {
    for (int i = 0; i < 5000; ++i) {
        if (done.load()) return;
        usleep(1000);
    }
    fputs("task completion timeout\n", stderr);
    exit(5);
}

bool assignReal(zThread*, zTask*) asm("__real__ZN7zThread14setExecuteTaskEP5zTask");
bool assignWrap(zThread*, zTask*) asm("__wrap__ZN7zThread14setExecuteTaskEP5zTask");
bool assignWrap(zThread* worker, zTask* task) {
    bool ok = assignReal(worker, task);
    if (ok && armed && poolCase && pthread_equal(pthread_self(), caller)) {
        waitDone();
    }
    return ok;
}

extern "C" int signalReal(pthread_cond_t*) asm("__real_pthread_cond_signal");
extern "C" int signalWrap(pthread_cond_t*) asm("__wrap_pthread_cond_signal");
extern "C" int signalWrap(pthread_cond_t* cond) {
    int rc = signalReal(cond);
    if (rc == 0 && armed && !poolCase && pthread_equal(pthread_self(), caller)) {
        waitDone();
    }
    return rc;
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    poolCase = strcmp(argv[1], "pool") == 0;
    if (!poolCase && strcmp(argv[1], "worker") != 0) return 2;
    pageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    struct sigaction action{};
    action.sa_sigaction = onFault;
    action.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &action, nullptr) != 0) return 8;
    caller = pthread_self();
    allocTask = true;
    auto* task = new zTask(string(160, 'T'), []() { ran.fetch_add(1); });
    bool ok;
    if (poolCase) {
        auto* pool = zThreadPool::getInstance();
        if (!pool) return 3;
        armed = true;
        ok = pool->addTask(task);
        armed = false;
        zThreadPool::cleanup();
    } else {
        zThread worker(0, "task-check");
        worker.setTaskCompletionCallback(std::function<void(string, void*)>(
            [](string id, void* data) { done.store(true); }));
        if (!worker.start()) return 3;
        armed = true;
        ok = worker.setExecuteTask(task);
        armed = false;
        worker.stop();
    }
    printf("{\"accepted\":%s,\"done\":%s,\"freed\":%s,\"ran\":%d}\n",
           ok ? "true" : "false", done.load() ? "true" : "false",
           freed.load() ? "true" : "false", ran.load());
    if (munmap(taskMem, pageSize) != 0) return 9;
    return fflush(stdout) == 0 ? 0 : 4;
}
