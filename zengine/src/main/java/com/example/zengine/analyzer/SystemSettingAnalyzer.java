package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 系统设置信息分析器。
 * <p>
 * 迁移自 zinfo zSystemSettingInfo.cpp get_system_setting_info 的内联判断：
 * - battery=="1" 充电 → warn
 * - installer 不在官方市场白名单 → warn
 * - sim=="0" 无 SIM → error
 * - developer_mode=="1" → error
 * - usb_debug=="1" → error
 * - proxy=="1" → error
 * - password=="0" 未设锁屏密码 → warn
 * - vpn=="1" → error
 */
public final class SystemSettingAnalyzer implements MainApplication.Analyzer {

    /** 官方应用市场白名单(迁移自 C++ market_name_list)。 */
    private static final String[] OFFICIAL_MARKETS = {
            "com.oppo.market", "com.bbk.appstore", "com.xiaomi.market",
            "com.huawei.appmarket", "com.hihonor.appmarket",
    };

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            if ("1".equals(valueOf(raw, "battery"))) {
                out.put("battery", new JSONObject()
                        .put("risk", "warn")
                        .put("explain", "phone is being charged"));
            }

            String installer = valueOf(raw, "installer");
            if (!isOfficialMarket(installer)) {
                out.put("installer", new JSONObject()
                        .put("risk", "warn")
                        .put("explain", "not install from official app market [" + installer + "]"));
            }

            if ("0".equals(valueOf(raw, "sim"))) {
                out.put("sim", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "no sim card"));
            }

            if ("1".equals(valueOf(raw, "developer_mode"))) {
                out.put("developer_mode", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "developer mode is enabled"));
            }

            if ("1".equals(valueOf(raw, "usb_debug"))) {
                out.put("usb_debug", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "usb debugging is enabled"));
            }

            if ("1".equals(valueOf(raw, "proxy"))) {
                out.put("proxy", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "proxy is enabled"));
            }

            if ("0".equals(valueOf(raw, "password"))) {
                out.put("password", new JSONObject()
                        .put("risk", "warn")
                        .put("explain", "lock screen password is not set"));
            }

            if ("1".equals(valueOf(raw, "vpn"))) {
                out.put("vpn", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "vpn is enable"));
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    private static boolean isOfficialMarket(String installer) {
        if (installer == null || installer.isEmpty()) return false;
        for (String m : OFFICIAL_MARKETS) {
            if (m.equals(installer)) return true;
        }
        return false;
    }

    private static String valueOf(JSONObject raw, String key) {
        JSONObject item = raw.optJSONObject(key);
        return (item == null) ? "" : item.optString("value", "");
    }
}