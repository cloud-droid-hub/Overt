package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * 动态链接器信息分析器。
 * <p>
 * 迁移自 zinfo zLinkerInfo.cpp get_linker_info 的内联判断：
 * - 已加载共享库路径含 "lsposed"/"frida" → error
 * - 关键系统库(libc/libart/libinput)CRC 校验失败(crc:N, N!=0) → error
 */
public final class LinkerInfoAnalyzer implements MainApplication.Analyzer {

    /** 黑名单库名(迁移自 C++ 内联判断)。 */
    private static final String[] BLACK_LIB_SUBSTR = {"lsposed", "frida"};

    /** 需要 CRC 完整性检查的关键系统库。 */
    private static final String[] CRC_SO_LIST = {"libc.so", "libart.so", "libinput.so"};

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // 1) 库路径黑名单检测(值 == "so" 表示这是一条库路径)
            Iterator<String> keys = raw.keys();
            while (keys.hasNext()) {
                String path = keys.next();
                JSONObject item = raw.optJSONObject(path);
                if (item == null || !"so".equals(item.optString("value", ""))) continue;

                for (String black : BLACK_LIB_SUBSTR) {
                    if (path.contains(black)) {
                        out.put(AnalyzerUtil.trimKey(path), new JSONObject()
                                .put("risk", "error")
                                .put("explain", "black soname"));
                        break;
                    }
                }
            }

            // 2) 关键系统库 CRC 校验(crc:N, N!=0 表示校验失败)
            for (String so : CRC_SO_LIST) {
                JSONObject item = raw.optJSONObject(so);
                if (item == null) continue;
                String crcVal = item.optString("value", "");
                if (crcVal.startsWith("crc:") && !"crc:0".equals(crcVal)) {
                    out.put(so, new JSONObject()
                            .put("risk", "error")
                            .put("explain", "check_lib_crc error"));
                }
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}