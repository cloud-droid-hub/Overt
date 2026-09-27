package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 设备指纹信息分析器。
 * <p>
 * 迁移自 zinfo zFingerInfo.cpp get_finger_info 的内联判断：
 * - drm_id / boot_id 为空 → error
 * - weixin_finger(微信指纹)为空 → warn
 * - android_id / build_prop_finger / data_finger → 始终 safe(原始值)
 */
public final class FingerInfoAnalyzer implements MainApplication.Analyzer {

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // android_id: 始终 safe(原始值在 explain)
            String androidId = valueOf(raw, "android_id");
            out.put("android_id", new JSONObject()
                    .put("risk", "safe")
                    .put("explain", androidId));

            // drm_id: 空 → error
            String drmId = valueOf(raw, "drm_id");
            if (drmId.isEmpty()) {
                out.put("drm_id", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "drm_id is empty"));
            } else {
                out.put("drm_id", new JSONObject()
                        .put("risk", "safe")
                        .put("explain", drmId));
            }

            // weixin_finger: 空 → warn
            String weixinFinger = valueOf(raw, "weixin_finger");
            if (weixinFinger.isEmpty()) {
                out.put("weixin_finger", new JSONObject()
                        .put("risk", "warn")
                        .put("explain", "weixin_finger is empty"));
            } else {
                out.put("weixin_finger", new JSONObject()
                        .put("risk", "safe")
                        .put("explain", valueOf(raw, "weixin_apk_path")));
            }

            // boot_id: 空 → error
            String bootId = valueOf(raw, "boot_id");
            if (bootId.isEmpty()) {
                out.put("boot_id", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "boot_id is empty"));
            } else {
                out.put("boot_id", new JSONObject()
                        .put("risk", "safe")
                        .put("explain", bootId));
            }

            // build_prop_finger / data_finger: 始终 safe(原始值)
            out.put("build_prop_finger", new JSONObject()
                    .put("risk", "safe")
                    .put("explain", valueOf(raw, "build_prop_finger")));
            out.put("data_finger", new JSONObject()
                    .put("risk", "safe")
                    .put("explain", valueOf(raw, "data_finger")));

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