package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * ClassLoaderAnalyzer 硬编码数据单测。
 */
public class ClassLoaderAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new ClassLoaderAnalyzer();

    /** LSPosed 注入的类加载器 → error。 */
    @Test
    public void lspModuleClassLoader_emitsError() throws Exception {
        String raw = "{" +
                "\"dalvik.system.PathClassLoader[LspModuleClassLoader@0x123]\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 1);
        assertEquals("error", out.getJSONObject(out.keys().next()).getString("risk"));
    }

    /** InMemoryDexClassLoader 但无 cookie 标记 → 不命中(保留原判定语义)。 */
    @Test
    public void inMemoryDex_withoutCookie_notHit() throws Exception {
        String raw = "{" +
                "\"dalvik.system.InMemoryDexClassLoader@0x456\":{\"value\":\"1\"}," +
                "\"com.example.overt.MainActivity\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("无 cookie 标记不应命中", out.length() == 0);
    }

    /** InMemoryDexClassLoader 且含 cookie 标记 → error。 */
    @Test
    public void inMemoryDex_withCookie_emitsError() throws Exception {
        String raw = "{" +
                "\"dalvik.system.InMemoryDexClassLoader[InMemoryDexFile[cookie=[0, -1045552]]]\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 1);
        assertEquals("error", out.getJSONObject(out.keys().next()).getString("risk"));
    }

    /** 良性类加载器 → 空输出。 */
    @Test
    public void benignLoader_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"dalvik.system.PathClassLoader[DexPathList[[zip file \"/data/app/com.example.overt/base.apk\"]]]\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 0);
    }

    /** 超长且命中的 key → 截断,不抛异常。 */
    @Test
    public void veryLongKey_truncated() throws Exception {
        // 超长前缀 + 命中黑名单特征(LspModuleClassLoader),确保走到 trimKey 路径
        StringBuilder longKey = new StringBuilder();
        for (int i = 0; i < 2000; i++) longKey.append('a');
        longKey.append("LspModuleClassLoader");
        String raw = "{\"" + longKey + "\":{\"value\":\"1\"}}";

        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("应仍能解析出 1 个 key", out.length() == 1);
        String actualKey = out.keys().next();
        assertTrue("key 应被截断为 <= 513 字符", actualKey.length() <= 513);
        assertTrue("截断后应以省略号结尾", actualKey.endsWith("…"));
        // 命中逻辑在被截断前已判定,输出 risk=error
        assertEquals("error", out.getJSONObject(actualKey).getString("risk"));
    }
}