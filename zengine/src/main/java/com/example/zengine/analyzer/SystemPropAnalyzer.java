package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 系统属性信息分析器。
 * <p>
 * 迁移自 zinfo zSystemPropInfo.cpp get_system_prop_info 的内联判断：
 * - 对关键属性(prop_map)：值不在期望值列表 → error "value is not correct"
 * - 对 ro.* 属性序列号版本 serial != 0 → error "serial_version is not 0"
 */
public final class SystemPropAnalyzer implements MainApplication.Analyzer {

    /** 关键属性名 → 期望值集合(迁移自 C++ prop_map)。 */
    private static final String[][] PROP_EXPECTED = {
            {"ro.secure", "1"},
            {"ro.debuggable", "0"},
            {"ro.boot.flash.locked", "1"},
            {"ro.dalvik.vm.native.bridge", "0"},
            {"ro.boot.vbmeta.device_state", "locked"},
            {"ro.boot.verifiedbootstate", "green"},
            {"ro.boot.veritymode", "enforcing"},
            {"ro.build.tags", "release-keys"},
            {"ro.bootimage.build.tags", "release-keys"},
            {"ro.system.build.tags", "release-keys"},
            {"ro.vendor.build.tags", "release-keys"},
            {"ro.build.type", "user"},
            {"init.svc.adbd", "stopped"},
            {"sys.usap.enable", "true"},
    };

    /** 允许取多个值的属性(USB 配置等)。 */
    private static final String[][] PROP_ALLOWED_VALUES = {
            {"persist.sys.usb.config", "mtp", "ptp", "none", ""},
            {"persist.security.adbinput", "0"},
    };

    /** 需要做 serial_version!=0 检查的 ro.* 属性(与原 C++ prop_map 内的 ro.* 集合一致,不放大范围)。 */
    private static final String[] SERIAL_CHECK_KEYS = {
            "ro.secure",
            "ro.debuggable",
            "ro.boot.flash.locked",
            "ro.dalvik.vm.native.bridge",
            "ro.boot.vbmeta.device_state",
            "ro.boot.verifiedbootstate",
            "ro.boot.veritymode",
            "ro.build.tags",
            "ro.bootimage.build.tags",
            "ro.system.build.tags",
            "ro.vendor.build.tags",
            "ro.build.type",
    };

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // 1) 单值期望属性
            for (String[] entry : PROP_EXPECTED) {
                String name = entry[0];
                String expected = entry[1];
                JSONObject item = raw.optJSONObject(name);
                if (item == null) continue;
                String value = item.optString("value", "");
                if (!expected.equals(value)) {
                    out.put(name + ":value[" + value + "]", new JSONObject()
                            .put("risk", "error")
                            .put("explain", "value is not correct"));
                }
            }

            // 2) 多值允许属性
            for (String[] entry : PROP_ALLOWED_VALUES) {
                String name = entry[0];
                JSONObject item = raw.optJSONObject(name);
                if (item == null) continue;
                String value = item.optString("value", "");
                boolean ok = false;
                for (int i = 1; i < entry.length; i++) {
                    if (entry[i].equals(value)) { ok = true; break; }
                }
                if (!ok) {
                    out.put(name + ":value[" + value + "]", new JSONObject()
                            .put("risk", "error")
                            .put("explain", "value is not correct"));
                }
            }

            // 3) 关键 ro.* 属性序列号版本检查(serial != 0 表示被修改；仅检查与 C++ 一致的关键属性，不遍历全部 ro.*)
            for (String name : SERIAL_CHECK_KEYS) {
                JSONObject item = raw.optJSONObject(name);
                if (item == null) continue;
                String serial = item.optString("serial", "0");
                if (!"0".equals(serial)) {
                    out.put(name + ":serial[" + serial + "]", new JSONObject()
                            .put("risk", "error")
                            .put("explain", "serial_version is not 0"));
                }
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}