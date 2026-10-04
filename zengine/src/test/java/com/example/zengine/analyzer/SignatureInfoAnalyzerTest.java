package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * SignatureInfoAnalyzer 硬编码数据单测(等价于 zSignatureInfo.cpp 行为)。
 */
public class SignatureInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SignatureInfoAnalyzer();
    private static final String EXPECTED = "4D8ADE7A8C33C37B774F402EF0ED88D69C6E543DC11CAC7C573077EEF2903F6F";

    @Test
    public void matchingSignature_noError() throws Exception {
        String raw = "{\"signature\":{\"value\":\"" + EXPECTED + "\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("匹配签名无输出", out.length() == 0);
    }

    @Test
    public void mismatchingSignature_isError() throws Exception {
        String raw = "{\"signature\":{\"value\":\"AAAAAAAA\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("signature").getString("risk"));
        assertEquals("signature is not " + EXPECTED, out.getJSONObject("signature").getString("explain"));
    }
}