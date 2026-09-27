package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * PackageInfoAnalyzer 硬编码数据单测(等价于 zPackageInfo.cpp 行为)。
 */
public class PackageInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new PackageInfoAnalyzer();

    @Test
    public void blackInstalled_isError() throws Exception {
        String raw = "{" +
                "\"com.topjohnwu.magisk\":{\"value\":\"pms\"}," +
                "\"com.example.normalapp\":{\"value\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("com.topjohnwu.magisk"));
        assertEquals("error", out.getJSONObject("com.topjohnwu.magisk").getString("risk"));
        assertFalse("未安装的普通包不应出现", out.has("com.example.normalapp"));
    }

    @Test
    public void whiteMissing_isWarn() throws Exception {
        String raw = "{\"com.tencent.mm\":{\"value\":\"0\"},\"com.eg.android.AlipayGphone\":{\"value\":\"1\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("com.tencent.mm"));
        assertEquals("warn", out.getJSONObject("com.tencent.mm").getString("risk"));
        assertEquals("white package name but uninstall 微信", out.getJSONObject("com.tencent.mm").getString("explain"));
        assertFalse("已装的支付宝不应警告", out.has("com.eg.android.AlipayGphone"));
    }

    @Test
    public void clean_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"com.topjohnwu.magisk\":{\"value\":\"0\"}," +
                "\"com.tencent.mm\":{\"value\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 0);
    }

    /** 黑名单命中 → explain 带应用名(内置名单的语义)。 */
    @Test
    public void blackInstalled_explainHasAppName() throws Exception {
        String raw = "{\"com.topjohnwu.magisk\":{\"value\":\"file\"},\"org.lsposed.manager\":{\"value\":\"path_hole\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("black package name but install[file] Magisk", out.getJSONObject("com.topjohnwu.magisk").getString("explain"));
        assertEquals("black package name but install[path_hole] LSPosed", out.getJSONObject("org.lsposed.manager").getString("explain"));
    }

    /** 关键查杀分离语义：非黑名单、非白名单的探测对象已安装 → 不报 error(判定只对名单内包生效)。 */
    @Test
    public void nonBlacklistProbeInstalled_noError() throws Exception {
        // 这些包在采集端探测列表里但不在黑/白名单(如普通试玩包),已安装不应被判 error
        String raw = "{" +
                "\"com.example.somegame\":{\"value\":\"pms\"}," +
                "\"com.blank.app\":{\"value\":\"file\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("非名单包已安装不应报 error: " + out, out.length() == 0);
    }

    /** 黑名单包以任意方式安装(0 之外)都应 error。 */
    @Test
    public void blackInstalled_viaAnyMethod_isError() throws Exception {
        String raw = "{\"com.github.kr328.clash\":{\"value\":\"shell_hole\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("com.github.kr328.clash"));
        assertEquals("error", out.getJSONObject("com.github.kr328.clash").getString("risk"));
    }
}