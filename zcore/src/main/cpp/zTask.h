//
// Created by Assistant on 2025/8/21.
// zTask - 任务类，用于封装任务信息
//

#ifndef TESTSLEEP_ZTASK_H
#define TESTSLEEP_ZTASK_H

#include "functional"

#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zStdUtil.h"


/**
 * 任务类 - 封装任务信息和执行逻辑
 */
class zTask {
private:
    string m_taskName;                      // 任务名称
    std::function<void()> m_taskFunction;   // 任务执行函数
    string m_taskId;                        // 任务唯一ID
    static int s_taskCounter;               // 静态任务计数器

public:
    /**
     * 构造函数
     * @param taskName 任务名称
     * @param taskFunction 任务执行函数
     */
    zTask(const string& taskName, std::function<void()> taskFunction);
    
    /**
     * 构造函数 - 使用默认任务函数
     * @param taskName 任务名称
     */
    zTask(const string& taskName);
    
    /**
     * 模板构造函数 - 支持成员函数
     * @param taskName 任务名称
     * @param obj 对象指针
     * @param memberFunc 成员函数指针
     * @param args 函数参数
     */
    template<typename Obj, typename RetType, typename... FuncArgs, typename... Args>
    zTask(const string& taskName, Obj* obj, RetType (Obj::*memberFunc)(FuncArgs...), Args&&... args)
        : m_taskName(taskName)
        , m_taskId(generateTaskId())
    {
        LOGI("zTask: Creating task '%s' with ID '%s'", m_taskName.c_str(), m_taskId.c_str());
        
        // 检查对象和函数指针是否有效
        if (!obj || !memberFunc) {
            LOGE("zTask: Invalid object or member function pointer for task '%s'", m_taskName.c_str());
            m_taskFunction = nullptr;
            return;
        }
        
        // 创建包装 lambda 来调用成员函数
        m_taskFunction = [this, obj, memberFunc, args...]() {
            LOGI("zTask: Executing task '%s' (ID: %s)", m_taskName.c_str(), m_taskId.c_str());
            (obj->*memberFunc)(args...); // 调用成员函数
            LOGI("zTask: Task '%s' (ID: %s) execution completed", m_taskName.c_str(), m_taskId.c_str());
        };
    }
    
    /**
     * 析构函数
     */
    ~zTask();
    
    /**
     * 执行任务
     */
    void execute();
    
    /**
     * 获取任务名称
     */
    const string& getTaskName() const { return m_taskName; }
    
    /**
     * 获取任务ID
     */
    const string& getTaskId() const { return m_taskId; }
    
    /**
     * 获取任务函数
     */
    const std::function<void()>& getTaskFunction() const { return m_taskFunction; }
    
    /**
     * 检查任务是否有效
     */
    bool isValid() const { return m_taskFunction != nullptr; }

private:
    /**
     * 默认任务函数
     */
    void defaultTaskFunction();
    
    /**
     * 生成唯一任务ID
     */
    string generateTaskId();
};

#endif //TESTSLEEP_ZTASK_H
