package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 日志信息分析器。
 * <p>
 * 迁移自 zinfo zLogcatInfo.cpp get_logcat_info 的内联判断：
 * 存在含 "u:r:su:s0"(Zygisk 痕迹)的 logcat_record → error。
 */
public final class LogcatInfoAnalyzer implements MainApplication.Analyzer {

    private static final String ZYGISK_MARK = "u:r:su:s0";

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

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
}