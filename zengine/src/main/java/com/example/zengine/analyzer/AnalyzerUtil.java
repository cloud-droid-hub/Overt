package com.example.zengine.analyzer;

/**
 * 分析器公共工具。
 */
public final class AnalyzerUtil {

    /** JSON key 最大长度(防止超长 classloader/路径字符串撑爆 UI TextView 内存)。 */
    private static final int MAX_KEY_LENGTH = 512;

    /**
     * 截断 JSON key：超过 {@link #MAX_KEY_LENGTH} 字符则截断并追加省略号。
     * 生僻字(JNI 往返可能损坏的补充平面字符)也会在此被裁剪，BMP 中文无损。
     */
    public static String trimKey(String key) {
        if (key == null) return "";
        if (key.length() <= MAX_KEY_LENGTH) return key;
        return key.substring(0, MAX_KEY_LENGTH) + "…";
    }

    private AnalyzerUtil() {
    }
}