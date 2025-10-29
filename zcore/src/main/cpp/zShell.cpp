//
// Created by lxz on 2025/8/11.
//

#include "zLibc.h"
#include "zStd.h"
#include "zLog.h"

/**
 * 执行Shell命令
 * 在Android系统中执行指定的Shell命令并返回输出结果
 * 使用fork和execve系统调用实现安全的命令执行
 * 主要用于检测系统中安装的应用程序和系统状态
 * @param cmd 要执行的Shell命令
 * @return 命令执行的输出结果字符串
 */
string runShell(string cmd){
    string ret = "";

    // 创建管道用于进程间通信
    int pipefd[2];
    pid_t pid;
    char buffer[128];

    // 创建管道，pipefd[0]用于读取，pipefd[1]用于写入
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return ret;
    }

    // 创建子进程
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return ret;
    }

    if (pid == 0) {  // 子进程
        // 关闭管道的读取端，只保留写入端
        close(pipefd[0]);
        // 将标准输出重定向到管道的写入端，这样命令的输出会写入管道
        dup2(pipefd[1], STDOUT_FILENO);
        // 执行Shell命令，使用sh -c来执行命令字符串
        execve("/system/bin/sh", (char* const[]){"sh", "-c", (char*)cmd.c_str(), nullptr}, nullptr);
        // 如果execve失败，输出错误信息并退出
        perror("execve");
        exit(1);
    } else {  // 父进程
        // 关闭管道的写入端，只保留读取端
        close(pipefd[1]);
        ssize_t n;
        // 从管道读取子进程的输出，直到没有更多数据
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';  // 确保字符串以 null 结尾
            ret += buffer;     // 将读取的数据添加到结果字符串中
        }
        // 关闭管道的读取端
        close(pipefd[0]);
    }

    return ret;
}
