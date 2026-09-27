package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * 已加载类检测分析器。
 * <p>
 * 迁移自 zinfo zClassLoaderInfo.cpp get_class_info 的内联判断：
 * 黑名单类名子串(lsposed/lspd/Xposed* 等)命中 → error。
 */
public final class ClassInfoAnalyzer implements MainApplication.Analyzer {

    /** 黑名单类名子串(迁移自 zClassLoaderInfo.cpp black_name_list)。 */
    private static final String[] BLACK_NAME_LIST = {
            "lsposed",
            "lspd",
            "XposedHooker",
            "XposedHelpers",
            "io.github.libxposed.api",
            "XposedInit",
            "XposedBridge",
    };

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            for (String black : BLACK_NAME_LIST) {
                Iterator<String> keys = raw.keys();
                while (keys.hasNext()) {
                    String className = keys.next();
                    if (className != null && className.contains(black)) {
                        out.put(AnalyzerUtil.trimKey(className), new JSONObject()
                                .put("risk", "error")
                                .put("explain", "Risk: black class"));
                    }
                }
            }
            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}