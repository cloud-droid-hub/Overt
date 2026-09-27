package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * SSL 证书信息分析器。
 * <p>
 * 迁移自 zinfo zSslInfo.cpp get_ssl_info 的内联判断：
 * - 对每个 URL:error 非空 → error；观察指纹 != 期望指纹 → error
 * - location:空 → error；不以"中国"开头 → error；否则 safe
 */
public final class SslInfoAnalyzer implements MainApplication.Analyzer {

    /** URL → 期望证书 SHA256 指纹(迁移自 C++ url_info 表)。 */
    private static final String[][] URL_FINGERPRINT = {
            {"https://www.baidu.com", "0D822C9A905AEFE98F3712C0E02630EE95332C455FE7745DF08DBC79F4B0A149"},
    };

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // URL 证书指纹比对(原始 key = URL)
            for (String[] uf : URL_FINGERPRINT) {
                String url = uf[0];
                String expectedFp = uf[1];
                JSONObject item = raw.optJSONObject(url);
                if (item == null) continue;

                String error = item.optString("error", "");
                String observedFp = item.optString("value", "");

                if (!error.isEmpty()) {
                    out.put(url, new JSONObject()
                            .put("risk", "error")
                            .put("explain", error));
                } else if (!expectedFp.equals(observedFp)) {
                    out.put(url, new JSONObject()
                            .put("risk", "error")
                            .put("explain", "Certificate Fingerprint is wrong " + observedFp));
                }
            }

            // 地理位置判定
            String location = valueOf(raw, "location");
            if (location.isEmpty()) {
                out.put("location", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "get_location failed"));
            } else if (location.startsWith("中国")) {
                out.put("location", new JSONObject()
                        .put("risk", "safe")
                        .put("explain", location));
            } else {
                out.put("location", new JSONObject()
                        .put("risk", "error")
                        .put("explain", location));
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