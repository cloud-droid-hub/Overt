#include "map.h"
#include "zLog.h"

// 测试map的const方法
void testMapConstMethods() {
    nonstd::map<nonstd::string, int> testMap;
    
    // 插入一些测试数据
    testMap.insert(nonstd::make_pair("key1", 100));
    testMap.insert(nonstd::make_pair("key2", 200));
    
    // 测试const find方法
    const nonstd::map<nonstd::string, int>& constMap = testMap;
    auto it = constMap.find("key1");
    
    if (it != constMap.end()) {
        LOGE("Found key1: %d", it->second);
    } else {
        LOGE("Key1 not found");
    }
    
    // 测试const operator[]
    int value = constMap["key1"];
    LOGE("Value for key1: %d", value);
    
    // 测试const at方法
    int atValue = constMap.at("key2");
    LOGE("Value for key2 using at(): %d", atValue);
    
    // 测试contains方法
    bool hasKey1 = constMap.contains("key1");
    bool hasKey3 = constMap.contains("key3");
    LOGE("Contains key1: %s", hasKey1 ? "true" : "false");
    LOGE("Contains key3: %s", hasKey3 ? "true" : "false");
    
    // 测试count方法
    size_t count1 = constMap.count("key1");
    size_t count3 = constMap.count("key3");
    LOGE("Count of key1: %zu", count1);
    LOGE("Count of key3: %zu", count3);
} 