package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * SystemPropAnalyzer 硬编码数据单测(等价于 zSystemPropInfo.cpp 行为)。
 */
public class SystemPropAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SystemPropAnalyzer();

    @Test
    public void wrongExpectedValue_isError() throws Exception {
        String raw = "{" +
                "\"ro.debuggable\":{\"value\":\"1\",\"serial\":\"0\"}," +
                "\"ro.build.tags\":{\"value\":\"release-keys\",\"serial\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("ro.debuggable:value[1]"));
        assertEquals("value is not correct", out.getJSONObject("ro.debuggable:value[1]").getString("explain"));
        assertTrue(!out.has("ro.build.tags:value[release-keys]"));
    }

    @Test
    public void nonZeroSerial_isError() throws Exception {
        String raw = "{\"ro.secure\":{\"value\":\"1\",\"serial\":\"5\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("ro.secure:serial[5]"));
        assertEquals("serial_version is not 0", out.getJSONObject("ro.secure:serial[5]").getString("explain"));
    }

    @Test
    public void cleanProps_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"ro.secure\":{\"value\":\"1\",\"serial\":\"0\"}," +
                "\"persist.sys.usb.config\":{\"value\":\"none\",\"serial\":\"0\"}," +
                "\"init.svc.adbd\":{\"value\":\"stopped\",\"serial\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("干净属性无输出", out.length() == 0);
    }

    /** serial 检查范围不放大：非关键 ro.* 属性(不在 prop_map)即使 serial 非0,也不应被判 error。 */
    @Test
    public void serialScope_notBroadened_toNonKeyRoProps() throws Exception {
        // ro.build.version.sdk 不在原 C++ prop_map 内,serial 非0 不应触发
        String raw = "{" +
                "\"ro.build.version.sdk\":{\"value\":\"34\",\"serial\":\"65536\"}," +
                "\"ro.secure\":{\"value\":\"1\",\"serial\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("非关键 ro.* 的 serial 不应检查: " + out, out.length() == 0);
    }

    /** serial 检查覆盖关键 ro.* 属性的子集(与原 C++ prop_map 内 ro.* 一致)。 */
    @Test
    public void serialCheck_appliesToKeyRoProps() throws Exception {
        String raw = "{" +
                "\"ro.debuggable\":{\"value\":\"0\",\"serial\":\"2\"}," +
                "\"ro.vendor.build.tags\":{\"value\":\"release-keys\",\"serial\":\"3\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("ro.debuggable:serial[2]"));
        assertTrue(out.has("ro.vendor.build.tags:serial[3]"));
        assertEquals("serial_version is not 0", out.getJSONObject("ro.debuggable:serial[2]").getString("explain"));
    }
}