package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * TimeInfoAnalyzer 硬编码数据单测(等价于 zTimeInfo.cpp 阈值行为)。
 */
public class TimeInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new TimeInfoAnalyzer();

    @Test
    public void shortBootTime_isWarn() throws Exception {
        String raw = "{" +
                "\"local_current_time\":{\"value\":\"1700000000\",\"formatted\":\"2026-01-01 00:00:00\"}," +
                "\"boot_time\":{\"value\":\"1700000000\",\"formatted\":\"2026-01-01 00:00:00\"}," +
                "\"remote_current_time\":{\"value\":\"1700000000\",\"formatted\":\"2026-01-01 00:00:00\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("boot_time").getString("risk"));
        assertEquals("safe", out.getJSONObject("local_current_time").getString("risk"));
        assertEquals("safe", out.getJSONObject("remote_current_time").getString("risk"));
    }

    @Test
    public void longBootTime_isSafe() throws Exception {
        // boot 2 天前
        String raw = "{" +
                "\"local_current_time\":{\"value\":\"1700000000\",\"formatted\":\"2026-01-01 00:00:00\"}," +
                "\"boot_time\":{\"value\":\"1699827200\",\"formatted\":\"2025-12-30 00:00:00\"}," +
                "\"remote_current_time\":{\"value\":\"1700000000\",\"formatted\":\"2026-01-01 00:00:00\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("safe", out.getJSONObject("boot_time").getString("risk"));
    }

    @Test
    public void clockSkew_isWarn() throws Exception {
        // 本地与远程相差 500s
        String raw = "{" +
                "\"local_current_time\":{\"value\":\"1700000000\",\"formatted\":\"2026-01-01 00:00:00\"}," +
                "\"boot_time\":{\"value\":\"1700000000\",\"formatted\":\"2026-01-01 00:00:00\"}," +
                "\"remote_current_time\":{\"value\":\"1700000500\",\"formatted\":\"2026-01-01 00:08:20\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("local_current_time").getString("risk"));
        assertEquals("warn", out.getJSONObject("remote_current_time").getString("risk"));
    }
}