package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * Root 文件检测分析器。
 * <p>
 * 迁移自 zinfo zRootStateInfo.cpp 的内联判断：
 * 11 个常见 Root 相关文件路径，存在(原始 value=="1")则判定为 error。
 */
public final class RootStateAnalyzer implements MainApplication.Analyzer {

    /** 常见 Root 相关文件路径黑名单(迁移自 zRootStateInfo.cpp 路径列表)。 */
    private static final String[] BLACK_PATHS = {
            "/sbin/su",
            "/system/bin/su",
            "/system/xbin/su",
            "/data/local/xbin/su",
            "/data/local/bin/su",
            "/system/sd/xbin/su",
            "/system/bin/failsafe/su",
            "/data/local/su",
            "/system/xbin/mu",
            "/system_ext/bin/su",
            "/apex/com.android.runtime/bin/suu",
    };

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // 只发射存在(风险)的项;不存在/良性路径不出现在结果中(risky-only)。
            for (String path : BLACK_PATHS) {
                JSONObject item = raw.optJSONObject(path);
                if (item != null && "1".equals(item.optString("value", ""))) {
                    out.put(AnalyzerUtil.trimKey(path), new JSONObject()
                            .put("risk", "error")
                            .put("explain", "black file but exist"));
                }
            }
            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}