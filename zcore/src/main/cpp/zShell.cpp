//
// Created by lxz on 2025/8/11.
//

#include "zLibc.h"
#include "zStd.h"
#include "zLog.h"

string runShell(string cmd){
    string ret = "";

    int pipefd[2];
    pid_t pid;
    char buffer[128];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return ret;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return ret;
    }

    if (pid == 0) {  // 子进程
        close(pipefd[0]); // 关闭读取端
        dup2(pipefd[1], STDOUT_FILENO); // 将标准输出重定向到管道
        execve("/system/bin/sh", (char* const[]){"sh", "-c", (char*)cmd.c_str(), nullptr}, nullptr);
        perror("execve");
        exit(1);
    } else {  // 父进程
        close(pipefd[1]);  // 关闭写入端
        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';  // 确保字符串以 null 结尾
            ret += buffer;
        }
        close(pipefd[0]);
    }

    return ret;
}
