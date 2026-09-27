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
        if (properties == nullptr || pi == nullptr) {
            return;
        }

        __system_property_read_callback(
                pi,
                [](void* cb_cookie, const char* name, const char* value, uint32_t serial) {
                    auto props = reinterpret_cast<map<string, PropertyValue>*>(cb_cookie);
                    if (props == nullptr || name == nullptr) {
                        return;
                    }

                    string prop_name(name);
                    string prop_value = value == nullptr ? "" : string(value);

                    // 解析序列号的版本位（保留原有逻辑）
                    uint32_t version = (serial & ~SERIAL_DIRTY & ~SERIAL_VALUE_LEN_MASK);
                    props->emplace(prop_name, PropertyValue{prop_value, serial, version});

                    LOGD("properties %s %s %x", prop_name.c_str(), prop_value.c_str(), serial);
                },
                properties
        );
    }, &properties);
    
    LOGI("getAllSystemProperties finished, found %zu properties", properties.size());
    return properties;
}

/**
 * 获取系统属性信息(查杀分离 — 采集端)
 * 遍历所有系统属性并全量上报属性名、值、序列号版本(原始数据)，不做风险判定；
 * 关键属性期望值(prop_map)与serial_version!=0判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{属性名 -> {value: "属性值", serial: "版本"}}
 */
map<string, map<string, string>> get_system_prop_info() {
    LOGD("get_system_prop_info called");
    map<string, map<string, string>> info;

    // 获取所有系统属性
    auto properties = getAllSystemProperties();
    LOGI("Got %zu properties", properties.size());

    // 全量上报每个属性名、值、序列号版本(不内置过滤)
    for (const auto& entry : properties) {
        const string& key = entry.first;
        const PropertyValue& pv = entry.second;
        info[key]["value"] = pv.value;
        info[key]["serial"] = to_string(pv.serial_version);
    }

    LOGI("system_prop_info raw count=%zu", info.size());
    return info;
}
