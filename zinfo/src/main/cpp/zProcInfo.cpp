//
// Created by lxz on 2025/8/24.
//

#include <dlfcn.h>
#include <regex>

#include "zLog.h"
#include "zLibc.h"
#include "zFile.h"

#include "zProcInfo.h"
#include "zStdUtil.h"
#include "zProcMaps.h"

inline size_t findBytes(const vector<uint8_t>& haystack,
                             const string& needle)
{
    if (needle.empty()) return 0;                       // 空串视为首位置
    const size_t n = needle.size();
    const size_t h = haystack.size();
    if (h < n) return string::npos;

    const uint8_t* const data = haystack.data();
    const uint8_t* const end  = data + h - n;

    for (const uint8_t* p = data; p <= end; ++p)
        if (memcmp(p, needle.data(), n) == 0)
            return static_cast<size_t>(p - data);

    return string::npos;
}

// 获取应用私有目录路径
string get_app_specific_dir_path2() {

    Dl_info dlInfo;
    dladdr((void *) get_app_specific_dir_path2, &dlInfo);
    LOGE("dlInfo.dli_fname %s", dlInfo.dli_fname);

    // 这里列举可能获取到的路径
    // extractNativeLibs=False
    // android8     /data/app/com.example.zappspecificdirpath-8vHILujCn2tIBffmQy2qEg==/base.apk!/lib/arm64-v8a/libzAppSpecificDirPath.so
    // android9     /data/app/com.example.zappspecificdirpath-fCRHWTJ-twcmhCP2MtacVQ==/base.apk!/lib/arm64-v8a/libzAppSpecificDirPath.so
    // android10    /data/app/com.example.zappspecificdirpath-HegrfXlnqnzOLki2G8wEDA==/base.apk!/lib/arm64-v8a/libzAppSpecificDirPath.so
    // android11    /data/app/~~Qpzn7ScoMn6IN0Ex83T_QQ==/com.example.zappspecificdirpath-ynF7aqmu0fLQVqvsCHoPYg==/base.apk!/lib/arm64-v8a/libzAppSpecificDirPath.so
    // android12    /data/app/~~f9g8U9zlePuy_WqgZ8okBQ==/com.example.zappspecificdirpath-lig31uP97bB0FBMrJtTc7A==/base.apk!/lib/arm64-v8a/libzAppSpecificDirPath.so
    // android13    /data/app/~~qkSn5MFpSCiivUU04zV6ww==/com.example.zappspecificdirpath-yAnEGWVCn4flICUYNYgmiQ==/base.apk!/lib/arm64-v8a/libzAppSpecificDirPath.so
    // android14    /data/app/~~a1c4uFCHhMq_tGlzDd3wNA==/com.example.zappspecificdirpath-tFhsF_RI7oMWWeYFXfX_XA==/base.apk!/lib/arm64-v8a/libzAppSpecificDirPath.so

    // extractNativeLibs=True
    // android8     /data/app/com.example.zappspecificdirpath-l4CqcuvX1VmXV44is1Rftw==/lib/arm64/libzAppSpecificDirPath.so
    // android9     /data/app/com.example.zappspecificdirpath-qFP8eTYb71-JAKlgnkTZFw==/lib/arm64/libzAppSpecificDirPath.so
    // android10    /data/app/com.example.zappspecificdirpath-lInIIiPhrBu1DeyZC5NFtw==/lib/arm64/libzAppSpecificDirPath.so
    // android11    /data/app/~~DERGEtEs5yQ62LLLf4m2lw==/com.example.zappspecificdirpath-y3Lykr5W_tGIZovYxEccAA==/lib/arm64/libzAppSpecificDirPath.so
    // android12    /data/app/~~i2VqkZlWtT6SVeMhRovP_w==/com.example.zappspecificdirpath-J1lZTjAtgm0Q4yWK9IQnoQ==/lib/arm64/libzAppSpecificDirPath.so
    // android13    /data/app/~~XsF13sQwJUJ5gfdYNAaSzQ==/com.example.zappspecificdirpath-f2dL9tXiEeEP_vUbLbuJng==/lib/arm64/libzAppSpecificDirPath.so
    // android14    /data/app/~~Kk6H0mvaWN4oFARFVd2O5A==/com.example.zappspecificdirpath-CJS-eW_VEWNq3aFObuFSMg==/lib/arm64/libzAppSpecificDirPath.so

    std::cmatch matchs;
    std::regex rx("(/data/app/.+?==)(?:/base.apk!)*/lib");
    bool found = std::regex_search((const char *) dlInfo.dli_fname,
                                   (const char *) dlInfo.dli_fname +
                                   strlen((char *) dlInfo.dli_fname), matchs, rx);
    if (found) {
        return matchs.str(1).c_str();
    }
    return "";
}

/**
 * 获取内存映射信息(查杀分离 — 采集端)
 * 分析/proc/self/maps文件，全量上报关键系统库的映射段数量/权限 与 base.odex 状态，
 * 不做风险判定；映射数量/权限异常判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{"libart.so" -> {value: "段数:权限序列"}} + {"odex.base"->..} {"odex.content"->..}
 */
map<string, map<string, string>> get_maps_info() {
    LOGD("get_maps_info called");
    map<string, map<string, string>> info;

    zProcMaps maps = zProcMaps();

    // 采集范围：关键系统库(仅WHERE to look，不是风险判定)
    vector<string> check_lib_list = {
            "libart.so",    // Android运行时库
            "libc.so",      // C标准库
            "libinput.so",  // 输入库
    };

    for (string lib_name: check_lib_list) {
        LibraryMapping library = maps.find_so_by_name(lib_name);
        if(library.address_range_start == nullptr) continue;   // 未找到哨兵

        // 拼接权限序列，如 "r--p,r-xp,r--p,rw-p"
        string perms;
        for (size_t i = 0; i < library.segments.size(); ++i) {
            if (i > 0) perms += ",";
            perms += library.segments[i].permissions;
        }
        info[lib_name]["value"] = string_format("%zu:%s", library.segments.size(), perms.c_str());
    }

    string base_odex_path = "";

    LibraryMapping library = maps.find_so_by_name("/oat/arm64/base.odex");
    if(library.address_range_start != nullptr){
        LOGE("base.odex: %s", library.file_path.c_str());
        base_odex_path = library.file_path;
        info["odex.base"]["value"] = base_odex_path;
    }else{
        LOGE("base.odex load failed");
        base_odex_path = get_app_specific_dir_path2() + "/oat/arm64/base.odex";
        info["odex.base"]["value"] = "not_loaded:" + base_odex_path;
    }

    zFile base_odex = zFile(base_odex_path);
    if(base_odex.exists()){
        vector<uint8_t> bytes = base_odex.readAllBytes();
        size_t pos = findBytes(bytes, "--inline-max-code-units=0");
        // 全量上报黑串存在状态(原始值)，判定交 zengine
        info["odex.content"]["value"] = (pos != string::npos) ? "1" : "0";
    }else{
        info["odex.content"]["value"] = "not_exists";
    }

    return info;
}

/**
 * 获取挂载点信息(查杀分离 — 采集端)
 * 读取/proc/self/mounts全量上报每一行(不做过滤)，不做风险判定；
 * 异常挂载名(dex2oat/APatch/shamiko等)与overlay判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{"mounts:N" -> {value: "原始挂载行"}}
 */
map<string, map<string, string>> get_mounts_info() {
    LOGI("get_mounts_info called");
    map<string, map<string, string>> info;

    // 读取/proc/self/mounts文件，获取当前进程的挂载点信息(全量)
    vector<string> mounts_lines = zFile("/proc/self/mounts").readAllLines();
    LOGI("Read %zu lines from /proc/self/mounts", mounts_lines.size());

    // 遍历每一行，全量上报(不内置过滤)
    for (int i = 0; i < mounts_lines.size(); i++) {
        LOGD("Processing line %d: %s", i, mounts_lines[i].c_str());
        info["mounts:" + to_string(i)]["value"] = mounts_lines[i];
    }

    LOGI("mounts_info raw count=%zu", info.size());
    return info;
}

/**
 * 获取任务信息(查杀分离 — 采集端)
 * 遍历/proc/self/task每个线程的stat行全量上报(不做过滤)，不做风险判定；
 * Frida特征线程名(gmain/pool-frida)判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{"task:<tid>" -> {value: "stat行"}}
 */
map<string, map<string, string>> get_task_info() {
    LOGD("get_task_info called");
    map<string, map<string, string>> info;

    // 获取当前进程的所有任务目录列表
    vector<string> task_dir_list = zFile("/proc/self/task").listDirectories();
    LOGI("Found %zu task directories", task_dir_list.size());

    // 遍历每个任务目录
    for (string task_dir: task_dir_list) {
        LOGD("Processing task_dir: %s", task_dir.c_str());

        // 构建线程状态文件路径
        string stat_path = "/proc/self/task/" + task_dir + "/stat";

        // 读取线程状态信息
        vector<string> stat_line_list = zFile(stat_path).readAllLines();

        // 全量上报每行(不内置过滤)
        for (string stat_line: stat_line_list) {
            LOGD("Processing stat_line: %s", stat_line.c_str());
            info["task:" + task_dir]["value"] = stat_line;
        }
    }
    LOGI("task_info raw count=%zu", info.size());
    return info;
}


/**
 * 获取进程属性信息(查杀分离 — 采集端)
 * 读取/proc/self/attr/prev全量上报每一行，不做风险判定；
 * zygote特征(可能为Magisk痕迹)判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{"prev:N" -> {value: "原始行"}}
 */
map<string, map<string, string>> get_attr_prev_info() {
    LOGI("get_attr_prev_info called");
    map<string, map<string, string>> info;

    vector<string> lines = zFile("/proc/self/attr/prev").readAllLines();

    // 遍历每一行，全量上报(不内置过滤)
    for (size_t i = 0; i < lines.size(); ++i) {
        LOGD("line %zu %s", i, lines[i].c_str());
        info["prev:" + to_string(i)]["value"] = lines[i];
    }
    LOGI("attr_prev_info raw count=%zu", info.size());
    return info;
}

/**
 * 获取网络TCP信息(查杀分离 — 采集端)
 * 读取/proc/self/net/tcp全量上报每一行(android7后无权限则为空)，不做风险判定；
 * Frida/IDA端口特征(:69A2/:69A3/:5D8A)判定由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{"net_tcp:N" -> {value: "原始行"}}
 */
map<string, map<string, string>> get_net_tcp_info() {
    LOGI("get_net_tcp_info called");
    map<string, map<string, string>> info;

    // android7 之后没权限
    vector<string> lines = zFile("/proc/self/net/tcp").readAllLines();

    // 遍历每一行，全量上报(不内置过滤)
    for (size_t i = 0; i < lines.size(); ++i) {
        LOGD("line %zu %s", i, lines[i].c_str());
        info["net_tcp:" + to_string(i)]["value"] = lines[i];
    }
    LOGI("net_tcp_info raw count=%zu", info.size());
    return info;
}

/**
 * 获取进程信息的主函数
 * 整合所有进程相关的检测功能，包括内存映射、挂载点、任务状态等
 * 通过多种检测手段综合分析进程的安全状态
 * @return 包含所有检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_proc_info() {
    map<string, map<string, string>> info;

    LOGI("get_maps_info is called");
    map<string, map<string, string>> maps_info = get_maps_info();
    LOGI("get_maps_info insert is called");
    info.insert(maps_info.begin(), maps_info.end());

    LOGI("get_mounts_info is called");
    map<string, map<string, string>> mounts_info = get_mounts_info();
    LOGI("get_mounts_info insert is called");
    info.insert(mounts_info.begin(), mounts_info.end());

    LOGI("get_task_info is called");
    map<string, map<string, string>> task_info = get_task_info();
    LOGI("get_task_info insert is called");
    info.insert(task_info.begin(), task_info.end());

    LOGI("get_attr_prev_info is called");
    map<string, map<string, string>> attr_prev_info = get_attr_prev_info();
    LOGI("get_attr_prev_info insert is called");
    info.insert(attr_prev_info.begin(), attr_prev_info.end());

    LOGI("get_net_tcp_info is called");
    map<string, map<string, string>> net_tcp_info = get_net_tcp_info();
    LOGI("get_net_tcp_info insert is called");
    info.insert(net_tcp_info.begin(), net_tcp_info.end());

    return info;
}