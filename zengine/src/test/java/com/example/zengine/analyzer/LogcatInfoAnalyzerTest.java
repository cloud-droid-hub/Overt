package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * LogcatInfoAnalyzer 硬编码数据单测(等价于 zLogcatInfo.cpp 行为)。
 */
public class LogcatInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new LogcatInfoAnalyzer();

    @Test
    public void zygiskMark_emitsError() throws Exception {
        String raw = "{\"logcat_record\":{\"value\":\"avc: denied { search } for pid=1800 comm=zygiskd scontext=u:r:su:s0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("logcat_record").getString("risk"));
        assertEquals("avc: denied { search } for pid=1800 comm=zygiskd scontext=u:r:su:s0",
                out.getJSONObject("logcat_record").getString("explain"));
    }

    @Test
    public void noMark_emitsEmpty() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"logcat_record\":{\"value\":\"normal log\"}}"));
        assertTrue(out.length() == 0);
    }

    @Test
    public void empty_emitsEmpty() {
        assertEquals("{}", analyzer.analyze("{}"));
    }
}