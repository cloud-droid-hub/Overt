package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * ClassInfoAnalyzer 硬编码数据单测。
 */
public class ClassInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new ClassInfoAnalyzer();

    /** 良性类名 + 黑名单类名混存 → 只输出黑名单命中。 */
    @Test
    public void blackClassHit_emitsError_onlyBlack() throws Exception {
        String raw = "{" +
                "\"com.example.overt.MainActivity\":{\"value\":\"1\"}," +
                "\"io.github.libxposed.api.XposedService\":{\"value\":\"1\"}," +
                "\"de.robv.android.xposed.XposedBridge\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));

        assertFalse("良性类不应出现", out.has("com.example.overt.MainActivity"));
        assertTrue(out.has("io.github.libxposed.api.XposedService"));
        assertEquals("error", out.getJSONObject("io.github.libxposed.api.XposedService").getString("risk"));
        assertEquals("Risk: black class", out.getJSONObject("io.github.libxposed.api.XposedService").getString("explain"));
        assertTrue(out.has("de.robv.android.xposed.XposedBridge"));
    }

    /** 只有良性类 → 空。 */
    @Test
    public void onlyBenign_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"com.example.overt.MainActivity\":{\"value\":\"1\"}," +
                "\"java.lang.String\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("良性类不应有任何输出", out.length() == 0);
    }

    /** 空原始 → fail-closed 空。 */
    @Test
    public void emptyRaw_returnsEmpty() {
        assertEquals("{}", analyzer.analyze("{}"));
    }
}