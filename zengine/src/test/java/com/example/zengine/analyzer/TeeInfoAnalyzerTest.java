package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * TeeInfoAnalyzer 硬编码数据单测(等价于 zTeeInfo.cpp 行为)。
 */
public class TeeInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new TeeInfoAnalyzer();

    @Test
    public void secureTee_noError() throws Exception {
        String raw = "{" +
                "\"tee_state\":{\"value\":\"cert_ok\"}," +
                "\"device_locked\":{\"value\":\"1\"}," +
                "\"verified_boot_state\":{\"value\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("安全TEE无输出: " + out, out.length() == 0);
    }

    @Test
    public void unlockedDevice_isError() throws Exception {
        String raw = "{" +
                "\"tee_state\":{\"value\":\"cert_ok\"}," +
                "\"device_locked\":{\"value\":\"0\"}," +
                "\"verified_boot_state\":{\"value\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("device_locked").getString("risk"));
        assertEquals("device_locked is unsafe", out.getJSONObject("device_locked").getString("explain"));
    }

    @Test
    public void unverifiedBoot_isError() throws Exception {
        String raw = "{" +
                "\"tee_state\":{\"value\":\"cert_ok\"}," +
                "\"device_locked\":{\"value\":\"1\"}," +
                "\"verified_boot_state\":{\"value\":\"2\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("verified_boot_state").getString("risk"));
    }

    @Test
    public void certFailure_isError() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"tee_state\":{\"value\":\"cert_empty\"}}"));
        assertEquals("error", out.getJSONObject("tee_statue").getString("risk"));
        assertEquals("tee_statue is damage", out.getJSONObject("tee_statue").getString("explain"));
        assertFalse("证书失败不应再有其他检查", out.has("device_locked"));
    }
}