package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * 系统设置信息分析器。
 * <p>
 * 查杀分离：采集端(zinfo zSystemSettingInfo.cpp)按数据源分组上报原始键值对，
 * 每个分组一个顶层 key(values 为组内扁平键值对)：
 *   settings_info / battery_info / telephony_info / network_info /
 *   biometric_info / installer_info / keyguard_info
 * 本类负责全部风险判定。
 * <p>
 * 判定表：
 * - settings_info: adb_enabled=1 → usb_debug error; development_settings_enabled=1 → developer_mode error;
 *                  http_proxy 非空 → proxy error
 * - battery_info: status==CHARGING(2)|FULL(5) → warn
 * - telephony_info: getSimState==UNKNOWN(0)|ABSENT(1) → sim error
 * - network_info: 任一值含 VPN(4) → vpn error
 * - biometric_info: fingerprint_feature=0 无指纹硬件 → warn; biometric_status 7/11/1/13/15 → warn
 * - installer_info: installer 非官方市场 → warn
 * - keyguard_info: password=0 未设锁屏 → warn
 */
public final class SystemSettingAnalyzer implements MainApplication.Analyzer {

    /** 官方应用市场白名单(迁移自 C++ market_name_list)。 */
    private static final String[] OFFICIAL_MARKETS = {
            "com.oppo.market", "com.bbk.appstore", "com.xiaomi.market",
            "com.huawei.appmarket", "com.hihonor.appmarket",
    };

    // BatteryManager 状态常量
    private static final int BATTERY_STATUS_CHARGING = 2;
    private static final int BATTERY_STATUS_FULL = 5;
    // TelephonyManager SIM 状态常量
    private static final int SIM_STATE_UNKNOWN = 0;
    private static final int SIM_STATE_ABSENT = 1;
    // NetworkCapabilities transport 常量
    private static final int TRANSPORT_VPN = 4;

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // —— settings_info: 三表合并,键带表前缀(global:/secure:/system:) ——
            JSONObject settings = raw.optJSONObject("settings_info");
            if (settings != null) {
                Iterator<String> sit = settings.keys();
                while (sit.hasNext()) {
                    String name = sit.next();
                    String value = settings.optString(name, "");
                    if ("adb_enabled".equals(keyOf(name)) && "1".equals(value)) {
                        out.put("usb_debug", new JSONObject()
                                .put("risk", "error")
                                .put("explain", "usb debugging is enabled"));
                    } else if ("development_settings_enabled".equals(keyOf(name)) && "1".equals(value)) {
                        out.put("developer_mode", new JSONObject()
                                .put("risk", "error")
                                .put("explain", "developer mode is enabled"));
                    } else if ("http_proxy".equals(keyOf(name)) && !value.isEmpty()) {
                        out.put("proxy", new JSONObject()
                                .put("risk", "error")
                                .put("explain", "system proxy configured: " + value));
                    }
                }
            }

            // —— battery_info ——
            JSONObject battery = raw.optJSONObject("battery_info");
            if (battery != null) {
                String status = battery.optString("status", "");
                if (!status.isEmpty()) {
                    try {
                        int st = Integer.parseInt(status);
                        if (st == BATTERY_STATUS_CHARGING || st == BATTERY_STATUS_FULL) {
                            out.put("battery", new JSONObject()
                                    .put("risk", "warn")
                                    .put("explain", "phone is being charged (status=" + st + ")"));
                        }
                    } catch (NumberFormatException ignored) {
                    }
                }
            }

            // —— telephony_info ——
            JSONObject telephony = raw.optJSONObject("telephony_info");
            if (telephony != null) {
                String simState = telephony.optString("getSimState", "");
                if (!simState.isEmpty()) {
                    try {
                        int st = Integer.parseInt(simState);
                        if (st == SIM_STATE_UNKNOWN || st == SIM_STATE_ABSENT) {
                            out.put("sim", new JSONObject()
                                    .put("risk", "error")
                                    .put("explain", "no sim card (state=" + st + ")"));
                        }
                    } catch (NumberFormatException ignored) {
                    }
                }
            }

            // —— network_info: 任一网络含 VPN transport → vpn error ——
            JSONObject network = raw.optJSONObject("network_info");
            if (network != null) {
                Iterator<String> nit = network.keys();
                while (nit.hasNext()) {
                    String transports = network.optString(nit.next(), "");
                    if (containsTransport(transports, TRANSPORT_VPN)) {
                        out.put("vpn", new JSONObject()
                                .put("risk", "error")
                                .put("explain", "vpn transport detected: " + transports));
                        break;
                    }
                }
            }

            // —— biometric_info ——
            JSONObject biometric = raw.optJSONObject("biometric_info");
            if (biometric != null) {
                String feature = biometric.optString("fingerprint_feature", "");
                if ("0".equals(feature)) {
                    out.put("fingerprint_feature", new JSONObject()
                            .put("risk", "warn")
                            .put("explain", "device has no fingerprint hardware"));
                }
                String bio = biometric.optString("biometric_status", "");
                if (!bio.isEmpty()) {
                    if ("7".equals(bio)) {
                        out.put("biometric_status", new JSONObject()
                                .put("risk", "warn")
                                .put("explain", "fingerprint is not enrolled"));
                    } else if ("11".equals(bio)) {
                        out.put("biometric_status", new JSONObject()
                                .put("risk", "warn")
                                .put("explain", "biometric has no hardware"));
                    } else if ("1".equals(bio) || "13".equals(bio) || "15".equals(bio)) {
                        out.put("biometric_status", new JSONObject()
                                .put("risk", "warn")
                                .put("explain", "biometric is not ready: " + bio));
                    }
                    // "0" 可用 / "-1"/"16"/"95" 未知 → 不报
                }
            }

            // —— installer_info ——
            JSONObject installer = raw.optJSONObject("installer_info");
            if (installer != null) {
                String name = installer.optString("installer", "");
                if (!isOfficialMarket(name)) {
                    out.put("installer", new JSONObject()
                            .put("risk", "warn")
                            .put("explain", "not install from official app market [" + name + "]"));
                }
            }

            // —— keyguard_info ——
            JSONObject keyguard = raw.optJSONObject("keyguard_info");
            if (keyguard != null) {
                if ("0".equals(keyguard.optString("password", ""))) {
                    out.put("password", new JSONObject()
                            .put("risk", "warn")
                            .put("explain", "lock screen password is not set"));
                }
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    /** settings 键 "global:adb_enabled" → "adb_enabled"。 */
    private static String keyOf(String tablePrefixedKey) {
        int idx = tablePrefixedKey.indexOf(':');
        return (idx < 0) ? tablePrefixedKey : tablePrefixedKey.substring(idx + 1);
    }

    /** network_info 值(transport int 逗号串)是否含指定 transport。 */
    private static boolean containsTransport(String value, int transport) {
        if (value == null || value.isEmpty()) return false;
        for (String tok : value.split(",")) {
            String t = tok.trim();
            if (!t.isEmpty()) {
                try {
                    if (Integer.parseInt(t) == transport) return true;
                } catch (NumberFormatException ignored) {
                }
            }
        }
        return false;
    }

    private static boolean isOfficialMarket(String installer) {
        if (installer == null || installer.isEmpty()) return false;
        for (String m : OFFICIAL_MARKETS) {
            if (m.equals(installer)) return true;
        }
        return false;
    }
}