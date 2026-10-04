package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * SelinuxInfoAnalyzer 硬编码数据单测。
 */
public class SelinuxInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SelinuxInfoAnalyzer();

    @Test
    public void normalAppContext_noError() throws Exception {
        String raw = "{\"selinux_context\":{\"value\":\"u:r:untrusted_app:s0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("正常 app 域不应报: " + out, out.length() == 0);
    }

    @Test
    public void magiskContext_isError() throws Exception {
        String raw = "{\"selinux_context\":{\"value\":\"u:r:magisk:s0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("selinux_context").getString("risk"));
        assertEquals("u:r:magisk:s0", out.getJSONObject("selinux_context").getString("explain"));
    }

    @Test
    public void suDomainContext_isError() throws Exception {
        // su 域标记(":su" 后缀)
        String raw = "{\"selinux_context\":{\"value\":\"u:r:system_server:s0:su\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("selinux_context").getString("risk"));
    }

    @Test
    public void unreadableContext_noError() throws Exception {
        String raw = "{\"selinux_context\":{\"value\":\"unreadable\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("unreadable 不应报: " + out, out.length() == 0);
    }

    @Test
    public void zygiskLog_isError() throws Exception {
        String raw = "{\"logcat_record\":{\"value\":\"avc: denied { search } for pid=1800 comm=zygiskd scontext=u:r:su:s0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("logcat_record").getString("risk"));
    }

    @Test
    public void cleanLog_noError() throws Exception {
        String raw = "{\"logcat_record\":{\"value\":\"normal log\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("普通日志不应报: " + out, out.length() == 0);
    }

    @Test
    public void malformedJson_returnsEmpty() {
        assertEquals("{}", analyzer.analyze("not json"));
    }
}