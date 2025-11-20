//
// Created by liuxi on 2025/11/19.
//

#include "zProcMaps.h"
#include "zStdUtil.h"


zProcMaps::zProcMaps() {

    zFile maps = zFile("/proc/self/maps");

    // 按行读取maps文件
    vector<string> lines = maps.readAllLines();
    LOGI("Read %zu lines from file", lines.size());

    // 解析每一行映射信息
    for (size_t i = 0; i < lines.size(); i++) {
        LOGD("Processing line %zu", i + 1);

        if (!string_end_with(lines[i].c_str(), "linker64") && !string_end_with(lines[i].c_str(), ".so")) continue;

        LOGD("Line %zu: %s", i + 1, lines[i].c_str());

        maps_line_t maps_line;

        // 按空格分割行内容
        vector<string> parts = split_str(lines[i], ' ');

        // 检查分割后的部分数量是否正确（应该有6个部分）
        if (parts.size() != 6) {
            LOGE("Line %zu: insufficient parts %s", i + 1, lines[i].c_str());
            continue;
        }

        // 解析地址范围（格式：start-end）
        vector<string> address_range_parts = split_str(parts[0], '-');
        if (address_range_parts.size() == 2) {
            // 将十六进制字符串转换为指针地址
            maps_line.address_range_start = (void *) strtoul(address_range_parts[0].c_str(), nullptr, 16);
            maps_line.address_range_end = (void *) strtoul(address_range_parts[1].c_str(), nullptr, 16);
        }

        // 解析其他字段
        maps_line.permissions = parts[1];           // 权限标志
        maps_line.file_offset = parts[2];           // 文件偏移
        maps_line.device_major_minor = parts[3];    // 设备号
        maps_line.inode = parts[4];                 // 节点号
        maps_line.file_path = parts[5];             // 文件路径

        LOGD("Address range: %p - %p permissions: %s file path: %s",
             maps_line.address_range_start, maps_line.address_range_end,
             maps_line.permissions.c_str(), maps_line.file_path.c_str());

        // 按文件路径分组存储映射信息

        maps_line_vector.push_back(maps_line);

        sleep(0); // 让出CPU时间片

    }

    for (size_t i = 0; i < maps_line_vector.size(); i++) {
        LOGI("%d %p - %p %s %s %s %s %s", i, maps_line_vector[i].address_range_start, maps_line_vector[i].address_range_end, maps_line_vector[i].permissions.c_str(), maps_line_vector[i].file_offset.c_str(), maps_line_vector[i].device_major_minor.c_str(), maps_line_vector[i].inode.c_str(), maps_line_vector[i].file_path.c_str());
        sleep(0); // 让出CPU时间片
    }

    // 修复后的代码（第69-90行）
    for (size_t i = 0; i < maps_line_vector.size(); i++) {
        // 检查是否是新的 .so 文件（通过 ELF 头或文件路径变化）
        bool is_new_so = false;

        // 方法1：检查 ELF 头（需要确保内存可读）
        if (string_start_with(maps_line_vector[i].permissions.c_str(), "r")
            && memcmp(maps_line_vector[i].address_range_start, "\x7f""ELF", 4) == 0) {
            is_new_so = true;
        }

        if (is_new_so) {
            // 创建新的 maps_so
            maps_so_t maps_so;
            maps_so.address_range_start = maps_line_vector[i].address_range_start;
            maps_so.address_range_end = maps_line_vector[i].address_range_end;
            maps_so.file_path = maps_line_vector[i].file_path;
            maps_so.device_major_minor = maps_line_vector[i].device_major_minor;
            maps_so.inode = maps_line_vector[i].inode;
            maps_so.lines.push_back(maps_line_vector[i]);
            maps_so_vector.push_back(maps_so);
        } else if (!maps_so_vector.empty()) {
            // 更新现有的 maps_so
            maps_so_t &maps_so = maps_so_vector.back();
            maps_so.address_range_end = maps_line_vector[i].address_range_end;
            maps_so.lines.push_back(maps_line_vector[i]);
        }
    }

    for (size_t i = 0; i < maps_so_vector.size(); i++) {
        if(maps_so_vector[i].lines.size() < 4) continue;
        maps_so_maps[maps_so_vector[i].file_path] = maps_so_vector[i];
    }

}

maps_so_t* zProcMaps::find_so_by_name(string so_name) {
    for (auto it = maps_so_maps.begin(); it != maps_so_maps.end(); it++) {
        if(string_end_with(it->first.c_str(), so_name.c_str())){
            return &it->second;
        }
    }
    return nullptr;
}




