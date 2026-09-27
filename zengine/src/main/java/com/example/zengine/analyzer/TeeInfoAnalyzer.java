package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * TEE 信息分析器。
 * <p>
 * 迁移自 zinfo zTeeInfo.cpp get_tee_info_openssl 的内联判断：
 * - tee_state: 除 "cert_ok" 外(证书获取/解析失败、env/context 空) → error
 * - device_locked != "1" → error
 * - verified_boot_state != "0"(VERIFIED) → error
 */
public final class TeeInfoAnalyzer implements MainApplication.Analyzer {

    /** VERIFIED_BOOT_STATE_VERIFIED = 0。 */
    private static final String VERIFIED_BOOT_STATE = "0";

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            String teeState = valueOf(raw, "tee_state");
            if (!"cert_ok".equals(teeState)) {
                // 证书不可用 / 解析失败 → 按原逻辑报 error(tee_statue is damage)
                out.put("tee_statue", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "tee_statue is damage"));
                return out.toString();
            }

            // device_locked: "1" 安全,否则 error
            if (!"1".equals(valueOf(raw, "device_locked"))) {
                out.put("device_locked", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "device_locked is unsafe"));
            }

            // verified_boot_state: "0" 已验证,否则 error
            if (!VERIFIED_BOOT_STATE.equals(valueOf(raw, "verified_boot_state"))) {
                out.put("verified_boot_state", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "verified_boot_state is unsafe"));
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    private static String valueOf(JSONObject raw, String key) {
        JSONObject item = raw.optJSONObject(key);
        return (item == null) ? "" : item.optString("value", "");
    }
}