//
// Created by lxz on 2025/8/6.
//

#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"

void __attribute__((constructor)) init_(void){
    LOGI("zStdTest init - Starting comprehensive tests");

    // ========== STRING TESTS ==========
    LOGI("=== String Tests ===");
    
    // Basic string construction
    string str1 = "Hello World";
    LOGI("String 1: %s, length: %zu", str1.c_str(), str1.length());
    
    string str2("Test String");
    LOGI("String 2: %s, length: %zu", str2.c_str(), str2.length());
    
    // String concatenation
    string str3 = str1 + " " + str2;
    LOGI("Concatenated string: %s, length: %zu", str3.c_str(), str3.length());
    
    // String comparison
    LOGI("str1 == str2: %s", str1 == str2 ? "true" : "false");
    LOGI("str1 < str2: %s", str1 < str2 ? "true" : "false");
    
    // String operations
    string str4 = str1.substr(0, 5);
    LOGI("Substring (0,5): %s", str4.c_str());
    
    size_t pos = str1.find("World");
    LOGI("Find 'World' at position: %zu", pos);
    
    // String iteration
    LOGI("String characters: ");
    for (char c : str1) {
        LOGI("  %c", c);
    }
    
    // String modification
    str1 += " Modified";
    LOGI("Modified string: %s", str1.c_str());
    
    // ========== VECTOR TESTS ==========
    LOGI("=== Vector Tests ===");
    
    // Basic vector construction
    vector<int> vec1;
    LOGI("Empty vector size: %zu", vec1.size());
    
    // Adding elements
    vec1.push_back(10);
    vec1.push_back(20);
    vec1.push_back(30);
    LOGI("Vector after push_back: size=%zu", vec1.size());
    
    // Vector initialization
    vector<int> vec2;
    vec2.push_back(1);
    vec2.push_back(2);
    vec2.push_back(3);
    vec2.push_back(4);
    vec2.push_back(5);
    LOGI("Vector with push_back: size=%zu", vec2.size());
    
    // Vector iteration
    LOGI("Vector elements:");
    for (size_t i = 0; i < vec2.size(); ++i) {
        LOGI("  vec2[%zu] = %d", i, vec2[i]);
    }
    
    // Vector operations
    vec2.insert(vec2.begin() + 2, 99);
    LOGI("After insert at position 2: size=%zu", vec2.size());
    
    vec2.erase(vec2.begin() + 1);
    LOGI("After erase at position 1: size=%zu", vec2.size());
    
    // Vector of strings
    vector<string> strVec;
    strVec.push_back("First");
    strVec.push_back("Second");
    strVec.push_back("Third");
    LOGI("String vector size: %zu", strVec.size());
    
    for (const auto& s : strVec) {
        LOGI("  String: %s", s.c_str());
    }
    
    // ========== MAP TESTS ==========
    LOGI("=== Map Tests ===");
    
    // Basic map construction
    map<string, int> map1;
    LOGI("Empty map size: %zu", map1.size());
    
    // Inserting elements
    map1.insert(pair<string, int>("apple", 1));
    map1.insert(pair<string, int>("banana", 2));
    map1.insert(pair<string, int>("cherry", 3));
    LOGI("Map after insert: size=%zu", map1.size());
    
    // Map initialization
    map<string, string> map2;
    map2.insert(pair<string, string>("key1", "value1"));
    map2.insert(pair<string, string>("key2", "value2"));
    map2.insert(pair<string, string>("key3", "value3"));
    LOGI("Map with insert: size=%zu", map2.size());
    
    // Map iteration
    LOGI("Map elements:");
    for (const auto& pair : map2) {
        LOGI("  %s -> %s", pair.first.c_str(), pair.second.c_str());
    }
    
    // Map access
    map1["orange"] = 4;
    LOGI("After operator[]: map1 size=%zu", map1.size());
    LOGI("map1['apple'] = %d", map1["apple"]);
    
    // Map lookup
    auto it = map1.find("banana");
    if (it != map1.end()) {
        LOGI("Found 'banana': %d", it->second);
    }

    // Map with different types
    map<int, string> intMap;
    intMap[1] = "One";
    intMap[2] = "Two";
    intMap[3] = "Three";
    LOGI("Integer map size: %zu", intMap.size());
    
    for (const auto& pair : intMap) {
        LOGI("  %d -> %s", pair.first, pair.second.c_str());
    }
    
    // ========== COMPLEX TESTS ==========
    LOGI("=== Complex Tests ===");
    
    // Vector of maps
    vector<map<string, int>> vecOfMaps;
    map<string, int> map3;
    map3["a"] = 1;
    map3["b"] = 2;
    vecOfMaps.push_back(map3);
    
    map<string, int> map4;
    map4["x"] = 10;
    map4["y"] = 20;
    vecOfMaps.push_back(map4);
    
    LOGI("Vector of maps size: %zu", vecOfMaps.size());
    for (size_t i = 0; i < vecOfMaps.size(); ++i) {
        LOGI("  Map %zu:", i);
        for (const auto& pair : vecOfMaps[i]) {
            LOGI("    %s -> %d", pair.first.c_str(), pair.second);
        }
    }
    
    // Map of vectors
    map<string, vector<int>> mapOfVecs;
    
    vector<int> evenVec;
    evenVec.push_back(2);
    evenVec.push_back(4);
    evenVec.push_back(6);
    evenVec.push_back(8);
    mapOfVecs["even"] = evenVec;
    
    vector<int> oddVec;
    oddVec.push_back(1);
    oddVec.push_back(3);
    oddVec.push_back(5);
    oddVec.push_back(7);
    mapOfVecs["odd"] = oddVec;
    
    LOGI("Map of vectors:");
    for (const auto& pair : mapOfVecs) {
        LOGI("  %s: [", pair.first.c_str());
        for (size_t i = 0; i < pair.second.size(); ++i) {
            if (i > 0) LOGI(", ");
            LOGI("%d", pair.second[i]);
        }
        LOGI("]");
    }
    
    // String operations with vectors
    vector<string> words;
    words.push_back("Hello");
    words.push_back("World");
    words.push_back("Test");
    string combined;
    for (const auto& word : words) {
        if (!combined.empty()) combined += " ";
        combined += word;
    }
    LOGI("Combined string from vector: %s", combined.c_str());

    LOGI("zStdTest init - All tests completed");
}