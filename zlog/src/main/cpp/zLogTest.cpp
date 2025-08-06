#include "zLog.h"

void __attribute__((constructor)) init_(void){
    LOGI("init_ start");

    // 1. 基本日志级别测试
    LOGV("Verbose log message - 详细日志信息");
    LOGD("Debug log message - 调试日志信息");
    LOGI("Info log message - 信息日志");
    LOGW("Warning log message - 警告日志");
    LOGE("Error log message - 错误日志");

    // 2. 字符串格式化测试
    LOGI("String format test: %s", "Hello World");
    LOGI("Number format test: %d", 42);
    LOGI("Float format test: %.2f", 3.14159);
    LOGI("Multiple format test: %s %d %.2f", "Test", 123, 2.718);

    // 3. 中文日志测试
    LOGI("中文日志测试 - Chinese log test");
    LOGW("警告信息 - Warning message");
    LOGE("错误信息 - Error message");

    // 4. 特殊字符测试
    LOGI("Special chars: !@#$%%^&*()_+-=[]{}|;':\",./<>?");
    LOGI("New line test:\nSecond line");
    LOGI("Tab test:\tTabbed content");

    // 5. 变量测试
    int intValue = 100;
    float floatValue = 3.14f;
    const char* stringValue = "Test String";
    LOGI("Variable test - int: %d, float: %.2f, string: %s", intValue, floatValue, stringValue);

    // 6. 数组测试
    int numbers[] = {1, 2, 3, 4, 5};
    LOGI("Array test - first: %d, last: %d", numbers[0], numbers[4]);

    // 7. 条件日志测试
    bool condition = true;
    if (condition) {
        LOGI("Condition is true - 条件为真");
    } else {
        LOGW("Condition is false - 条件为假");
    }

    // 8. 循环日志测试
    for (int i = 0; i < 3; i++) {
        LOGI("Loop iteration %d", i + 1);
    }

    // 9. 函数调用测试
    LOGI("Function: %s, Line: %d", __FUNCTION__, __LINE__);

    // 10. 性能测试 - 大量日志
    for (int i = 0; i < 10; i++) {
        LOGD("Performance test log %d", i);
    }

    // 11. 长字符串测试
    const char* longString = "This is a very long string that contains many characters to test how the logging system handles long messages. It should be able to display this without any issues.";
    LOGI("Long string test: %s", longString);

    // 12. 空字符串测试
    LOGI("Empty string test: '%s'", "");
    LOGI("Null pointer test: %s", (char*)nullptr);

    // 13. 不同数据类型测试
    unsigned int uintValue = 4294967295;
    long longValue = 9223372036854775807LL;
    double doubleValue = 2.718281828459045;
    LOGI("Data types - uint: %u, long: %ld, double: %.6f", uintValue, longValue, doubleValue);

    // 14. 十六进制和八进制测试
    int hexValue = 0xABCD;
    int octValue = 0777;
    LOGI("Hex test: 0x%X, Oct test: %o", hexValue, octValue);

    // 15. 指针测试
    void* ptr = (void*)0x12345678;
    LOGI("Pointer test: %p", ptr);

    // 16. 结构体测试（模拟）
    struct TestStruct {
        int id;
        const char* name;
    };
    TestStruct testStruct = {1, "TestStruct"};
    LOGI("Struct test - id: %d, name: %s", testStruct.id, testStruct.name);

    // 17. 错误场景测试
    LOGW("This is a warning message for testing");
    LOGE("This is an error message for testing");

    // 18. 多行日志测试
    LOGI("Multi-line log test:");
    LOGI("  Line 1: Basic information");
    LOGI("  Line 2: Additional details");
    LOGI("  Line 3: Final summary");

    // 19. 时间相关测试
    LOGI("Time-related test - current function: %s", __FUNCTION__);

    // 20. 边界值测试
    LOGI("Boundary test - max int: %d", 2147483647);
    LOGI("Boundary test - min int: %d", -2147483648);

    LOGI("init_ over");
}
