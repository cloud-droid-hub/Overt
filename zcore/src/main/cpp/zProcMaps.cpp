//
// Created by liuxi on 2025/11/19.
//

#include "zProcMaps.h"
#include "zStdUtil.h"

zProcMaps::zProcMaps() {
    zFile maps = zFile("/proc/self/maps");
    vector<string> lines = maps.readAllLines();
    LOGI("Read %zu lines from file", lines.size());

    for(int i = 0; i < lines.size(); i++){
        // 过滤：只处理 .so 和 linker64
        if (!string_end_with(lines[i].c_str(), "linker64") &&
            !string_end_with(lines[i].c_str(), ".so")) {
            continue;
        }
        LOGV("maps[%d]: %s", i, lines[i].c_str());
    }

    // 使用局部变量构建 LibraryMapping，避免中间数据结构
    vector<LibraryMapping> temp_library_vector;

    // 一次遍历：解析并构建 LibraryMapping
    for (size_t i = 0; i < lines.size(); i++) {
        // 过滤：只处理 .so 和 linker64
        if (!string_end_with(lines[i].c_str(), "linker64") && 
            !string_end_with(lines[i].c_str(), ".so")) {
            continue;
        }

        // 解析行
        vector<string> parts = split_str(lines[i], ' ');
        if (parts.size() != 6) {
            LOGE("Line %zu: insufficient parts %s", i + 1, lines[i].c_str());
            continue;
        }

        // 解析地址范围
        vector<string> address_range_parts = split_str(parts[0], '-');
        if (address_range_parts.size() != 2) {
            continue;
        }

        MapSegment segment;
        segment.address_range_start = (void *) strtoul(address_range_parts[0].c_str(), nullptr, 16);
        segment.address_range_end = (void *) strtoul(address_range_parts[1].c_str(), nullptr, 16);
        segment.permissions = parts[1];
        segment.file_offset = parts[2];
        segment.device_major_minor = parts[3];
        segment.inode = parts[4];
        segment.file_path = parts[5];

        // 检查是否是新的 SO 映射（通过 ELF 头）
        bool is_new_so = false;
        if (string_start_with(segment.permissions.c_str(), "r")

            // 这里做个记录，有些 so 在 maps 中只有一行，但这个地址有可能并不能访问，
            // 如果用 memcmp 验 ELF 头，会导致崩溃，所以改为用 strstr 验 "p 00000000 "
            // && memcmp(segment.address_range_start, "\x7f""ELF", 4) == 0) {

            && strstr(lines[i].c_str(), "p 00000000 ")) {
            is_new_so = true;
        }

        if (is_new_so) {
            // 创建新的 LibraryMapping
            LibraryMapping library;
            library.address_range_start = segment.address_range_start;
            library.address_range_end = segment.address_range_end;
            library.file_path = segment.file_path;
            library.device_major_minor = segment.device_major_minor;
            library.inode = segment.inode;
            library.segments.push_back(segment);
            temp_library_vector.push_back(library);
        } else if (!temp_library_vector.empty()) {
            // 追加到最后一个 LibraryMapping（连续映射段）
            LibraryMapping &library = temp_library_vector.back();
            library.address_range_end = segment.address_range_end;
            library.segments.push_back(segment);
        }
    }

    // 构建 loaded_libraries：只保留第一个完整的映射（与 AOSP linker 行为一致）
    for (size_t i = 0; i < temp_library_vector.size(); i++) {
        // 先过滤：跳过段数小于 4 的库
        if (temp_library_vector[i].segments.size() < 4) continue;
        
        // 只保留第一个完整的映射（避免覆盖，与 AOSP linker 行为一致）
        if (loaded_libraries.find(temp_library_vector[i].file_path) == loaded_libraries.end()) {
            loaded_libraries[temp_library_vector[i].file_path] = temp_library_vector[i];
        }
    }
}

LibraryMapping* zProcMaps::find_so_by_name(string so_name) {
    for (auto it = loaded_libraries.begin(); it != loaded_libraries.end(); it++) {
        if(string_end_with(it->first.c_str(), so_name.c_str())){
            LOGI("Find so by name: %s", it->first.c_str());
            return &it->second;
        }
    }
    LOGE("Cannot find so by name: %s", so_name.c_str());
    return nullptr;
}




