package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * LocalNetworkAnalyzer 硬编码数据单测(等价于 zLocalNetworkInfo.cpp 行为)。
 */
public class LocalNetworkAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new LocalNetworkAnalyzer();

    @Test
    public void overtPeer_isWarn() throws Exception {
        String raw = "{\"192.168.1.5\":{\"value\":\"overt\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("192.168.1.5").getString("risk"));
        assertEquals("overt device", out.getJSONObject("192.168.1.5").getString("explain"));
    }

    @Test
    public void noPeer_emitsEmpty() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{}"));
        assertTrue(out.length() == 0);
    }
}