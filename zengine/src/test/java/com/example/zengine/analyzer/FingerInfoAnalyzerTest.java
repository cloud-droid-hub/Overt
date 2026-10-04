package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * FingerInfoAnalyzer 硬编码数据单测(等价于 zFingerInfo.cpp 行为)。
 */
public class FingerInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new FingerInfoAnalyzer();

    @Test
    public void allPresent_emitsSafe() throws Exception {
        String raw = "{" +
                "\"android_id\":{\"value\":\"abc123\"}," +
                "\"drm_id\":{\"value\":\"drm-xyz\"}," +
                "\"weixin_finger\":{\"value\":\"wx\"}," +
                "\"weixin_apk_path\":{\"value\":\"/data/app/weixin\"}," +
                "\"boot_id\":{\"value\":\"boot-1\"}," +
                "\"build_prop_finger\":{\"value\":\"fsid_dev_ino\"}," +
                "\"data_finger\":{\"value\":\"blocks_bsize_files\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("safe", out.getJSONObject("android_id").getString("risk"));
        assertEquals("safe", out.getJSONObject("drm_id").getString("risk"));
        assertEquals("safe", out.getJSONObject("weixin_finger").getString("risk"));
        assertEquals("/data/app/weixin", out.getJSONObject("weixin_finger").getString("explain"));
        assertEquals("safe", out.getJSONObject("boot_id").getString("risk"));
        assertEquals("safe", out.getJSONObject("build_prop_finger").getString("risk"));
        assertEquals("safe", out.getJSONObject("data_finger").getString("risk"));
    }

    @Test
    public void emptyDrmId_isError() throws Exception {
        String raw = "{\"drm_id\":{\"value\":\"\"},\"boot_id\":{\"value\":\"ok\"},\"weixin_finger\":{\"value\":\"wx\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("drm_id").getString("risk"));
        assertEquals("drm_id is empty", out.getJSONObject("drm_id").getString("explain"));
    }

    @Test
    public void emptyBootId_isError() throws Exception {
        String raw = "{\"boot_id\":{\"value\":\"\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("boot_id").getString("risk"));
    }

    @Test
    public void emptyWeixinFinger_isWarn() throws Exception {
        String raw = "{\"weixin_finger\":{\"value\":\"\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("weixin_finger").getString("risk"));
    }
}