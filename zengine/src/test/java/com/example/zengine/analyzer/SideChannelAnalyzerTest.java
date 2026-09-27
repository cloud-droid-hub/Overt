package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * SideChannelAnalyzer 硬编码数据单测。
 */
public class SideChannelAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SideChannelAnalyzer();

    @Test
    public void count7500_isError() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"side_channel\":{\"value\":\"7500\"}}"));
        assertEquals("error", out.getJSONObject("side_channel").getString("risk"));
        assertEquals("faccessat is slower than fchownat: 7500", out.getJSONObject("side_channel").getString("explain"));
    }

    @Test
    public void count5500_isWarn() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"side_channel\":{\"value\":\"5500\"}}"));
        assertEquals("warn", out.getJSONObject("side_channel").getString("risk"));
        assertEquals("faccessat is slower than fchownat: 5500", out.getJSONObject("side_channel").getString("explain"));
    }

    @Test
    public void count1000_isSafe() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"side_channel\":{\"value\":\"1000\"}}"));
        assertEquals("safe", out.getJSONObject("side_channel").getString("risk"));
    }

    @Test
    public void boundary5000_isSafe() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"side_channel\":{\"value\":\"5000\"}}"));
        assertEquals("safe", out.getJSONObject("side_channel").getString("risk"));
    }

    @Test
    public void boundary7000_isWarn() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"side_channel\":{\"value\":\"7000\"}}"));
        assertEquals("warn", out.getJSONObject("side_channel").getString("risk"));
    }

    @Test
    public void missingItem_defaultsSafe() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{}"));
        assertEquals("safe", out.getJSONObject("side_channel").getString("risk"));
    }

    @Test
    public void malformedValue_returnsFailClosedEmpty() {
        assertEquals("{}", analyzer.analyze("not json"));
    }
}