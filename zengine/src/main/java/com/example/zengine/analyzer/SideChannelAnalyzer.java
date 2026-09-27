package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 侧信道时序检测分析器(含 KernelSU/APatch prctl 探测判定)。
 * <p>
 * 迁移自 zinfo zSideChannelInfo.cpp 的内联判断：
 * 1. faccessat 慢于 fchownat 的次数(error_count):
 *    - > 7000 → error
 *    - > 5000 → warn
 *    - 否则 → safe
 *    总是发射 side_channel 元素(安全时也显示绿勾行)。
 * 2. KernelSU/APatch prctl 探测(采集端 ksu_prctl):
 *    - value != "0"(探测命中,内核响应了私有 prctl)→ error
 *    - 命中即内核已打 KSU/APatch 补丁,属强 root 信号
 */
public final class SideChannelAnalyzer implements MainApplication.Analyzer {

    private static final int ERROR_THRESHOLD = 7000;
    private static final int WARN_THRESHOLD = 5000;

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // 1) 侧信道时序判定
            JSONObject item = raw.optJSONObject("side_channel");
            int errorCount = (item == null) ? 0 : parseIntSafe(item.optString("value", "0"));

            String risk;
            if (errorCount > ERROR_THRESHOLD) {
                risk = "error";
            } else if (errorCount > WARN_THRESHOLD) {
                risk = "warn";
            } else {
                risk = "safe";
            }

            out.put("side_channel", new JSONObject()
                    .put("risk", risk)
                    .put("explain", String.format("faccessat is slower than fchownat: %d", errorCount)));

            // 2) KernelSU/APatch prctl 探测判定(命中 → error)
            JSONObject ksuItem = raw.optJSONObject("ksu_prctl");
            if (ksuItem != null) {
                String ksuValue = ksuItem.optString("value", "0");
                if (!"0".equals(ksuValue) && !ksuValue.isEmpty()) {
                    out.put("ksu_prctl", new JSONObject()
                            .put("risk", "error")
                            .put("explain", "kernel root prctl: " + ksuValue));
                }
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    private static int parseIntSafe(String s) {
        try {
            return Integer.parseInt(s.trim());
        } catch (NumberFormatException e) {
            return 0;
        }
    }
}