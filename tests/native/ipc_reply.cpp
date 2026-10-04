// Copyright (c) 2025-2026 feicong(https://github.com/feicong/feicong-course)
#include "zBinder.h"
#include <android/sharedmem.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const std::string mode = argv[1];
    const size_t size = strtoul(argv[2], nullptr, 10);
    if (size < 16 || size > 1048571) return 2;
    alarm(12);
    zBinder* ipc = zBinder::getInstance();
    if (ipc == nullptr) return 3;
    if (mode == "short-map") {
        int fd = ASharedMemory_create("qa_short", 64);
        if (fd < 0) return 3;
        int ret = ipc->mapSharedMemory(fd);
        close(fd);
        printf("{\"rejected\":%s}\n", ret < 0 ? "true" : "false");
        return ret < 0 ? 0 : 1;
    }
    if (ipc->createSharedMemory() < 0) return 3;
    if (mode == "request") {
        int ret = ipc->sendMsg(std::string(SHM_SIZE, 'x').c_str());
        printf("{\"rejected\":%s}\n", ret < 0 ? "true" : "false");
        return ret < 0 ? 0 : 1;
    }
    if (mode == "timeout") {
        std::string got = ipc->sendMessage("qa");
        printf("{\"rejected\":%s}\n", got.empty() ? "true" : "false");
        return got.empty() ? 0 : 1;
    }
    const std::string want = "{\"value\":\"" + std::string(size - 16, 'x') + "-end\"}";
    const pid_t child = fork();
    if (child < 0) return 4;
    if (child == 0) {
        char req[64] = {};
        if (ipc->waitForMessage() != 0 || ipc->readMessage(req, sizeof(req)) != 0 ||
                strcmp(req, "qa") != 0) _exit(5);
        const std::string reply = mode == "oversize" ? std::string(SHM_SIZE, 'x') : want;
        int ret = ipc->sendResponse(reply);
        _exit((mode == "oversize" ? ret < 0 : ret == 0) ? 0 : 6);
    }
    std::string got;
    int ret = 0;
    char small[8] = "keep";
    if (mode == "small") {
        if (ipc->sendMsg("qa") != 0) return 8;
        ret = ipc->waitForResponse(small, sizeof(small));
    } else {
        got = ipc->sendMessage("qa");
    }
    int status = 0;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0) return 7;
    if (mode == "oversize" || mode == "small") {
        const bool rejected = mode == "oversize" ? got.empty() : ret < 0 && strcmp(small, "keep") == 0;
        printf("{\"rejected\":%s}\n", rejected ? "true" : "false");
        return rejected ? 0 : 1;
    }
    printf("{\"expected\":%zu,\"got\":%zu,\"exact\":%s}\n", want.size(), got.size(), got == want ? "true" : "false");
    return got == want ? 0 : 1;
}
