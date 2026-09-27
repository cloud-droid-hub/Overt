package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * 传感器信息分析器。
 * <p>
 * 迁移自 zinfo zSensorInfo.cpp get_sensor_info 的内联判断：
 * - score > 60 → error 级别;score > 0 → warn 级别(score<=0 无输出)
 * - riskBits 位标志：FIFO_EMPTY/WAKEUP_TOO_FEW/DELAY_UNIFORM/COUNT_LOW → 对应项
 */
public final class SensorInfoAnalyzer implements MainApplication.Analyzer {

    private static final int SCORE_ERROR_THRESHOLD = 60;

    // 与 zSensorManager.h 的 SENSOR_* 位标志对齐
    private static final int SENSOR_FIFO_EMPTY     = 1;
    private static final int SENSOR_WAKEUP_TOO_FEW = 1 << 1;
    private static final int SENSOR_DELAY_UNIFORM  = 1 << 2;
    private static final int SENSOR_COUNT_LOW      = 1 << 3;

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            int score = parseIntSafe(valueOf(raw, "sensor_score"));
            int riskBits = parseIntSafe(valueOf(raw, "sensor_bits"));

            if (score <= 0) {
                return out.toString(); // 无风险,不输出
            }

            String level = score > SCORE_ERROR_THRESHOLD ? "error" : "warn";
            Map<String, String> bitExplains = new LinkedHashMap<>();
            bitExplains.put("fifo", "sensor fifo is empty");
            bitExplains.put("wakeup_sensor", "wakeup sensors too few");
            bitExplains.put("delay", "sensor uniform delays");
            bitExplains.put("count", "sensor count is too low");

            int[] bits = {SENSOR_FIFO_EMPTY, SENSOR_WAKEUP_TOO_FEW, SENSOR_DELAY_UNIFORM, SENSOR_COUNT_LOW};
            String[] keys = {"fifo", "wakeup_sensor", "delay", "count"};
            for (int i = 0; i < bits.length; i++) {
                if ((riskBits & bits[i]) != 0) {
                    out.put(keys[i], new JSONObject()
                            .put("risk", level)
                            .put("explain", bitExplains.get(keys[i])));
                }
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    private static int parseIntSafe(String s) {
        try {
            return Integer.parseInt(s.trim());
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    private static String valueOf(JSONObject raw, String key) {
        JSONObject item = raw.optJSONObject(key);
        return (item == null) ? "" : item.optString("value", "");
    }
}