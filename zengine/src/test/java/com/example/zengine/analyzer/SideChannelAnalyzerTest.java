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

    /** KernelSU prctl 命中(present)→ error,单独发射 ksu_prctl 项。 */
    @Test
    public void ksuPresent_isError() throws Exception {
        String raw = "{" +
                "\"side_channel\":{\"value\":\"1000\"}," +
                "\"ksu_prctl\":{\"value\":\"present\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("ksu_prctl").getString("risk"));
        assertEquals("kernel root prctl: present", out.getJSONObject("ksu_prctl").getString("explain"));
        // side_channel 时序仍正常判定 safe
        assertEquals("safe", out.getJSONObject("side_channel").getString("risk"));
    }

    /** KernelSU prctl 带版本号→ error。 */
    @Test
    public void ksuVersion_isError() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"ksu_prctl\":{\"value\":\"v:11972\"}}"));
        assertEquals("error", out.getJSONObject("ksu_prctl").getString("risk"));
        assertEquals("kernel root prctl: v:11972", out.getJSONObject("ksu_prctl").getString("explain"));
    }

    /** KernelSU prctl 未命中(值 0)→ 不报 ksu_prctl。 */
    @Test
    public void ksuNotPresent_noError() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"ksu_prctl\":{\"value\":\"0\"}}"));
        assertEquals("safe", out.getJSONObject("side_channel").getString("risk"));
        assertEquals(false, out.has("ksu_prctl"));
    }

    /** 无 ksu_prctl 字段(旧采集)→ 不报,不破坏原行为。 */
    @Test
    public void noKsuField_noError() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"side_channel\":{\"value\":\"5500\"}}"));
        assertEquals("warn", out.getJSONObject("side_channel").getString("risk"));
        assertEquals(false, out.has("ksu_prctl"));
    }
}