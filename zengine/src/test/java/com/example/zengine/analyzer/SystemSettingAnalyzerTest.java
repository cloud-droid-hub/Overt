package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * SystemSettingAnalyzer 硬编码数据单测(等价于 zSystemSettingInfo.cpp 行为)。
 * 查杀分离后采集端按数据源分组上报扁平键值对:
 *   settings_info / battery_info / telephony_info / network_info /
 *   biometric_info / installer_info / keyguard_info
 * 本测试验证各分组的判定逻辑。
 */
public class SystemSettingAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SystemSettingAnalyzer();

    @Test
    public void riskySettings_emitErrors() throws Exception {
        String raw = "{" +
                "\"settings_info\":{" +
                "  \"global:adb_enabled\":\"1\"," +
                "  \"secure:development_settings_enabled\":\"1\"," +
                "  \"global:http_proxy\":\"10.0.0.1:8080\"" +
                "}," +
                "\"battery_info\":{\"status\":\"2\",\"level\":\"50\"}," +
                "\"telephony_info\":{\"getSimState\":\"1\"}," +
                "\"network_info\":{\"0\":\"0,1,4\"}," +
                "\"installer_info\":{\"installer\":\"com.unknown.store\"}," +
                "\"keyguard_info\":{\"password\":\"0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("usb_debug").getString("risk"));
        assertEquals("error", out.getJSONObject("developer_mode").getString("risk"));
        assertEquals("error", out.getJSONObject("proxy").getString("risk"));
        assertEquals("warn",  out.getJSONObject("battery").getString("risk"));
        assertEquals("error", out.getJSONObject("sim").getString("risk"));
        assertEquals("error", out.getJSONObject("vpn").getString("risk"));
        assertEquals("warn",  out.getJSONObject("installer").getString("risk"));
        assertEquals("warn",  out.getJSONObject("password").getString("risk"));
    }

    @Test
    public void officialMarket_noInstallerWarn() throws Exception {
        String raw = "{\"installer_info\":{\"installer\":\"com.xiaomi.market\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("官方市场不应有 installer 警告", !out.has("installer"));
    }

    @Test
    public void biometricSuccess_emitsNothing() throws Exception {
        String raw = "{\"biometric_info\":{\"fingerprint_feature\":\"1\",\"biometric_status\":\"0\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("success 无输出", !out.has("biometric_status") && !out.has("fingerprint_feature"));
    }

    @Test
    public void biometricNotReady_emitsWarn() throws Exception {
        String[] badCodes = {"7", "11", "1", "13", "15"};
        for (String code : badCodes) {
            String raw = "{\"biometric_info\":{\"fingerprint_feature\":\"1\",\"biometric_status\":\"" + code + "\"}}";
            JSONObject out = new JSONObject(analyzer.analyze(raw));
            assertEquals("编码 " + code + " 应为 warn",
                    "warn", out.getJSONObject("biometric_status").getString("risk"));
        }
    }

    @Test
    public void biometricNoHardware_emitsWarn() throws Exception {
        String raw = "{\"biometric_info\":{\"fingerprint_feature\":\"0\",\"biometric_status\":\"11\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("fingerprint_feature").getString("risk"));
    }

    @Test
    public void biometricUnknown_skipped() throws Exception {
        String[] unknownCodes = {"-1", "16", "95"};
        for (String code : unknownCodes) {
            String raw = "{\"biometric_info\":{\"fingerprint_feature\":\"1\",\"biometric_status\":\"" + code + "\"}}";
            JSONObject out = new JSONObject(analyzer.analyze(raw));
            assertTrue("未知 " + code + " 无输出", !out.has("biometric_status"));
        }
    }

    @Test
    public void vpn_viaNetworkInfo() throws Exception {
        String raw = "{\"network_info\":{\"0\":\"0,1,2\",\"1\":\"4\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("vpn").getString("risk"));
    }

    @Test
    public void vpn_wifiOnly_noOutput() throws Exception {
        String raw = "{\"network_info\":{\"0\":\"0,1,2\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("wifi 无 VPN 不输出", !out.has("vpn"));
    }

    @Test
    public void simPresent_viaTelephonyInfo() throws Exception {
        // SIM READY(5) → 不报
        String raw = "{\"telephony_info\":{\"getSimState\":\"5\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("有卡 SIM 不报", !out.has("sim"));
    }

    @Test
    public void cleanFullStructure_emitsEmpty() throws Exception {
        String raw = "{" +
                "\"settings_info\":{" +
                "  \"global:adb_enabled\":\"0\"," +
                "  \"secure:development_settings_enabled\":\"0\"," +
                "  \"global:http_proxy\":\"\"" +
                "}," +
                "\"battery_info\":{\"status\":\"3\",\"level\":\"50\"}," +
                "\"telephony_info\":{\"getSimState\":\"5\",\"getSimOperator\":\"46000\"}," +
                "\"network_info\":{\"0\":\"0,1\"}," +
                "\"installer_info\":{\"installer\":\"com.xiaomi.market\"}," +
                "\"biometric_info\":{\"fingerprint_feature\":\"1\",\"biometric_status\":\"0\"}," +
                "\"keyguard_info\":{\"password\":\"1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("干净设备无输出", out.length() == 0);
    }
}