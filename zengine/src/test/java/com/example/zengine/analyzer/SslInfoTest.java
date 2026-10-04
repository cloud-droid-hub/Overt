// Copyright (c) 2025-2026 fei_cong(https://github.com/feicong/feicong-course)

package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

/**
 * SslInfoAnalyzer 动态期望指纹单测(真实网络)。
 * <p>
 * 期望指纹不再硬编码:由 SslFingerprintFetcher 真实请求 baidu 现场获取。
 * 测试直接依赖外网(https://www.baidu.com 可达);无网环境下 fetchFromNetwork 返回 null。
 */
public class SslInfoTest {

    private final MainApplication.Analyzer analyzer = new SslInfoAnalyzer();
    private static final String BAIDU_URL = "https://www.baidu.com";

    @Test
    public void fetchFingerprint() throws Exception {
        // 真实请求 baidu,应能拿到叶子证书 SHA-256 指纹(64 hex)
        String fp = SslFingerprintFetcher.getExpectedFingerprint(BAIDU_URL);
        assertNotNull("真实网络应拿到指纹", fp);
        assertTrue("指纹应是 64 hex: " + fp, fp.matches("[0-9A-F]{64}"));
    }

    @Test
    public void certMatches() throws Exception {
        // 先拿 zengine 动态期望指纹,作为"观察指纹"填入(native 与 zengine 一致 → 不报错)
        String expectedFp = SslFingerprintFetcher.getExpectedFingerprint(BAIDU_URL);
        if (expectedFp == null) return; // 无网络,跳过

        String raw = "{" +
                "\"" + BAIDU_URL + "\":{\"value\":\"" + expectedFp + "\",\"error\":\"\"}," +
                "\"location\":{\"value\":\"中国北京市\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertFalse("指纹一致不应有URL错误", out.has(BAIDU_URL));
        assertEquals("safe", out.getJSONObject("location").getString("risk"));
    }

    @Test
    public void certMismatch() throws Exception {
        String raw = "{" +
                "\"" + BAIDU_URL + "\":{\"value\":\"ABCDEF\",\"error\":\"\"}," +
                "\"location\":{\"value\":\"中国\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        // 若网络不可达 → expectedFp=null → warn;否则(能拿到真实指纹)不匹配 → error
        String expectedFp = SslFingerprintFetcher.getExpectedFingerprint(BAIDU_URL);
        if (expectedFp == null) {
            assertEquals("warn", out.getJSONObject(BAIDU_URL).getString("risk"));
        } else {
            assertEquals("error", out.getJSONObject(BAIDU_URL).getString("risk"));
            assertEquals("Certificate Fingerprint is wrong ABCDEF",
                    out.getJSONObject(BAIDU_URL).getString("explain"));
        }
    }

    @Test
    public void certError() throws Exception {
        String raw = "{" +
                "\"" + BAIDU_URL + "\":{\"value\":\"\",\"error\":\"connection failed\"}," +
                "\"location\":{\"value\":\"中国\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject(BAIDU_URL).getString("risk"));
        assertEquals("connection failed", out.getJSONObject(BAIDU_URL).getString("explain"));
    }

    @Test
    public void foreignExit() throws Exception {
        String[] locations = {
                "United States", "美国纽约", "Canada", "Germany", "日本", "中国北京市"
        };
        for (String location : locations) {
            String raw = new JSONObject().put("location",
                    new JSONObject().put("value", location)).toString();
            JSONObject item = new JSONObject(analyzer.analyze(raw)).getJSONObject("location");
            assertEquals(location, "safe", item.getString("risk"));
            assertEquals(location, item.getString("explain"));
        }
    }

    @Test
    public void emptyLocation() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"location\":{\"value\":\"\"}}"));
        assertEquals("error", out.getJSONObject("location").getString("risk"));
        assertEquals("get_location failed", out.getJSONObject("location").getString("explain"));
    }

    @Test
    public void noLocation() throws Exception {
        JSONObject item = new JSONObject(analyzer.analyze("{}")).getJSONObject("location");
        assertEquals("error", item.getString("risk"));
        assertEquals("get_location failed", item.getString("explain"));
    }

    @Test
    public void blankLocation() throws Exception {
        String raw = new JSONObject().put("location",
                new JSONObject().put("value", " \t ")).toString();
        JSONObject item = new JSONObject(analyzer.analyze(raw)).getJSONObject("location");
        assertEquals("error", item.getString("risk"));
        assertEquals("get_location failed", item.getString("explain"));
    }
}
