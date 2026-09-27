//
// Created by lxz on 2025/6/6.
//


#include "zLog.h"
#include "zFile.h"
#include "zRiskFileInfo.h"

/**
 * 风险特征文件探测清单(纯路径集合，无类别语义)。
 * 涵盖了 root 特征文件与模拟器特征文件等；
 * "哪些路径属于哪类风险、命中判什么等级"由 zengine 分析引擎决定。
 */
static const char* kRiskFilePaths[] = {
        // ---- Root 特征文件 ----
        "/sbin/su",
        "/system/bin/su",
        "/system/xbin/su",
        "/data/local/xbin/su",
        "/data/local/bin/su",
        "/system/sd/xbin/su",
        "/system/bin/failsafe/su",
        "/data/local/su",
        "/system/xbin/mu",
        "/system_ext/bin/su",
        "/apex/com.android.runtime/bin/suu",

        // ---- 模拟器特征文件 ----
        "/system/bin/androVM-prop",
        "/system/bin/microvirt-prop",
        "/system/lib/libdroid4x.so",
        "/system/bin/windroyed",
        "/system/bin/nox-prop",
        "/system/lib/libnoxspeedup.so",
        "/system/bin/ttVM-prop",
        "/data/.bluestacks.prop",
        "/system/bin/duosconfig",
        "/system/etc/xxzs_prop.sh",
        "/system/etc/mumu-configs/device-prop-configs/mumu.config",
        "/system/etc/mumu-configs",
        "/system/priv-app/ldAppStore",
        "/system/lib/libc_malloc_debug_qemu.so",
        "/dev/qemu_pipe",
        "/dev/goldfish_pipe",
        "/sys/qemu_trace",
        "/dev/socket/qemud",
        "/system/bin/ldinit",
        "/system/lib64/libldutils.so",
        "/system/bin/bstconf",
        "/dev/vboxuser",
        "/dev/vboxguest",
        "/dev/socket/genyd",
        "/dev/socket/baseband_genyd",
        "/system/bin/genybaseband",
        "/system/bin/qemu-props",
        "/system/bin/microvirtd",
        "/system/bin/droid4x-prop",
        "/system/bin/ldmountsf",
        "/system/app/AntStore",
        "/system/app/AntLauncher",
        "/dev/.redroid",
        "/dev/redroid",
};

/**
 * 获取风险文件信息(查杀分离 — 采集端)
 * 统一探测所有风险特征文件路径的存在状态，全量上报原始数据，不做任何判定；
 * 风险判定(哪些路径存在应判为风险)由 zengine 分析引擎负责。
 * @return 包含原始数据的Map，格式：{文件路径 -> {value: "1"存在 / "0"不存在}}
 */
map<string, map<string, string>> get_risk_file_info(){
    LOGD("get_risk_file_info called");
    map<string, map<string, string>> info;

    // 遍历检测每个路径，全量上报存在状态，不掺类别/判定
    size_t count = sizeof(kRiskFilePaths) / sizeof(kRiskFilePaths[0]);
    for (size_t i = 0; i < count; i++) {
        const char* path = kRiskFilePaths[i];
        LOGD("Checking path: %s", path);
        zFile file(path);
        info[path]["value"] = file.exists() ? "1" : "0";
    }

    LOGI("risk_file_info raw count=%zu", info.size());
    return info;
}