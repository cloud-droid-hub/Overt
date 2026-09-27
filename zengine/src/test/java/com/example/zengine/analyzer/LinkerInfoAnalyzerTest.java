package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * LinkerInfoAnalyzer 硬编码数据单测(等价于 zLinkerInfo.cpp 行为)。
 */
public class LinkerInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new LinkerInfoAnalyzer();

    @Test
    public void lsposedLib_emitsError() throws Exception {
        String raw = "{" +
                "\"/data/adb/modules/lsposed/lib/lspd.so\":{\"value\":\"so\"}," +
                "\"/system/lib64/libc.so\":{\"value\":\"so\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("/data/adb/modules/lsposed/lib/lspd.so"));
        assertEquals("error", out.getJSONObject("/data/adb/modules/lsposed/lib/lspd.so").getString("risk"));
        assertEquals("black soname", out.getJSONObject("/data/adb/modules/lsposed/lib/lspd.so").getString("explain"));
        assertFalse(out.has("/system/lib64/libc.so"));
    }

    @Test
    public void fridaLib_emitsError() throws Exception {
        String raw = "{\"/data/local/frida/frida-agent.so\":{\"value\":\"so\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 1);
        assertEquals("black soname", out.getJSONObject(out.keys().next()).getString("explain"));
    }

    @Test
    public void crcMismatch_emitsError() throws Exception {
        String raw = "{" +
                "\"libc.so\":{\"value\":\"crc:1\"}," +
                "\"libart.so\":{\"value\":\"crc:0\"}," +
                "\"libinput.so\":{\"value\":\"-\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("libc.so"));
        assertEquals("check_lib_crc error", out.getJSONObject("libc.so").getString("explain"));
        assertFalse(out.has("libart.so"));
        assertFalse(out.has("libinput.so"));
    }

    @Test
    public void cleanLibs_emitsEmpty() throws Exception {
        String raw = "{\"/system/lib64/libc.so\":{\"value\":\"so\"},\"libc.so\":{\"value\":\"crc:0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 0);
    }
}