//
// Created by lxz on 2025/6/6.
//

#include <sys/system_properties.h>
#include <unistd.h>

#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zStdUtil.h"

#include "zSystemPropInfo.h"

/**
 * 系统属性值结构体
 * 存储属性的值、序列号和版本信息
 */
struct PropertyValue {
    string value;           // 属性值
    uint32_t serial;        // 序列号
    uint32_t serial_version; // 版本号
};

// 系统属性序列号相关常量定义
#define SERIAL_DIRTY             (1u << 0)         // 第 0 位：dirty bit
#define SERIAL_VERSION_INC       (1u << 1)         // 每次修改，版本号递增
#define SERIAL_VALUE_LEN_SHIFT   24                // 高 8 位表示 value 的长度
#define SERIAL_VALUE_LEN_MASK    0xFF000000        // 提取 value 长度

/**
 * 系统属性内部结构体
 * 对应Android系统属性存储的内部结构
 */
struct prop_info_internal {
    uint32_t serial;                    // 序列号
    char value[PROP_VALUE_MAX];         // 属性值
    char name[PROP_NAME_MAX];           // 属性名
};

/**
 * 获取所有系统属性
 * 遍历系统属性表，收集所有属性的信息
 * @return 包含所有系统属性的Map
 */
map<string, PropertyValue> getAllSystemProperties() {
    LOGD("getAllSystemProperties called");
    map<string, PropertyValue> properties;
    
    // 使用系统API遍历所有属性
    __system_property_foreach([](const prop_info* pi, void* cookie) {
        auto properties = reinterpret_cast<map<string, PropertyValue> *>(cookie);
        const prop_info_internal* internal = reinterpret_cast<const prop_info_internal*>(pi);

        string name(internal->name);
        string value(internal->value);
        uint32_t serial = internal->serial;

        // 解析序列号的各个组成部分
        uint32_t dirty = serial & SERIAL_DIRTY;                                    // dirty标志
        uint32_t value_len = (serial & SERIAL_VALUE_LEN_MASK) >> SERIAL_VALUE_LEN_SHIFT;  // 值长度
        uint32_t version = (serial & ~SERIAL_DIRTY & ~SERIAL_VALUE_LEN_MASK);     // 版本号
        
        // 存储属性信息
        properties->emplace(name, PropertyValue{value, serial, version});

        LOGD("properties %s %s %x", name.c_str(), value.c_str(), serial);
        sleep(0); // 让出CPU时间片

    }, &properties);
    
    LOGI("getAllSystemProperties finished, found %zu properties", properties.size());
    return properties;
}

/**
 * 获取系统属性信息
 * 检测关键系统属性的值是否正确，如ro.secure、ro.debuggable等
 * 这些属性的异常值通常表明系统已被修改或Root
 * @return 包含检测结果的Map，格式：{属性名 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_system_prop_info() {
    LOGD("get_system_prop_info called");
    map<string, map<string, string>> info;
    
    // 获取所有系统属性
    auto properties = getAllSystemProperties();
    LOGI("Got %zu properties", properties.size());

    // 定义需要检查的关键属性列表
    vector<string> prop_list{
            "ro.secure",                    // 安全标志
            "ro.debuggable",                // 可调试标志
            "ro.boot.flash.locked",         // 启动分区锁定状态
            "ro.dalvik.vm.native.bridge",   // Dalvik原生桥接
            "ro.boot.vbmeta.device_state",  // 设备状态
            "ro.boot.verifiedbootstate",    // 验证启动状态
            "ro.boot.veritymode",           // 验证模式
            "ro.boot.verifiedbootstate",    // 验证启动状态
            "ro.build.tags",                // 构建标签
            "ro.build.type",                // 构建类型
    };

    // 定义属性名和期望值的映射关系
    map<string, vector<string>> prop_map{
            {"ro.secure",                   {"1"}},              // 安全模式应为1
            {"ro.debuggable",               {"0"}},              // 调试模式应为0
            {"ro.boot.flash.locked",        {"1"}},              // 启动分区应锁定
            {"ro.dalvik.vm.native.bridge",  {"0"}},              // 原生桥接应关闭
            {"ro.boot.vbmeta.device_state", {"locked"}},         // 设备状态应锁定
            {"ro.boot.verifiedbootstate",   {"green"}},          // 验证启动状态应为绿色
            {"ro.boot.veritymode",          {"enforcing"}},      // 验证模式应强制
            {"ro.boot.verifiedbootstate",   {"green"}},          // 验证启动状态应为绿色
            {"ro.build.tags",               {"release-keys"}},   // 构建标签应为发布密钥
            {"ro.build.type",               {"user"}},           // 构建类型应为用户版
            {"init.svc.adbd",               {"stopped"}},        // ADB服务应停止
            {"persist.sys.usb.config",      {"mtp", "ptp", "none", ""}},  // USB配置
            {"persist.security.adbinput",   {"0"}}               // ADB输入应关闭
    };

    // 检查属性值是否正确
    for(const auto& [key, value] : prop_map){
        LOGD("Checking property: %s", key.c_str());
        if(properties.find(key) == properties.end()) {
            LOGD("Property not found: %s", key.c_str());
            continue;
        }

        // 检查属性值是否在期望值列表中
        bool flag = false;
        for(const auto& v: value){
            if(v == properties[key.c_str()].value){
                flag = true;
                break;
            }
        }
        
        // 如果值不在期望列表中，标记为错误
        if(!flag){
            string buffer = string_format(":value[%s]", properties[key.c_str()].value.c_str());
            info[key+buffer]["risk"] = "error";
            info[key+buffer]["explain"] = "value is not correct";
        }
    }

    // 检查关键属性的版本号是否为0（表示未被修改）
    for(const string& key : prop_list){
        if(properties.find(key) != properties.end()) {
            if(properties[key.c_str()].serial_version != 0){
                string buffer = string_format(":serial[%d]", properties[key.c_str()].serial_version);
                info[key+buffer]["risk"] = "error";
                info[key+buffer]["explain"] = "serial_version is not 0";
            }
        }
    }

    return info;
}
