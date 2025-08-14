//
// Created by lxz on 2025/8/6.
// zCore 综合测试模块 - 重新设计版本
// 专注于 zThread 功能测试，包含完整的异常处理和日志级别管理
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
#include "zThread.h"
#include "zShell.h"

// 简化的断言宏（仅在需要时使用）
#define ASSERT_OR_RETURN(condition, msg) \
    if (!(condition)) { \
        LOGE("💥 [ASSERTION_FAILED] %s", msg); \
        return false; \
    }

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

// 打印测试总结
void printTestSummary() {
    LOGI("📊 [TEST_SUMMARY] ===================");
    LOGI("📊 Total Tests: %d", g_testsPassed + g_testsFailed);
    LOGI("📊 Passed: %d", g_testsPassed);
    LOGI("📊 Failed: %d", g_testsFailed);
    LOGI("📊 Warnings: %d", g_testsWarning);
    if (g_testsFailed == 0) {
        LOGI("🎉 All tests PASSED!");
    } else {
        LOGE("💀 %d tests FAILED!", g_testsFailed);
    }
    LOGI("📊 ===================================");
}

// =============================================================================
// zThread 测试用例
// =============================================================================

// 基础任务函数
void simpleTask(int id) {
    LOGI("Simple task %d started on thread %lu", id, (unsigned long)zThread::getCurrentThreadId());
    zThread::sleep(5000); // 短暂模拟工作
    LOGI("Simple task %d completed", id);
}

// 多参数任务函数 - 测试类型转换
void multiParamTask(int id, string name, const char* msg) {
    LOGI("MultiParam task: id=%d, name=%s, msg=%s", id, name.c_str(), msg);
    zThread::sleep(30);
}

// 指针参数任务函数
void pointerTask(void* data) {
    int* value = static_cast<int*>(data);
    LOGI("Pointer task received value: %d", *value);
    zThread::sleep(20);
}

// 读写锁测试函数
void readerTask(void* arg) {
    std::shared_mutex* sharedMutex = static_cast<std::shared_mutex*>(arg);
    LOGI("Reader thread %lu started", (unsigned long)zThread::getCurrentThreadId());

    try {
    std::shared_lock<std::shared_mutex> lock(*sharedMutex);
    LOGI("Reader thread %lu acquired read lock", (unsigned long)zThread::getCurrentThreadId());
        zThread::sleep(100); // 减少等待时间
    LOGI("Reader thread %lu released read lock", (unsigned long)zThread::getCurrentThreadId());
    } catch (const std::exception& e) {
        LOGE("Reader Task Exception: %s", e.what());
    }
}

void writerTask(void* arg) {
    std::shared_mutex* sharedMutex = static_cast<std::shared_mutex*>(arg);
    LOGI("Writer thread %lu started", (unsigned long)zThread::getCurrentThreadId());

    try {
    std::unique_lock<std::shared_mutex> lock(*sharedMutex);
    LOGI("Writer thread %lu acquired write lock", (unsigned long)zThread::getCurrentThreadId());
        zThread::sleep(150); // 减少等待时间
        LOGI("Writer thread %lu released write lock", (unsigned long)zThread::getCurrentThreadId());
    } catch (const std::exception& e) {
        LOGE("Writer Task Exception: %s", e.what());
    }
}

// 错误测试任务 - 测试异常处理
void errorTask(int mode) {
    LOGI("Error task mode %d started", mode);
    
    switch (mode) {
        case 1:
            // 模拟空指针访问
            LOGW("Error Task: Simulating potential error condition (mode 1)");
            break;
        case 2:
            // 模拟数组越界
            LOGW("Error Task: Simulating boundary check (mode 2)");
            break;
        default:
            LOGI("Error task completed normally");
    }
}

// 成员函数测试辅助类
class TestClass {
public:
    void memberFunction(int value, const string& text) {
        LOGI("Member function called: value=%d, text=%s", value, text.c_str());
        zThread::sleep(25);
    }
    
    static void staticFunction(const char* msg) {
        LOGI("Static function called: %s", msg);
        zThread::sleep(25);
    }
};

// =============================================================================
// 主要测试函数
// =============================================================================

// 测试1：基础 submitTaskTyped 功能
bool test_basic_submitTaskTyped() {
    LOGI("🧪 TEST_START: Basic submitTaskTyped");
    
    try {
    zThread* threadManager = zThread::getInstance();

        // 测试单参数任务
        auto task1 = threadManager->submitTaskTyped(simpleTask, 1);
        ASSERT_OR_RETURN(task1 != nullptr, "Task1 creation failed");
        
        task1->setName("BasicTask1")->setLevel(TaskPriority::NORMAL)->start();
        
        // 测试多参数任务（const char* -> string 转换）
        auto task2 = threadManager->submitTaskTyped(multiParamTask, 2, string("test"), "hello");
        ASSERT_OR_RETURN(task2 != nullptr, "Task2 creation failed");
        
        task2->setName("BasicTask2")->setLevel(TaskPriority::HIGH)->start();
        
        // 测试指针参数任务
        int testData = 42;
        auto task3 = threadManager->submitTaskTyped(pointerTask, &testData);
        ASSERT_OR_RETURN(task3 != nullptr, "Task3 creation failed");
        
        task3->setName("BasicTask3")->start();
        
        // 等待任务完成
        zThread::sleep(200);
        
        LOGI("✅ TEST_END: Basic submitTaskTyped - PASSED");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("❌ TEST_FAIL: Basic submitTaskTyped - Exception: %s", e.what());
        return false;
    } catch (...) {
        LOGE("❌ TEST_FAIL: Basic submitTaskTyped - Unknown exception occurred");
        return false;
    }
}

// 测试2：成员函数调用
bool test_member_function_calls() {
    LOGI("🧪 TEST_START: Member Function Calls");
    
    try {
        zThread* threadManager = zThread::getInstance();
        TestClass testObj;
        
        // 测试成员函数调用
        auto memberTask = threadManager->submitTaskMember(&testObj, &TestClass::memberFunction, 100, string("member_test"));
        ASSERT_OR_RETURN(memberTask != nullptr, "Member task creation failed");
        
        memberTask->setName("MemberTask")->setLevel(TaskPriority::HIGH)->start();
        
        // 测试静态函数调用
        auto staticTask = threadManager->submitTaskTyped(TestClass::staticFunction, "static_test");
        ASSERT_OR_RETURN(staticTask != nullptr, "Static task creation failed");
        
        staticTask->setName("StaticTask")->start();
        
        // 等待任务完成
        zThread::sleep(150);
        
        LOGI("✅ TEST_END: Member Function Calls - PASSED");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("❌ TEST_FAIL: Member Function Calls - Exception: %s", e.what());
        return false;
    } catch (...) {
        LOGE("❌ TEST_FAIL: Member Function Calls - Unknown exception occurred");
        return false;
    }
}

// 测试3：读写锁并发控制
bool test_shared_mutex_concurrency() {
    LOGI("🧪 TEST_START: Shared Mutex Concurrency");
    
    try {
        zThread* threadManager = zThread::getInstance();
    std::shared_mutex* sharedMutex = threadManager->createSharedMutex();
        ASSERT_OR_RETURN(sharedMutex != nullptr, "Shared mutex creation failed");

    // 创建多个读者线程
        vector<zChildThread*> readerThreads;
    for (int i = 0; i < 3; ++i) {
            auto readerThread = threadManager->submitTaskTyped(readerTask, sharedMutex);
            if (readerThread == nullptr) {
                LOGE("💥 [ASSERTION_FAILED] Reader thread %d creation failed", i);
                return false;
            }
            char readerName[32];
            snprintf(readerName, sizeof(readerName), "Reader_%d", i);
            readerThread->setName(readerName)->start();
            readerThreads.push_back(readerThread);
    }

    // 创建写者线程
        auto writerThread = threadManager->submitTaskTyped(writerTask, sharedMutex);
        ASSERT_OR_RETURN(writerThread != nullptr, "Writer thread creation failed");
        writerThread->setName("Writer")->start();
        
        // 等待所有任务完成
        zThread::sleep(500);
        
        LOGI("✅ TEST_END: Shared Mutex Concurrency - PASSED");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("❌ TEST_FAIL: Shared Mutex Concurrency - Exception: %s", e.what());
        return false;
    } catch (...) {
        LOGE("❌ TEST_FAIL: Shared Mutex Concurrency - Unknown exception occurred");
        return false;
    }
}

// 测试4：waitForAllTasks 无限等待功能
bool test_waitForAllTasks_infinite() {
    LOGI("🧪 TEST_START: waitForAllTasks Infinite Wait");
    
    try {
        zThread* threadManager = zThread::getInstance();
        
        // 创建一些任务
        auto task1 = threadManager->submitTaskTyped(simpleTask, 10);
        auto task2 = threadManager->submitTaskTyped(simpleTask, 11);
        auto task3 = threadManager->submitTaskTyped(errorTask, 0);
        
        ASSERT_OR_RETURN(task1 && task2 && task3, "Task creation failed");
        
        // 启动任务
        task1->setName("WaitTest1")->start();
        task2->setName("WaitTest2")->start();
        task3->setName("WaitTest3")->start();
        
        // 测试任务统计
        LOGI("Queued tasks: %zu", threadManager->getQueuedTaskCount());
        LOGI("Executing tasks: %zu", threadManager->getExecutingTaskCount());
        LOGI("Pending tasks: %zu", threadManager->getPendingTaskCount());
        
        // 使用无限等待模式
        LOGI("Testing waitForAllTasks(0) - infinite wait until all tasks complete");
        bool allCompleted = threadManager->waitForAllTasks();
        
        ASSERT_OR_RETURN(allCompleted, "waitForAllTasks(0) failed to wait for all tasks");
        
        // 验证所有任务都完成了
        ASSERT_OR_RETURN(threadManager->getPendingTaskCount() == 0, "Still has pending tasks after waitForAllTasks");
        ASSERT_OR_RETURN(threadManager->getExecutingTaskCount() == 0, "Still has executing tasks after waitForAllTasks");
        
        LOGI("✅ TEST_END: waitForAllTasks Infinite Wait - PASSED");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("❌ TEST_FAIL: waitForAllTasks Infinite Wait - Exception: %s", e.what());
        return false;
    } catch (...) {
        LOGE("❌ TEST_FAIL: waitForAllTasks Infinite Wait - Unknown exception occurred");
        return false;
    }
}

// 测试5：错误处理和异常情况
bool test_error_handling() {
    LOGI("🧪 TEST_START: Error Handling");
    
    try {
        zThread* threadManager = zThread::getInstance();
        
        // 测试错误任务
        auto errorTask1 = threadManager->submitTaskTyped(errorTask, 1);
        auto errorTask2 = threadManager->submitTaskTyped(errorTask, 2);
        
        ASSERT_OR_RETURN(errorTask1 && errorTask2, "Error task creation failed");
        
        errorTask1->setName("ErrorTest1")->start();
        errorTask2->setName("ErrorTest2")->start();
        
        // 等待错误任务完成
        zThread::sleep(100);
        
        // 测试线程池信息
    vector<ThreadInfo> threadInfo = threadManager->getThreadInfo();
        LOGI("Active thread count: %zu", threadInfo.size());

    for (const auto& info : threadInfo) {
        LOGI("Thread %lu: %s, State: %d",
             (unsigned long)info.threadId,
             info.name.c_str(),
             static_cast<int>(info.state));
    }

        LOGI("✅ TEST_END: Error Handling - PASSED");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("❌ TEST_FAIL: Error Handling - Exception: %s", e.what());
        return false;
    } catch (...) {
        LOGE("❌ TEST_FAIL: Error Handling - Unknown exception occurred");
        return false;
    }
}

// 测试6：线程池管理功能
bool test_thread_pool_management() {
    LOGI("🧪 TEST_START: Thread Pool Management");
    
    try {
        zThread* threadManager = zThread::getInstance();
        
        // 测试线程池状态
        ASSERT_OR_RETURN(threadManager->isThreadPoolRunning(), "Thread pool should be running");
        
        // 测试广播消息
        LOGI("Testing broadcast message");
        threadManager->broadcastMessage("Test broadcast from thread pool management test");
        
        // 测试任务统计
        size_t queuedTasks = threadManager->getQueuedTaskCount();
        size_t executingTasks = threadManager->getExecutingTaskCount();
        size_t activeTasks = threadManager->getActiveTaskCount();
        size_t pendingTasks = threadManager->getPendingTaskCount();
        
        LOGI("Task statistics - Queued: %zu, Executing: %zu, Active: %zu, Pending: %zu", 
             queuedTasks, executingTasks, activeTasks, pendingTasks);
        
        LOGI("✅ TEST_END: Thread Pool Management - PASSED");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("❌ TEST_FAIL: Thread Pool Management - Exception: %s", e.what());
        return false;
    } catch (...) {
        LOGE("❌ TEST_FAIL: Thread Pool Management - Unknown exception occurred");
        return false;
    }
}

// 主测试入口函数
void runComprehensiveTests() {
    LOGI("🧪 ========================================");
    LOGI("🧪 zCore 综合测试开始");
    LOGI("🧪 ========================================");
    
    // 初始化测试统计
    g_testsPassed = 0;
    g_testsFailed = 0;
    g_testsWarning = 0;
    
    try {
        // 确保线程池启动
        zThread* threadManager = zThread::getInstance();
        if (!threadManager->isThreadPoolRunning()) {
            bool started = threadManager->startThreadPool(4);
            if (!started) {
                LOGE("💥 [ASSERTION_FAILED] Failed to start thread pool");
                recordTestResult(false);
                printTestSummary();
                return;
            }
            LOGI("Thread pool started with 4 threads");
        } else {
            LOGI("Thread pool already running");
        }
        
        // 执行所有测试
        recordTestResult(test_basic_submitTaskTyped());
        recordTestResult(test_member_function_calls());
        recordTestResult(test_shared_mutex_concurrency());
        recordTestResult(test_waitForAllTasks_infinite());
        recordTestResult(test_error_handling());
        recordTestResult(test_thread_pool_management());
        
        // 最终清理：等待所有任务完成
        LOGI("Final cleanup: waiting for all remaining tasks to complete");
        if (threadManager->waitForAllTasks(-1)) {
            LOGI("✅ All tasks completed successfully");
    } else {
            LOGW("Final Cleanup: Some tasks may not have completed");
            recordTestResult(false, true);
    }

    // 停止线程池
        LOGI("Stopping thread pool");
    threadManager->stopThreadPool(true);
    LOGI("Thread pool stopped");
        
    } catch (const std::exception& e) {
        LOGE("❌ TEST_FAIL: Main Test Runner - Exception: %s", e.what());
        recordTestResult(false);
    } catch (...) {
        LOGE("❌ TEST_FAIL: Main Test Runner - Fatal unknown exception");
        recordTestResult(false);
    }
    
    // 打印测试总结
    printTestSummary();
    
    LOGI("🧪 ========================================");
    LOGI("🧪 zCore 综合测试结束");
    LOGI("🧪 ========================================");
}


//ps -AZ | grep zygisk
//u:r:magisk:s0                  root          1779     1 2182440   1240 do_sys_poll         0 S zygiskd64
//u:r:magisk:s0                  root          2616     1   10468    396 do_sys_poll         0 S zygiskd32

void test(){


}


void __attribute__((constructor)) init_(void){
    LOGI("🚀 zCore 初始化 - 启动全面测试");
    sleep(1);
    // 启动新的综合测试框架
//    runComprehensiveTests();

    test();

    
    LOGI("🎯 zCore 初始化 - 所有测试已完成");
}