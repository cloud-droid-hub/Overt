//
// Created by Assistant on 2025/8/21.
// zTask 实现 - 任务类
//

#include "zTask.h"
#include <sstream>
#include <unistd.h>

/**
 * 静态任务计数器初始化
 * 用于生成唯一的任务ID，从0开始递增
 * 线程安全：在单线程环境中使用，无需额外保护
 */
int zTask::s_taskCounter = 0;

/**
 * 构造函数 - 使用自定义任务函数
 * 
 * 功能说明：
 * 1. 创建任务实例，使用外部提供的执行函数
 * 2. 自动生成唯一任务ID
 * 3. 验证任务函数的有效性
 * 
 * 使用场景：
 * - 需要执行特定逻辑的任务
 * - 复杂的异步操作
 * - 自定义的任务处理流程
 * 
 * @param taskName 任务名称，用于标识和日志记录
 * @param taskFunction 任务执行函数，不能为空
 */
zTask::zTask(const string& taskName, std::function<void()> taskFunction)
    : m_taskName(taskName)
    , m_taskFunction(taskFunction)
    , m_taskId(generateTaskId())
{
    LOGI("zTask: Creating task '%s' with ID '%s'", m_taskName.c_str(), m_taskId.c_str());
    
    // 验证任务函数的有效性
    if (!m_taskFunction) {
        LOGE("zTask: Invalid task function for task '%s'", m_taskName.c_str());
    }
}

/**
 * 构造函数 - 使用默认任务函数
 * 
 * 功能说明：
 * 1. 创建任务实例，使用内置的默认执行函数
 * 2. 自动生成唯一任务ID
 * 3. 设置lambda表达式包装默认任务函数
 * 
 * 使用场景：
 * - 测试和演示目的
 * - 简单的周期性任务
 * - 不需要复杂逻辑的任务
 * 
 * @param taskName 任务名称，用于标识和日志记录
 */
zTask::zTask(const string& taskName)
    : m_taskName(taskName)
    , m_taskId(generateTaskId())
{
    LOGI("zTask: Creating task '%s' with ID '%s' (using default task function)", m_taskName.c_str(), m_taskId.c_str());
    
    // 设置默认任务函数
    // 使用lambda表达式包装defaultTaskFunction，保持this指针
    m_taskFunction = [this]() {
        this->defaultTaskFunction();
    };
}

/**
 * 默认任务函数
 * 
 * 功能说明：
 * 1. 提供简单的任务执行示例
 * 2. 模拟任务执行过程，包含多个步骤
 * 3. 每个步骤之间有500ms的延迟
 * 4. 记录详细的执行日志
 * 
 * 执行流程：
 * 1. 记录任务开始日志
 * 2. 循环执行5个步骤，每步间隔500ms
 * 3. 记录每个步骤的执行状态
 * 4. 记录任务完成日志
 * 
 * 使用场景：
 * - 测试任务调度系统
 * - 演示任务执行流程
 * - 作为其他任务的模板
 */
void zTask::defaultTaskFunction() {
    LOGI("zTask: Starting default task execution for '%s'", m_taskName.c_str());
    
    // 执行5个步骤，模拟任务处理过程
    for(int i = 0; i < 5; i++) {
        LOGI("zTask: '%s' is executing step %d/5", m_taskName.c_str(), i + 1);
        usleep(500000); // 500ms延迟，模拟实际工作
    }
    
    LOGI("zTask: Default task execution completed for '%s'", m_taskName.c_str());
}

/**
 * 析构函数
 * 
 * 功能说明：
 * 1. 清理任务资源
 * 2. 记录任务销毁日志
 * 3. 执行必要的清理工作
 * 
 * 注意事项：
 * - 不需要手动清理std::function，会自动析构
 * - 主要用于日志记录和调试
 */
zTask::~zTask() {
    LOGI("zTask: Destroying task '%s' (ID: %s)", m_taskName.c_str(), m_taskId.c_str());
}

/**
 * 执行任务
 * 
 * 功能说明：
 * 1. 验证任务函数的有效性
 * 2. 调用任务执行函数
 * 3. 处理执行过程中的异常
 * 4. 记录执行结果日志
 * 
 * 异常处理：
 * - 捕获所有类型的异常
 * - 记录异常信息到日志
 * - 确保程序不会因任务异常而崩溃
 * 
 * 线程安全：
 * - 可以在多线程环境中调用
 * - 每个任务实例独立执行
 */
void zTask::execute() {
    // 验证任务函数的有效性
    if (!m_taskFunction) {
        LOGE("zTask: Cannot execute task '%s' - task function is null", m_taskName.c_str());
        return;
    }
    
    LOGI("zTask: Executing task '%s' (ID: %s)", m_taskName.c_str(), m_taskId.c_str());
    
    try {
        // 执行任务函数
        m_taskFunction();
        LOGI("zTask: Task '%s' (ID: %s) execution completed successfully", m_taskName.c_str(), m_taskId.c_str());
    } catch (...) {
        // 捕获所有异常，防止程序崩溃
        LOGE("zTask: Task '%s' (ID: %s) execution failed with exception", m_taskName.c_str(), m_taskId.c_str());
    }
}

/**
 * 生成唯一任务ID
 * 
 * 功能说明：
 * 1. 使用静态计数器生成唯一ID
 * 2. 格式为"task_" + 递增数字
 * 3. 确保每个任务都有唯一标识
 * 
 * 实现细节：
 * - 使用前置递增操作符，确保从1开始
 * - 使用to_string转换数字为字符串
 * - 线程安全：在单线程环境中使用
 * 
 * @return 格式化的任务ID字符串
 */
string zTask::generateTaskId() {
    string TaskId = "task_" + to_string(++s_taskCounter);
    return TaskId;
}
