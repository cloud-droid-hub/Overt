// Copyright (c) 2025-2026 feicong(https://github.com/feicong/feicong-course)
package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;
import org.json.JSONObject;
import org.junit.Test;
import org.junit.Assume;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.charset.StandardCharsets;
import static org.junit.Assert.*;

public final class ProcCardTest {
    private JSONObject read(String name) throws Exception {
        String path = System.getenv(name);
        Assume.assumeNotNull(path);
        return new JSONObject(new String(Files.readAllBytes(Paths.get(path)), StandardCharsets.UTF_8));
    }

    @Test
    public void cardReplay() throws Exception {
        JSONObject raw = read("CDH_ISO_JSON");
        assertTrue(raw.length() > 0);
        assertFalse(raw.getJSONObject("libc.so").getString("value").isEmpty());
        assertFalse(raw.getJSONObject("mountinfo.self").getString("value").isEmpty());
        JSONObject out = new JSONObject(MainApplication.analyze("isoloated_process_info", raw.toString()));
        assertEquals(0, out.length());
        System.out.println("actual isolated raw items=" + raw.length() + ", analyzed=" + out);
    }

    @Test
    public void mapError() throws Exception {
        JSONObject raw = read("CDH_ISO_JSON");
        raw.put("libc.so", new JSONObject().put("value", "4:rwx-p,r-xp,r--p,rw-p"));
        JSONObject out = new JSONObject(MainApplication.analyze("isoloated_process_info", raw.toString()));
        assertEquals("error", out.getJSONObject("libc.so").getString("risk"));
        assertEquals("permissions error", out.getJSONObject("libc.so").getString("explain"));
    }

    @Test
    public void netReplay() throws Exception {
        JSONObject raw = read("CDH_NET_JSON");
        JSONObject out = new JSONObject(MainApplication.analyze("local_network_info", raw.toString()));
        assertEquals(0, raw.length());
        assertEquals(0, out.length());
        raw.put("192.0.2.1", new JSONObject().put("value", "overt"));
        out = new JSONObject(MainApplication.analyze("local_network_info", raw.toString()));
        assertEquals("warn", out.getJSONObject("192.0.2.1").getString("risk"));
        System.out.println("actual local network raw items=0; controlled peer remains warn");
    }
}
