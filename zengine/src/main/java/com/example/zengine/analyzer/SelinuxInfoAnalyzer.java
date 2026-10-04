package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Locale;

/**
 * SELinux 检测分析器。
 * <p>
 * 迁移自 zinfo zSelinuxInfo.cpp get_selinux_info 的原始数据采集：
 * 1. selinux_context:进程 SELinux 上下文含 "magisk" / "su:" / ":su" 标记 → error
 *    (正常 app 域为 untrusted_app;su/magisk 域说明被 root 框架提权)
 * 2. logcat_record:含 "u:r:su:s0"(Zygisk 痕迹)→ error
 */
public final class SelinuxInfoAnalyzer implements MainApplication.Analyzer {

    /** Zygisk 痕迹(日志中 su 域的 SELinux 审计)。 */
    private static final String ZYGISK_MARK = "u:r:su:s0";
    /** SELinux 上下文中被提权域标记。 */
    private static final String[] SU_DOMAIN_MARKS = {"magisk", "su:", ":su"};

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // 1) SELinux context 判定
            JSONObject ctxItem = raw.optJSONObject("selinux_context");
            if (ctxItem != null) {
                String context = ctxItem.optString("value", "");
                String lower = context.toLowerCase(Locale.ROOT);
                if (!context.isEmpty() && !"unreadable".equals(context)
                        && containsAny(lower)) {
                    out.put("selinux_context", new JSONObject()
                            .put("risk", "error")
                            .put("explain", context));
                }
            }

            // 2) logcat zygisk 痕迹判定(原 LogcatInfoAnalyzer 逻辑保留)
            JSONObject item = raw.optJSONObject("logcat_record");
            if (item != null && item.optString("value", "").contains(ZYGISK_MARK)) {
                out.put("logcat_record", new JSONObject()
                        .put("risk", "error")
                        .put("explain", item.optString("value", "")));
            }
            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    private static boolean containsAny(String lower) {
        for (String mark : SU_DOMAIN_MARKS) {
            if (lower.contains(mark)) return true;
        }
        return false;
    }
}