package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 时间信息分析器。
 * <p>
 * 迁移自 zinfo zTimeInfo.cpp get_time_info 的内联判断：
 * - 本地时间 - 启动时间 < 24h → boot_time warn(否则 safe)
 * - |远程时间 - 本地时间| > 60s → local/remote warn(否则 safe)
 * 显式输出 safe(与现状一致,时间卡片总有行)。
 */
public final class TimeInfoAnalyzer implements MainApplication.Analyzer {

    private static final long ONE_DAY_SECONDS = 24L * 60 * 60;
    private static final long SKEW_THRESHOLD_SECONDS = 60;

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            long localTime = parseLong(valueOf(raw, "local_current_time"));
            long bootTime = parseLong(valueOf(raw, "boot_time"));
            long remoteTime = parseLong(valueOf(raw, "remote_current_time"));

            String bootFormatted = formattedOf(raw, "boot_time");
            if (localTime - bootTime < ONE_DAY_SECONDS) {
                out.put("boot_time", new JSONObject()
                        .put("risk", "warn")
                        .put("explain", "boot_time is too short " + bootFormatted));
            } else {
                out.put("boot_time", new JSONObject()
                        .put("risk", "safe")
                        .put("explain", "boot_time is " + bootFormatted));
            }

            String localFormatted = formattedOf(raw, "local_current_time");
            String remoteFormatted = formattedOf(raw, "remote_current_time");
            String localRisk = (Math.abs(remoteTime - localTime) > SKEW_THRESHOLD_SECONDS) ? "warn" : "safe";
            String remoteRisk = localRisk;

            out.put("local_current_time", new JSONObject()
                    .put("risk", localRisk)
                    .put("explain", "local_current_time is " + localFormatted));
            out.put("remote_current_time", new JSONObject()
                    .put("risk", remoteRisk)
                    .put("explain", "remote_current_time is " + remoteFormatted));

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    private static String valueOf(JSONObject raw, String key) {
        JSONObject item = raw.optJSONObject(key);
        return (item == null) ? "" : item.optString("value", "");
    }

    private static String formattedOf(JSONObject raw, String key) {
        JSONObject item = raw.optJSONObject(key);
        return (item == null) ? "" : item.optString("formatted", "");
    }

    private static long parseLong(String s) {
        try {
            return Long.parseLong(s.trim());
        } catch (NumberFormatException e) {
            return 0L;
        }
    }
}