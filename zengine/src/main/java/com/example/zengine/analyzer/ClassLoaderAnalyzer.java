package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * 类加载器检测分析器。
 * <p>
 * 迁移自 zinfo zClassLoaderInfo.cpp get_class_loader_info 的内联判断：
 * - "LspModuleClassLoader" → LSPosed 注入
 * - "InMemoryDexClassLoader" 且含内存 DEX cookie 标记 → 内存动态加载(Xposed 常用)
 */
public final class ClassLoaderAnalyzer implements MainApplication.Analyzer {

    private static final String BLACK_LSP = "LspModuleClassLoader";
    private static final String BLACK_INMEM_DEX = "InMemoryDexClassLoader";
    private static final String BLACK_INMEM_DEX_COOKIE = "InMemoryDexFile[cookie=[0, -";

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            Iterator<String> keys = raw.keys();
            while (keys.hasNext()) {
                String loader = keys.next();
                if (isBlack(loader)) {
                    out.put(AnalyzerUtil.trimKey(loader), new JSONObject()
                            .put("risk", "error")
                            .put("explain", "black classloader"));
                }
            }
            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    /** 判断类加载器字符串是否命中黑名单(迁移自原 C++ strstr 判定)。 */
    private static boolean isBlack(String loader) {
        if (loader == null || loader.isEmpty()) return false;
        if (loader.contains(BLACK_LSP)) return true;
        return loader.contains(BLACK_INMEM_DEX) && loader.contains(BLACK_INMEM_DEX_COOKIE);
    }
}