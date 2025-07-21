//
// Created by lxz on 2025/7/11.
//

#include <unistd.h>
#include "maps_info.h"
#include "zLog.h"
#include "zFile.h"
#include "util.h"
#include "zDevice.h"

struct maps_line_t {
    void* address_range_start;           // 地址范围开始
    void* address_range_end;             // 地址范围结束
    string permissions;            // 权限标志
    string file_offset;            // 文件偏移量
    string device_major_minor;     // 设备号
    string inode;                  // 节点号
    string file_path;              // 文件路径
};


map<string, map<string, string>> get_maps_info(){
    map<string, map<string, string>> info;

    map<string, vector<maps_line_t>> maps_table;
    zFile file("/proc/self/maps");

    // 检查文件是否成功打开
    if (file.isOpen()) {
        LOGE("文件成功打开: %s", file.getPath().c_str());
    } else {
        LOGE("文件打开失败: %s", file.getPath().c_str());
        return info;
    }

    // 按行读取
    vector<string> lines = file.readAllLines();
    LOGE("文件行数: %zu", lines.size());

    // 显示前几行
    for (size_t i = 0; i < lines.size(); i++) {
        if(string_end_with(lines[i].c_str(), ".so")){
            LOGE("第%zu行: %s", i + 1, lines[i].c_str());

            maps_line_t maps_line;

            vector<string> parts = split_str(lines[i], ' ');

            if (parts.size() != 6){
                LOGE("第%zu行: 部分信息不足 %s", i + 1, lines[i].c_str());
                continue;
            }

            // 解析地址范围

            vector<string> address_range_parts = split_str(parts[0], '-');
            if (address_range_parts.size() == 2) {
                maps_line.address_range_start = (void*)strtoul(address_range_parts[0].c_str(), nullptr, 16);   // 地址范围开始
                maps_line.address_range_end  = (void*)strtoul(address_range_parts[1].c_str(), nullptr, 16);     // 地址范围结束
            }

            maps_line.permissions = parts[1];
            maps_line.file_offset = parts[2];
            maps_line.device_major_minor = parts[3];
            maps_line.inode = parts[4];
            maps_line.file_path = parts[5];

            LOGE("地址范围: %p - %p 权限: %s 文件路径: %s", maps_line.address_range_start, maps_line.address_range_end, maps_line.permissions.c_str(), maps_line.file_path.c_str());

            maps_table[maps_line.file_path].push_back(maps_line);
            sleep(0);
        }
    }

    vector<string> check_lib_list = {
            "libart.so",
            "libc.so",
    };

    for (auto it = maps_table.begin(); it != maps_table.end(); it++){
        // LOGE("文件路径: %s, 映射数量: %zu", it->first.c_str(), it->second.size());
        for(string lib_name : check_lib_list){
            if(string_end_with(it->first.c_str(), lib_name.c_str())){
                if(it->second.size() != 4){
                    LOGE("文件路径: %s, 映射数量不符合预期: %zu", it->first.c_str(), it->second.size());
                    info[lib_name]["risk"] = "error";
                    info[lib_name]["explain"] = "reference count error";
                }else if(it->second.size() == 4 && (it->second[0].permissions!= "r--p" || it->second[1].permissions!= "r-xp" || it->second[2].permissions!= "r--p" || it->second[3].permissions!="rw-p")){
                    LOGE("文件路径: %s, 映射权限不符合预期", it->first.c_str());
                    info[lib_name]["risk"] = "error";
                    info[lib_name]["explain"] = "permissions error";
                }
                break;
            }
        }
    }

    return info;
}

