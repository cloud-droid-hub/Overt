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

    /** Community ROM 指纹属性存在 → warn。 */
    @Test
    public void communityRomProp_isWarn() throws Exception {
        String raw = "{" +
                "\"ro.lineage.version\":{\"value\":\"21.0\",\"serial\":\"0\"}," +
                "\"ro.secure\":{\"value\":\"1\",\"serial\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("ro.lineage.version").getString("risk"));
        assertEquals("custom rom", out.getJSONObject("ro.lineage.version").getString("explain"));
    }

    /** 模拟器/云手机特征属性命中 → warn。 */
    @Test
    public void emulatorProp_isWarn() throws Exception {
        String raw = "{\"ro.kernel.qemu\":{\"value\":\"1\",\"serial\":\"0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("ro.kernel.qemu").getString("risk"));
        assertEquals("emulator/cloud property", out.getJSONObject("ro.kernel.qemu").getString("explain"));
    }

    /** 分区 fingerprint 与主指纹不一致 → error。 */
    @Test
    public void fingerprintMismatch_isError() throws Exception {
        String raw = "{" +
                "\"ro.build.fingerprint\":{\"value\":\"google/sdk_gphone/emu64a:17/CP31/123:user/release-keys\",\"serial\":\"0\"}," +
                "\"ro.vendor.build.fingerprint\":{\"value\":\"xiaomi/venus/venus:15/UKQ1/456:user/release-keys\",\"serial\":\"0\"}," +
                "\"ro.system.build.fingerprint\":{\"value\":\"google/sdk_gphone/emu64a:17/CP31/123:user/release-keys\",\"serial\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        // key 形如 "ro.vendor.build.fingerprint:mismatch[xiaomi/...]" (带实际 fp 值),用前缀判断
        java.util.Iterator<String> keys = out.keys();
        boolean vendorMismatch = false;
        boolean systemMismatch = false;
        while (keys.hasNext()) {
            String k = keys.next();
            if (k.startsWith("ro.vendor.build.fingerprint:mismatch[")) {
                vendorMismatch = true;
                assertEquals("warn", out.getJSONObject(k).getString("risk"));
                assertEquals("fingerprint mismatch", out.getJSONObject(k).getString("explain"));
            }
            if (k.startsWith("ro.system.build.fingerprint:mismatch[")) {
                systemMismatch = true;
            }
        }
        assertTrue("vendor 指纹不一致应报 warn", vendorMismatch);
        assertTrue("system 指纹一致不应报", !systemMismatch);
    }
}
