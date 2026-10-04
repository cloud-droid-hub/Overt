package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 风险文件检测分析器。
 * <p>
 * 消费 zinfo zRiskFileInfo.cpp 上报的 {路径 -> {value: "1"/"0"}} 原始数据。
 * 采集端不掺类别语义，本分析器内置多类风险文件名单并各自判定：
 * - Root 特征文件(11 个)存在 → error "black file but exist"
 * - 模拟器特征文件(34 个)存在 → error "emulator file"
 */
public final class RiskFileAnalyzer implements MainApplication.Analyzer {

    /** Root 特征文件路径(迁移自原 zRootStateInfo.cpp)。命中 → error。 */
    private static final String[] ROOT_PATHS = {
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
    };

    /** 模拟器特征文件路径(来自 RiskEngine list_emulator_files)。命中 → error。 */
    private static final String[] EMULATOR_PATHS = {
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

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // Root 特征文件:存在 → error
            for (String path : ROOT_PATHS) {
                JSONObject item = raw.optJSONObject(path);
                if (item != null && "1".equals(item.optString("value", ""))) {
                    out.put(AnalyzerUtil.trimKey(path), new JSONObject()
                            .put("risk", "error")
                            .put("explain", "black file but exist"));
                }
            }

            // 模拟器特征文件:存在 → error
            for (String path : EMULATOR_PATHS) {
                JSONObject item = raw.optJSONObject(path);
                if (item != null && "1".equals(item.optString("value", ""))) {
                    out.put(AnalyzerUtil.trimKey(path), new JSONObject()
                            .put("risk", "error")
                            .put("explain", "emulator file"));
                }
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}