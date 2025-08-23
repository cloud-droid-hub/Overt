//
// Created by Assistant on 2025/8/21.
// zTask 实现 - 任务类
//

#include "zTask.h"
#include <sstream>
#include <unistd.h>

// 静态任务计数器初始化
int zTask::s_taskCounter = 0;

/**
 * 构造函数
 */
zTask::zTask(const string& taskName, std::function<void()> taskFunction)
    : m_taskName(taskName)
    , m_taskFunction(taskFunction)
    , m_taskId(generateTaskId())
{
    LOGI("zTask: Creating task '%s' with ID '%s'", m_taskName.c_str(), m_taskId.c_str());
    
    if (!m_taskFunction) {
        LOGE("zTask: Invalid task function for task '%s'", m_taskName.c_str());
    }
}

/**
 * 构造函数 - 使用默认任务函数
 */
zTask::zTask(const string& taskName)
    : m_taskName(taskName)
    , m_taskId(generateTaskId())
{
    LOGI("zTask: Creating task '%s' with ID '%s' (using default task function)", m_taskName.c_str(), m_taskId.c_str());
    
    // 设置默认任务函数
    m_taskFunction = [this]() {
        this->defaultTaskFunction();
    };
}

/**
 * 默认任务函数
 */
void zTask::defaultTaskFunction() {
    LOGI("zTask: Starting default task execution for '%s'", m_taskName.c_str());
    
    for(int i = 0; i < 5; i++) {
        LOGI("zTask: '%s' is executing step %d/5", m_taskName.c_str(), i + 1);
        usleep(500000); // 500ms
    }
    
    LOGI("zTask: Default task execution completed for '%s'", m_taskName.c_str());
}

/**
 * 析构函数
 */
zTask::~zTask() {
    LOGI("zTask: Destroying task '%s' (ID: %s)", m_taskName.c_str(), m_taskId.c_str());
}

/**
 * 执行任务
 */
void zTask::execute() {
    if (!m_taskFunction) {
        LOGE("zTask: Cannot execute task '%s' - task function is null", m_taskName.c_str());
        return;
    }
    
    LOGI("zTask: Executing task '%s' (ID: %s)", m_taskName.c_str(), m_taskId.c_str());
    try {
        m_taskFunction();
        LOGI("zTask: Task '%s' (ID: %s) execution completed successfully", m_taskName.c_str(), m_taskId.c_str());
    } catch (...) {
        LOGE("zTask: Task '%s' (ID: %s) execution failed with exception", m_taskName.c_str(), m_taskId.c_str());
    }
}

/**
 * 生成唯一任务ID
 */
string zTask::generateTaskId() {
    string TaskId = "task_" + to_string(++s_taskCounter);
    return TaskId;
}
