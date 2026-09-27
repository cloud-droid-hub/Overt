package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 侧信道时序检测分析器。
 * <p>
 * 迁移自 zinfo zSideChannelInfo.cpp 的内联判断：
 * 采集端上报 faccessat 慢于 fchownat 的次数(error_count)。
 * - > 7000 → error
 * - > 5000 → warn
 * - 否则 → safe
 * 总是发射单一元素(与现状一致,安全时也显示绿勾行)。
 */
public final class SideChannelAnalyzer implements MainApplication.Analyzer {

    private static final int ERROR_THRESHOLD = 7000;
    private static final int WARN_THRESHOLD = 5000;

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
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

            JSONObject out = new JSONObject();
            out.put("side_channel", new JSONObject()
                    .put("risk", risk)
                    .put("explain", String.format("faccessat is slower than fchownat: %d", errorCount)));
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