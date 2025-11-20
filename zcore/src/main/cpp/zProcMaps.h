//
// Created by liuxi on 2025/11/19.
//

#ifndef OVERT_ZPROCMAPS_H
#define OVERT_ZPROCMAPS_H

#include "zLog.h"
#include "zStd.h"
#include "zFile.h"

struct MapSegment {
    void* address_range_start;              // 地址范围开始
    void* address_range_end;                // 地址范围结束
    string permissions;                     // 权限标志（如r--p, r-xp等）
    string file_offset;                     // 文件偏移量
    string device_major_minor;              // 设备号（主设备号:次设备号）
    string inode;                           // 节点号
    string file_path;                       // 文件路径
};

struct LibraryMapping {
    void* address_range_start;              // 地址范围开始
    void* address_range_end;                // 地址范围结束
    string device_major_minor;              // 设备号（主设备号:次设备号）
    string inode;                           // 节点号
    string file_path;                       // 文件路径
    vector<MapSegment> segments;            // 映射段信息
};

class zProcMaps{
public :

    map<string, LibraryMapping> loaded_libraries = {};

    zProcMaps();

    ~zProcMaps(){};

    LibraryMapping* find_so_by_name(string so_name);

};


#endif //OVERT_ZPROCMAPS_H
