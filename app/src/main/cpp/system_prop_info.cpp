//
// Created by lxz on 2025/6/6.
//

#include <sys/system_properties.h>
#include "system_prop_info.h"
#include <stdatomic.h>
#include <unistd.h>
#include "android/log.h"

#define LOGE(...)  __android_log_print(6, "lxz", __VA_ARGS__)

// 定义一个结构体来存储 value 和 serial
struct PropertyValue {
    std::string value;
    uint32_t serial;
    uint32_t serial_version;
};

#define SERIAL_DIRTY             (1u << 0)         // 第 0 位：dirty bit
#define SERIAL_VERSION_INC       (1u << 1)         // 每次修改，版本号递增
#define SERIAL_VALUE_LEN_SHIFT   24                // 高 8 位表示 value 的长度
#define SERIAL_VALUE_LEN_MASK    0xFF000000        // 提取 value 长度


struct prop_info_internal {
    uint32_t serial;
    char value[PROP_VALUE_MAX];
    char name[PROP_NAME_MAX];
};

// 封装函数，返回以 name 为键的 map
std::map<std::string, PropertyValue> getAllSystemProperties() {
    std::map<std::string, PropertyValue> properties;
    __system_property_foreach([](const prop_info* pi, void* cookie) {

        auto properties = reinterpret_cast<std::map<std::string, PropertyValue> *>(cookie);

        const prop_info_internal* internal = reinterpret_cast<const prop_info_internal*>(pi);

        std::string name(internal->name);
        std::string value(internal->value);
        uint32_t serial = internal->serial;

        uint32_t dirty = serial & SERIAL_DIRTY;
        uint32_t value_len = (serial & SERIAL_VALUE_LEN_MASK) >> SERIAL_VALUE_LEN_SHIFT;
        uint32_t version = (serial & ~SERIAL_DIRTY & ~SERIAL_VALUE_LEN_MASK);  // 去掉 dirty 和 value_len 部分
        properties->emplace(name, PropertyValue{value, serial, version});

        LOGE("properties %s %s %x", name.c_str(), value.c_str(), serial);
        sleep(0);

    }, &properties);
    return properties;
}



std::map<std::string, std::string> get_system_prop_info() {
    std::map<std::string, std::string> info;
    auto properties = getAllSystemProperties();

    std::vector<std::string> prop_list{
            "ro.secure",
            "ro.debuggable",
            "ro.boot.flash.locked",
            "ro.dalvik.vm.native.bridge",
            "ro.boot.vbmeta.device_state",
            "ro.boot.verifiedbootstate",
            "ro.boot.veritymode",
            "ro.boot.verifiedbootstate",
            "ro.build.tags",
            "ro.build.type",
    };

    std::map<std::string, std::string> prop_map{
            {"ro.secure",                   "1"},
            {"ro.debuggable",               "0"},
            {"ro.boot.flash.locked",        "1"},
            {"ro.dalvik.vm.native.bridge",  "0"},
            {"ro.boot.vbmeta.device_state", "locked"},
            {"ro.boot.verifiedbootstate",   "green"},
            {"ro.boot.veritymode",          "enforcing"},
            {"ro.boot.verifiedbootstate",   "green"},
            {"ro.build.tags",               "release-keys"},
            {"ro.build.type",               "user"},
            {"init.svc.adbd",               "stopped"},
            {"persist.sys.usb.config",      "none"},
            {"persist.security.adbinput",      "0"},
    };

    for(const auto& [key, value] : prop_map){
        if(properties.find(key.c_str()) != properties.end()) {
            if(properties[key.c_str()].value != value) {
                char buffer[100] = {0};
                sprintf(buffer, "value[%s]", properties[key.c_str()].value.c_str());
                info[key.c_str()] = buffer;
            }
        }
    }

    for(std::string key : prop_list){
        if(properties[key.c_str()].serial_version != 0){
            char buffer[100] = {0};
            sprintf(buffer, "serial[%d]", properties[key.c_str()].serial_version);
            info[key.c_str()] = buffer;
        }
    }

//    for(const auto& [key, value] : properties){
//        LOGE("%s", key.c_str());
//        if(strstr(key.c_str(), "sim") != 0) {
//            char buffer[100] = {0};
//            sprintf(buffer, "value[%s]", properties[key.c_str()].value.c_str());
//            info[key.c_str()] = buffer;
//        }
//    }

    return info;
}
