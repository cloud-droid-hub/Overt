package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * 本地网络信息分析器。
 * <p>
 * 迁移自 zinfo zLocalNetworkInfo.cpp get_local_network_info 的内联判断：
 * 活跃的同网 Overt 设备 IP(value=="overt") → warn。
 */
public final class LocalNetworkAnalyzer implements MainApplication.Analyzer {

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            Iterator<String> keys = raw.keys();
            while (keys.hasNext()) {
                String ip = keys.next();
                JSONObject item = raw.optJSONObject(ip);
                if (item != null && "overt".equals(item.optString("value", ""))) {
                    out.put(AnalyzerUtil.trimKey(ip), new JSONObject()
                            .put("risk", "warn")
                            .put("explain", "overt device"));
                }
            }
            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}