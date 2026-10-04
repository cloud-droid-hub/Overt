package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * 端口信息分析器。
 * <p>
 * 迁移自 zinfo zPortInfo.cpp get_port_info 的内联判断：
 * 端口被占用(value=="1") → 对应工具名 error。
 */
public final class PortInfoAnalyzer implements MainApplication.Analyzer {

    /** 端口号 → 工具名(迁移自 C++ tcp_info 表)。 */
    private static final String[] SUSPICIOUS_PORTS = {"27042", "27043", "27047", "23946"};

    private static final String BLACK_TOOL_NAME(int port) {
        // 与 C++ 语义一致：27042/27043/27047 → frida；23946 → ida
        return (port == 23946) ? "ida" : "frida";
    }

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            for (String portStr : SUSPICIOUS_PORTS) {
                JSONObject item = raw.optJSONObject(portStr);
                if (item != null && "1".equals(item.optString("value", ""))) {
                    int port = Integer.parseInt(portStr);
                    String tool = BLACK_TOOL_NAME(port);
                    out.put(tool, new JSONObject()
                            .put("risk", "error")
                            .put("explain", "black port is in use " + tool));
                }
            }
            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}