package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * ProcInfoAnalyzer 硬编码数据单测(等价于 zProcInfo.cpp 各子检测器行为;也用于 isoloated_process_info)。
 */
public class ProcInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new ProcInfoAnalyzer();

    @Test
    public void mapsSegmentCountMismatch_emitsError() throws Exception {
        String raw = "{\"libart.so\":{\"value\":\"3:r--p,r-xp,rw-p\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("libart.so"));
        assertEquals("reference count error", out.getJSONObject("libart.so").getString("explain"));
    }

    @Test
    public void mapsPermsMismatch_emitsError() throws Exception {
        String raw = "{\"libc.so\":{\"value\":\"4:rwx-p,r-xp,r--p,rw-p\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("permissions error", out.getJSONObject("libc.so").getString("explain"));
    }

    @Test
    public void mapsValidPerms_noError() throws Exception {
        String raw = "{\"libc.so\":{\"value\":\"4:r--p,r-xp,r--p,rw-p\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 0);
    }

    @Test
    public void odexBlackString_emitsError() throws Exception {
        String raw = "{\"odex.content\":{\"value\":\"1\"},\"odex.base\":{\"value\":\"/data/app/base.apk\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("--inline-max-code-units=0"));
        assertEquals("black string but find in base.odex", out.getJSONObject("--inline-max-code-units=0").getString("explain"));
    }

    @Test
    public void odexNotExists_emitsError() throws Exception {
        String raw = "{\"odex.content\":{\"value\":\"not_exists\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.has("base.odex"));
        assertEquals("base.odex is not exists", out.getJSONObject("base.odex").getString("explain"));
    }

    @Test
    public void mountsBlackName_emitsError() throws Exception {
        String raw = "{\"mounts:3\":{\"value\":\"/data/adb/modules/magisk overlay\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 1);
        assertEquals("black name but in system path", out.getJSONObject(out.keys().next()).getString("explain"));
    }

    @Test
    public void mountsSystemOverlay_emitsError() throws Exception {
        String raw = "{\"mounts:1\":{\"value\":\"/system overlay ro\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 1);
    }

    @Test
    public void taskFridaThread_emitsError() throws Exception {
        String raw = "{\"task:3145\":{\"value\":\"3145 (pool-frida) S\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 1);
        assertEquals("frida hooked this process", out.getJSONObject(out.keys().next()).getString("explain"));
    }

    @Test
    public void attrPrevZygote_emitsError() throws Exception {
        String raw = "{\"prev:0\":{\"value\":\"u:r:zygote:s0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 1);
        assertEquals("magisk is found in prev", out.getJSONObject(out.keys().next()).getString("explain"));
    }

    @Test
    public void netTcpFridaPort_emitsError() throws Exception {
        String raw = "{\"net_tcp:2\":{\"value\":\"0100007F:69A2 00000000:0000 01\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue(out.length() == 1);
        assertEquals("find frida port", out.getJSONObject(out.keys().next()).getString("explain"));
    }

    @Test
    public void netTcpIdaPort_emitsError() throws Exception {
        String raw = "{\"net_tcp:0\":{\"value\":\"0100007F:5D8A 00000000:0000 01\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("find ida port", out.getJSONObject(out.keys().next()).getString("explain"));
    }

    @Test
    public void cleanProcInfo_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"libc.so\":{\"value\":\"4:r--p,r-xp,r--p,rw-p\"}," +
                "\"odex.content\":{\"value\":\"0\"}," +
                "\"mounts:0\":{\"value\":\"/dev/block/dm-0 /system ext4 ro\"}," +
                "\"task:100\":{\"value\":\"100 (main) S\"}," +
                "\"prev:0\":{\"value\":\"u:r:app:s0\"}," +
                "\"net_tcp:0\":{\"value\":\"0100007F:B0B2 0100007F:B0B2 01\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("干净环境应无输出: " + out, out.length() == 0);
    }

    /** ns 不同(隔离 mount namespace)→ error。 */
    @Test
    public void mountNsDiffers_isError() throws Exception {
        String raw = "{" +
                "\"ns.self\":{\"value\":\"mnt:[4026531841]\"}," +
                "\"ns.init\":{\"value\":\"mnt:[4026531840]\"}," +
                "\"mountinfo.self\":{\"value\":\"/ / rw\"}," +
                "\"mountinfo.init\":{\"value\":\"/ / rw\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("ns_differs").getString("risk"));
    }

    /** ns 相同但 init 的关键挂载点在 self 缺失(隐藏)→ error。 */
    @Test
    public void hiddenMount_isError() throws Exception {
        // init 有 /system 挂载点,self 没有
        String raw = "{" +
                "\"ns.self\":{\"value\":\"mnt:[4026531840]\"}," +
                "\"ns.init\":{\"value\":\"mnt:[4026531840]\"}," +
                "\"mountinfo.self\":{\"value\":\"1 2 0:3 / / rw\"}," +   // self 没有 /system
                "\"mountinfo.init\":{\"value\":\"1 2 0:3 / /system rw\"}" +  // init 有 /system
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("应报 mount_hidden: " + out, out.length() > 0);
        assertTrue(!out.has("ns_differs"));
    }

    /** ns 相同且关键挂载点都在 → 无输出。 */
    @Test
    public void mountNsSame_noError() throws Exception {
        String raw = "{" +
                "\"ns.self\":{\"value\":\"mnt:[4026531840]\"}," +
                "\"ns.init\":{\"value\":\"mnt:[4026531840]\"}," +
                "\"mountinfo.self\":{\"value\":\"1 2 0:3 / /system rw\"}," +
                "\"mountinfo.init\":{\"value\":\"1 2 0:3 / /system rw\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("ns 相同且无隐藏应无输出: " + out, out.length() == 0);
    }
}