package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * 进程信息分析器(聚合 maps/mounts/task/attr_prev/net_tcp 的原始数据)。
 * <p>
 * 迁移自 zinfo zProcInfo.cpp 各子检测器的内联判断：
 * - maps:libart/libc/libinput 段数 != 4 或权限序列异常 → error
 * - odex.content=="1" 含 --inline-max-code-units=0 → error；odex 不存在 → error
 * - mounts: 含 dex2oat/APatch/shamiko//data/adb/modules 黑名单或 /system overlay → error
 * - task: 含 "gamin"(gmain)/"pool-frida" → error
 * - prev: 含 "zygote" → error
 * - net_tcp: 含 :69A2/:69A3(frida)或 :5D8A(ida) → error
 * - mount ns 对比: self 与 init 的 mnt namespace 不同 → error;
 *   init 的关键挂载点(/system /vendor /product /system_ext /odm /data/adb /sbin /debug_ramdisk /apex)
 *   在 self mountinfo 中缺失 → error(mount 隐藏证据)
 */
public final class ProcInfoAnalyzer implements MainApplication.Analyzer {

    /** mounts 黑名单子串。 */
    private static final String[] MOUNT_BLACK = {"dex2oat", "APatch", "shamiko", "/data/adb/modules"};
    private static final String[] TASK_BLACK = {"gamin", "pool-frida"};
    private static final String[] NET_TCP_BLACK = {":69A2", ":69A3", ":5D8A"};

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();
            Iterator<String> keys = raw.keys();

            while (keys.hasNext()) {
                String key = keys.next();
                JSONObject item = raw.optJSONObject(key);
                if (item == null) continue;
                String value = item.optString("value", "");

                // maps: 关键系统库段数/权限 "N:r--p,r-xp,..."
                if (key.equals("libart.so") || key.equals("libc.so") || key.equals("libinput.so")) {
                    int colon = value.indexOf(':');
                    if (colon > 0) {
                        int segCount;
                        try { segCount = Integer.parseInt(value.substring(0, colon)); }
                        catch (NumberFormatException e) { continue; }
                        String perms = value.substring(colon + 1);
                        if (segCount != 4 || !isPermsValid(perms)) {
                            out.put(key, new JSONObject()
                                    .put("risk", "error")
                                    .put("explain", segCount != 4 ? "reference count error" : "permissions error"));
                        }
                    }
                    continue;
                }

                // base.odex
                if (key.equals("odex.base")) {
                    if (value.startsWith("not_loaded:")) {
                        out.put("base.odex", new JSONObject()
                                .put("risk", "error")
                                .put("explain", "base.odex is not loaded"));
                    }
                    continue;
                }
                if (key.equals("odex.content")) {
                    if ("1".equals(value)) {
                        out.put("--inline-max-code-units=0", new JSONObject()
                                .put("risk", "error")
                                .put("explain", "black string but find in base.odex"));
                    } else if ("not_exists".equals(value)) {
                        out.put("base.odex", new JSONObject()
                                .put("risk", "error")
                                .put("explain", "base.odex is not exists"));
                    }
                    continue;
                }

                // mounts: "mounts:N"
                if (key.startsWith("mounts:")) {
                    for (String black : MOUNT_BLACK) {
                        if (value.contains(black)) {
                            out.put(AnalyzerUtil.trimKey(key), new JSONObject()
                                    .put("risk", "error")
                                    .put("explain", "black name but in system path"));
                            break;
                        }
                    }
                    if (value.contains("/system ") && value.contains("overlay")) {
                        out.put(AnalyzerUtil.trimKey(key), new JSONObject()
                                .put("risk", "error")
                                .put("explain", "black name but in system path"));
                    }
                    continue;
                }

                // task: "task:N"
                if (key.startsWith("task:")) {
                    for (String black : TASK_BLACK) {
                        if (value.contains(black)) {
                            out.put(AnalyzerUtil.trimKey(key), new JSONObject()
                                    .put("risk", "error")
                                    .put("explain", "frida hooked this process"));
                            break;
                        }
                    }
                    continue;
                }

                // prev: "prev:N"
                if (key.startsWith("prev:")) {
                    if (value.contains("zygote")) {
                        out.put(AnalyzerUtil.trimKey(key), new JSONObject()
                                .put("risk", "error")
                                .put("explain", "magisk is found in prev"));
                    }
                    continue;
                }

                // net_tcp: "net_tcp:N"
                if (key.startsWith("net_tcp:")) {
                    for (String portHex : NET_TCP_BLACK) {
                        if (value.contains(portHex)) {
                            String explain = (portHex.equals(":5D8A")) ? "find ida port" : "find frida port";
                            out.put(AnalyzerUtil.trimKey(key), new JSONObject()
                                    .put("risk", "error")
                                    .put("explain", explain));
                            break;
                        }
                    }
                }
            }

            // —— mount namespace 对比判定 ——
            analyzeMountNs(raw, out);

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    /** 关键挂载点前缀(init 有、self 缺失 → 隐藏)。 */
    private static final String[] HIDDEN_MOUNT_PREFIXES = {
            "/system", "/vendor", "/product", "/system_ext", "/odm",
            "/data/adb", "/sbin", "/debug_ramdisk", "/apex",
    };

    /** mount ns 对比:ns 不同 + 关键挂载点隐藏检测。 */
    private static void analyzeMountNs(JSONObject raw, JSONObject out) throws Exception {
        String selfNs = valueOf(raw, "ns.self");
        String initNs = valueOf(raw, "ns.init");
        if (!selfNs.isEmpty() && !initNs.isEmpty() && !selfNs.equals(initNs)) {
            out.put("ns_differs", new JSONObject()
                    .put("risk", "error")
                    .put("explain", "mount namespace differs from init"));
        }

        String selfMountinfo = valueOf(raw, "mountinfo.self");
        String initMountinfo = valueOf(raw, "mountinfo.init");
        if (selfMountinfo.isEmpty() || initMountinfo.isEmpty()) return;

        // 提取 init mountinfo 的所有挂载点(field 5)
        java.util.Set<String> initPoints = extractMountPoints(initMountinfo);
        java.util.Set<String> selfPoints = extractMountPoints(selfMountinfo);

        int missing = 0;
        for (String point : initPoints) {
            if (!isInterestingMount(point)) continue;
            if (!selfPoints.contains(point)) {
                missing++;
                if (missing <= 4) {
                    out.put(AnalyzerUtil.trimKey("mount_hidden:" + point), new JSONObject()
                            .put("risk", "error")
                            .put("explain", "mount hidden"));
                }
            }
        }
        if (missing > 0) {
            out.put("mount_hidden_count", new JSONObject()
                    .put("risk", "error")
                    .put("explain", "hidden mounts: " + missing));
        }
    }

    private static java.util.Set<String> extractMountPoints(String mountinfo) {
        java.util.Set<String> points = new java.util.HashSet<>();
        String[] lines = mountinfo.split("\n");
        for (String line : lines) {
            // mountinfo 字段: id parent major:minor root mountpoint ...
            // 取第 5 个字段(空格分隔,索引4)
            String[] parts = line.split(" ");
            if (parts.length >= 5) {
                points.add(parts[4]);
            }
        }
        return points;
    }

    private static boolean isInterestingMount(String point) {
        for (String prefix : HIDDEN_MOUNT_PREFIXES) {
            if (point.startsWith(prefix)) return true;
        }
        return false;
    }

    private static String valueOf(JSONObject raw, String key) {
        JSONObject item = raw.optJSONObject(key);
        return (item == null) ? "" : item.optString("value", "");
    }

    /** 权限序列必须形如 "r--p,r-xp,r--p,rw-p"(兼容 "--xp"/"rw-p" 变体)。 */
    private static boolean isPermsValid(String perms) {
        String[] segs = perms.split(",");
        if (segs.length != 4) return false;
        boolean s1 = segs[0].equals("r--p");
        boolean s2 = segs[1].equals("r-xp") || segs[1].equals("--xp");
        boolean s3 = segs[2].equals("r--p") || segs[2].equals("rw-p");
        boolean s4 = segs[3].equals("rw-p") || segs[3].equals("r--p");
        return s1 && s2 && s3 && s4;
    }
}