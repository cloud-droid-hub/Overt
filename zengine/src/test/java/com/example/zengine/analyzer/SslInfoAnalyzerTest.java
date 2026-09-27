package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

/**
 * SslInfoAnalyzer 硬编码数据单测(等价于 zSslInfo.cpp 行为)。
 */
public class SslInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SslInfoAnalyzer();
    private static final String BAIDU_FP = "0D822C9A905AEFE98F3712C0E02630EE95332C455FE7745DF08DBC79F4B0A149";

    @Test
    public void correctFingerprint_noError() throws Exception {
        String raw = "{" +
                "\"https://www.baidu.com\":{\"value\":\"" + BAIDU_FP + "\",\"error\":\"\"}," +
                "\"location\":{\"value\":\"中国北京市\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertFalse("指纹正确不应有URL错误", out.has("https://www.baidu.com"));
        assertEquals("safe", out.getJSONObject("location").getString("risk"));
    }

    @Test
    public void wrongFingerprint_isError() throws Exception {
        String raw = "{" +
                "\"https://www.baidu.com\":{\"value\":\"ABCDEF\",\"error\":\"\"}," +
                "\"location\":{\"value\":\"中国\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("https://www.baidu.com").getString("risk"));
        assertEquals("Certificate Fingerprint is wrong ABCDEF", out.getJSONObject("https://www.baidu.com").getString("explain"));
    }

    @Test
    public void errorMessage_isError() throws Exception {
        String raw = "{" +
                "\"https://www.baidu.com\":{\"value\":\"\",\"error\":\"connection failed\"}," +
                "\"location\":{\"value\":\"中国\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("https://www.baidu.com").getString("risk"));
        assertEquals("connection failed", out.getJSONObject("https://www.baidu.com").getString("explain"));
    }

    @Test
    public void foreignLocation_isError() throws Exception {
        String raw = "{\"location\":{\"value\":\"United States\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("location").getString("risk"));
    }

    @Test
    public void emptyLocation_isError() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"location\":{\"value\":\"\"}}"));
        assertEquals("error", out.getJSONObject("location").getString("risk"));
        assertEquals("get_location failed", out.getJSONObject("location").getString("explain"));
    }
}