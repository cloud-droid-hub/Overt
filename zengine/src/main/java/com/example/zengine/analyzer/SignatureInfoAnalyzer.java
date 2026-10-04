package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 签名信息分析器。
 * <p>
 * 迁移自 zinfo zSignatureInfo.cpp get_signature_info 的内联判断：
 * APK 签名文件(RSA)的 SHA256 与期望值不一致 → error。
 */
public final class SignatureInfoAnalyzer implements MainApplication.Analyzer {

    /** 期望的 base.apk 签名 SHA256(迁移自 C++ sha256_real 常量)。 */
    private static final String EXPECTED_SHA256 = "4D8ADE7A8C33C37B774F402EF0ED88D69C6E543DC11CAC7C573077EEF2903F6F";

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            JSONObject item = raw.optJSONObject("signature");
            if (item == null) return out.toString();
            String actual = item.optString("value", "");
            if (!EXPECTED_SHA256.equals(actual)) {
                out.put("signature", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "signature is not " + EXPECTED_SHA256));
            }
            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}