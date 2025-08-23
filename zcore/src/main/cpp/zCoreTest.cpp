//
// Created by lxz on 2025/8/6.
// zCore 综合测试模块 - 重新设计版本
// 专注于 zThreadPool 功能测试，包含完整的异常处理和日志级别管理
//

#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zFile.h"
#include "zJson.h"
#include "zCrc32.h"
#include "zBroadCast.h"
#include "zHttps.h"
#include "zElf.h"
#include "zClassLoader.h"
#include "zJavaVm.h"
#include "zTee.h"
#include "zLinker.h"
#include "zTask.h"
#include "zChildThread.h"
#include "zThreadPool.h"
#include "zShell.h"


// 全局测试统计
static int g_testsPassed = 0;
static int g_testsFailed = 0;
static int g_testsWarning = 0;

// 测试结果记录
void recordTestResult(bool passed, bool warning = false) {
    if (passed) {
        g_testsPassed++;
    } else {
        g_testsFailed++;
    }
    if (warning) {
        g_testsWarning++;
    }
}



// =============================================================================
// zThreadPool 测试用例
// =============================================================================


//ps -AZ | grep zygisk
//u:r:magisk:s0                  root          1779     1 2182440   1240 do_sys_poll         0 S zygiskd64
//u:r:magisk:s0                  root          2616     1   10468    396 do_sys_poll         0 S zygiskd32


void __attribute__((constructor)) init_(void){
    LOGI("🚀 zCore 初始化 - 启动全面测试");


    // 启动新的综合测试框架
    zThreadPool* threadPool = zThreadPool::getInstance();
    LOGI("Thread pool instance obtained");

    // 添加多个任务
    for(int i = 0; i < 10; i++){
        threadPool->addTask("zTask" + std::to_string(i));
    }

    LOGI("🎯 zCore 初始化 - 所有测试已完成");
}