package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * SSL 证书信息分析器。
 * <p>
 * 迁移自 zinfo zSslInfo.cpp get_ssl_info 的内联判断：
 * - 对每个 URL:error 非空 → error；观察指纹 != 期望指纹 → error
 *   (期望指纹不再硬编码,由 zengine 动态获取:SslFingerprintFetcher 重新请求目标 URL 现场解析)
 * - location:空 → error；不以"中国"开头 → error；否则 safe
 */
public final class SslInfoAnalyzer implements MainApplication.Analyzer {

    /** 需要检测证书指纹的目标 URL(仅探测范围;期望指纹动态获取,不硬编码)。
     *  与采集端 zSslInfo.cpp 的 urls[] 保持一致。 */
    private static final String[] URLS = {
            "https://www.baidu.com",
            "https://r.inews.qq.com/api/ip2city",   // 腾讯新闻 ip2city(地理位置)
    };

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // URL 证书指纹比对(原始 key = URL)
            for (String url : URLS) {
                JSONObject item = raw.optJSONObject(url);
                if (item == null) continue;

                String error = item.optString("error", "");
                String observedFp = item.optString("value", "");

                if (!error.isEmpty()) {
                    out.put(url, new JSONObject()
                            .put("risk", "error")
                            .put("explain", error));
                    continue;
                }

                // 动态获取期望指纹(zengine 重新请求现场解析,不硬编码)
                String expectedFp = SslFingerprintFetcher.getExpectedFingerprint(url);
                if (expectedFp == null) {
                    // 期望值获取失败(network 失败)→ 无法比对,不武断报 error,提示
                    out.put(url, new JSONObject()
                            .put("risk", "warn")
                            .put("explain", "cannot fetch expected fingerprint"));
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