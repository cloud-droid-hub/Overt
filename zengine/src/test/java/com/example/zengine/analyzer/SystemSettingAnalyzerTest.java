package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * SystemSettingAnalyzer 硬编码数据单测(等价于 zSystemSettingInfo.cpp 行为)。
 */
public class SystemSettingAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SystemSettingAnalyzer();

    @Test
    public void riskySettings_emitErrors() throws Exception {
        String raw = "{" +
                "\"battery\":{\"value\":\"1\"}," +
                "\"installer\":{\"value\":\"com.unknown.store\"}," +
                "\"sim\":{\"value\":\"0\"}," +
                "\"developer_mode\":{\"value\":\"1\"}," +
                "\"usb_debug\":{\"value\":\"1\"}," +
                "\"proxy\":{\"value\":\"1\"}," +
                "\"password\":{\"value\":\"0\"}," +
                "\"vpn\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("battery").getString("risk"));
        assertEquals("warn", out.getJSONObject("installer").getString("risk"));
        assertEquals("error", out.getJSONObject("sim").getString("risk"));
        assertEquals("error", out.getJSONObject("developer_mode").getString("risk"));
        assertEquals("error", out.getJSONObject("usb_debug").getString("risk"));
        assertEquals("error", out.getJSONObject("proxy").getString("risk"));
        assertEquals("warn", out.getJSONObject("password").getString("risk"));
        assertEquals("error", out.getJSONObject("vpn").getString("risk"));
    }

    @Test
    public void officialMarket_noInstallerWarn() throws Exception {
        String raw = "{\"installer\":{\"value\":\"com.xiaomi.market\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("官方市场不应有 installer 警告", !out.has("installer"));
    }

    @Test
    public void cleanSettings_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"battery\":{\"value\":\"0\"}," +
                "\"installer\":{\"value\":\"com.oppo.market\"}," +
                "\"sim\":{\"value\":\"1\"}," +
                "\"developer_mode\":{\"value\":\"0\"}," +
                "\"usb_debug\":{\"value\":\"0\"}," +
                "\"proxy\":{\"value\":\"0\"}," +
                "\"password\":{\"value\":\"1\"}," +
                "\"vpn\":{\"value\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("干净设置无输出", out.length() == 0);
    }
}