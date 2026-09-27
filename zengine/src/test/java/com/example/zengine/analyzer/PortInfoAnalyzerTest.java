package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * PortInfoAnalyzer 硬编码数据单测(等价于 zPortInfo.cpp 行为)。
 */
public class PortInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new PortInfoAnalyzer();

    @Test
    public void fridaPortInUse_emitsError() throws Exception {
        String raw = "{" +
                "\"27042\":{\"value\":\"1\"}," +
                "\"27043\":{\"value\":\"0\"}," +
                "\"27047\":{\"value\":\"0\"}," +
                "\"23946\":{\"value\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("frida"));
        assertEquals("error", out.getJSONObject("frida").getString("risk"));
        assertEquals("black port is in use frida", out.getJSONObject("frida").getString("explain"));
        assertFalse(out.has("ida"));
    }

    @Test
    public void idaPortInUse_emitsError() throws Exception {
        String raw = "{\"23946\":{\"value\":\"1\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("ida"));
        assertEquals("black port is in use ida", out.getJSONObject("ida").getString("explain"));
    }

    @Test
    public void noPorts_emitsEmpty() throws Exception {
        String raw = "{\"27042\":{\"value\":\"0\"},\"23946\":{\"value\":\"0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 0);
    }
}