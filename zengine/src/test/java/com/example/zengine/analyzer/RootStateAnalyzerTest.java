package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * RootStateAnalyzer 硬编码数据单测。
 * 喂原始 JSON(采集端全量输出信息),断言仅黑名单路径存在时输出 error。
 */
public class RootStateAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new RootStateAnalyzer();

    /** 全量原始数据:黑路径存在 + 良性路径存在。 */
    @Test
    public void blackPathExists_emitsError() throws Exception {
        String raw = "{" +
                "\"/sbin/su\":{\"value\":\"1\"}," +
                "\"/system/bin/su\":{\"value\":\"0\"}," +
                "\"/data/local/bin/other\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));

        assertTrue("黑路径应命中", out.has("/sbin/su"));
        assertEquals("error", out.getJSONObject("/sbin/su").getString("risk"));
        assertEquals("black file but exist", out.getJSONObject("/sbin/su").getString("explain"));
        assertFalse("不存在的黑路径不应出现", out.has("/system/bin/su"));
        assertFalse("非黑名单路径不应出现", out.has("/data/local/bin/other"));
    }

    /** 全部安全:无输出。 */
    @Test
    public void allSafe_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"/sbin/su\":{\"value\":\"0\"}," +
                "\"/system/bin/su\":{\"value\":\"0\"}" +
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