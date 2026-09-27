package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * 系统属性信息分析器。
 * <p>
 * 消费 zinfo zSystemPropInfo.cpp 全量上报的 {属性名 -> {value, serial}} 原始数据。
 * 判定维度：
 * 1. 关键属性期望值(prop_map):值不在期望列表 → error "value is not correct"
 * 2. 关键 ro.* 属性 serial_version != 0 → error "serial_version is not 0"(被篡改)
 * 3. Community ROM 指纹(属性存在) → warn "custom rom"(lineage/cm/mokee/rr/pixelexperience/modversion)
 * 4. 分区 fingerprint 一致性:主 fingerprint 与各分区 fingerprint 不一致 → warn "fingerprint mismatch"(厂商 ROM 正常现象,不判 error)
 * 5. 模拟器/云手机属性(ro.kernel.qemu / ro.hardware.virtual / cloudphone 家族)命中 → warn/error
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

    /** Community ROM 指纹属性(存在即大概率第三方 ROM;来自 RiskEngine list_allowed_properties)。命中 → warn。 */
    private static final String[] COMMUNITY_ROM_PROPS = {
            "ro.lineage.version",
            "ro.cm.version",
            "ro.mokee.version",
            "ro.rr.version",
            "ro.pixelexperience.version",
            "ro.modversion",
    };

    /** 模拟器/云手机特征属性(来自 RiskEngine list_allowed_properties)。命中 → warn。 */
    private static final String[] EMULATOR_CLOUD_PROPS = {
            "ro.kernel.qemu",       // qemu 模拟器
            "ro.boot.qemu",
            "ro.hardware.virtual",
            "ro.boot.cloudphone",   // 云手机
            "persist.sys.cloudphone",
            "ro.armcloud",
            "ro.cloud.model",
            "ro.vendor.cloudphone",
            "ro.boot.redroid",      // redroid 容器
    };

    /** 分区 fingerprint 属性(应与 ro.build.fingerprint 一致;来自 RiskEngine 分区指纹集合)。 */
    private static final String[] PARTITION_FINGERPRINT_PROPS = {
            "ro.build.fingerprint",          // 主指纹
            "ro.system.build.fingerprint",
            "ro.vendor.build.fingerprint",
            "ro.odm.build.fingerprint",
            "ro.product.build.fingerprint",
            "ro.system_ext.build.fingerprint",
            "ro.bootimage.build.fingerprint",
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

            // 4) Community ROM 指纹:属性存在 → warn(第三方 ROM 信号)
            for (String name : COMMUNITY_ROM_PROPS) {
                if (raw.has(name)) {
                    out.put(name, new JSONObject()
                            .put("risk", "warn")
                            .put("explain", "custom rom"));
                }
            }

            // 5) 模拟器/云手机特征属性:命中 → warn
            for (String name : EMULATOR_CLOUD_PROPS) {
                JSONObject item = raw.optJSONObject(name);
                if (item != null && !item.optString("value", "").isEmpty()) {
                    out.put(name, new JSONObject()
                            .put("risk", "warn")
                            .put("explain", "emulator/cloud property"));
                }
            }

            // 6) 分区 fingerprint 一致性:各分区指纹须与主指纹(ro.build.fingerprint)一致。
            //    不一致 → warn(而非 error):很多厂商 ROM(MIUI/ColorOS 等)各分区指纹与主指纹
            //    不一致是正常现象,error 会大面积误报;warn 提示"指纹不一致,可能被修改"。
            String mainFp = valueOf(raw, "ro.build.fingerprint");
            if (!mainFp.isEmpty()) {
                for (int i = 1; i < PARTITION_FINGERPRINT_PROPS.length; i++) {
                    String name = PARTITION_FINGERPRINT_PROPS[i];
                    String fp = valueOf(raw, name);
                    if (!fp.isEmpty() && !mainFp.equals(fp)) {
                        out.put(name + ":mismatch[" + fp + "]", new JSONObject()
                                .put("risk", "warn")
                                .put("explain", "fingerprint mismatch"));
                    }
                }
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
