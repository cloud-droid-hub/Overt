package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * RiskFileAnalyzer 硬编码数据单测。
 * 喂 {路径 -> {value: "1"/"0"}} 原始数据，断言 root/emulator 特征文件命中即 error。
 */
public class RiskFileAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new RiskFileAnalyzer();

    /** Root 特征文件存在 → error。 */
    @Test
    public void rootPathExists_emitsError() throws Exception {
        String raw = "{" +
                "\"/sbin/su\":{\"value\":\"1\"}," +
                "\"/system/bin/su\":{\"value\":\"0\"}," +
                "\"/data/local/bin/other\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));

        assertTrue("root 路径应命中", out.has("/sbin/su"));
        assertEquals("error", out.getJSONObject("/sbin/su").getString("risk"));
        assertEquals("black file but exist", out.getJSONObject("/sbin/su").getString("explain"));
        assertFalse("不存在的 root 路径不应出现", out.has("/system/bin/su"));
        assertFalse("非名单路径不应出现", out.has("/data/local/bin/other"));
    }

    /** 模拟器特征文件存在 → error(命中即 error)。 */
    @Test
    public void emulatorPathExists_emitsError() throws Exception {
        String raw = "{" +
                "\"/dev/qemu_pipe\":{\"value\":\"1\"}," +
                "\"/dev/goldfish_pipe\":{\"value\":\"1\"}," +
                "\"/dev/vboxuser\":{\"value\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));

        assertTrue("qemu_pipe 应命中", out.has("/dev/qemu_pipe"));
        assertEquals("error", out.getJSONObject("/dev/qemu_pipe").getString("risk"));
        assertEquals("emulator file", out.getJSONObject("/dev/qemu_pipe").getString("explain"));
        assertTrue("goldfish_pipe 应命中", out.has("/dev/goldfish_pipe"));
        assertFalse("不存在的 emulator 路径不应出现", out.has("/dev/vboxuser"));
    }

    /** 全部安全(无任何特征文件) → 无输出。 */
    @Test
    public void allSafe_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"/sbin/su\":{\"value\":\"0\"}," +
                "\"/dev/qemu_pipe\":{\"value\":\"0\"}," +
                "\"/dev/redroid\":{\"value\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("安全时应为空对象", out.length() == 0);
    }

    /** 非法 JSON → fail-closed 空对象。 */
    @Test
    public void malformedJson_returnsEmpty() {
        assertEquals("{}", analyzer.analyze("not json"));
    }
}