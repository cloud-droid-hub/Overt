package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.Iterator;

/**
 * 传感器信息分析器(查杀分离:判定全在 Java,采集端只报原始字段)。
 * <p>
 * 消费 zinfo zSensorInfo.cpp 上报的每个传感器原始字段(sensor:N -> "name,type,minDelay,maxDelay,fifoMax,fifoReserved,isWakeUp")。
 * 在此自己统计 + 判定(迁移自 zcore zSensorManager.cpp 的判定逻辑)：
 * - COUNT_LOW:    传感器总数 < 20                    → +30
 * - FIFO_EMPTY:   所有传感器 fifoMax==0 && fifoReserved==0 → +30
 * - WAKEUP_TOO_FEW: wake-up 传感器数 < 2              → +20
 * - 组合加分:     WAKEUP_TOO_FEW && FIFO_EMPTY        → +20(封顶 100)
 * 输出(与以前 C++ 判定保持一致):
 * - score > 60 → error 级别;score > 0 → warn 级别(score<=0 无输出)
 * - 发射项: fifo / wakeup_sensor / count
 */
public final class SensorInfoAnalyzer implements MainApplication.Analyzer {

    private static final int SCORE_ERROR_THRESHOLD = 60;

    // 判定常量(迁移自 zSensorManager.cpp calculateRiskScore / detectStructureAnomalies)
    private static final int SENSOR_COUNT_LOW_THRESHOLD = 20;
    private static final int WAKEUP_TOO_FEW_THRESHOLD = 2;
    private static final int SCORE_FIFO_EMPTY = 30;
    private static final int SCORE_COUNT_LOW = 30;
    private static final int SCORE_WAKEUP_TOO_FEW = 20;
    private static final int SCORE_COMBO = 20;  // WAKEUP_TOO_FEW && FIFO_EMPTY
    private static final int SCORE_MAX = 100;

    // 原始字段索引(与 zSensorInfo.cpp 上报顺序一致)
    private static final int IDX_NAME = 0;
    private static final int IDX_TYPE = 1;
    private static final int IDX_FIFO_MAX = 4;
    private static final int IDX_FIFO_RESERVED = 5;
    private static final int IDX_IS_WAKEUP = 6;

    /** 模拟器传感器 name 特征(goldfish 是 qemu 模拟器的传感器实现,真机绝无)。命中 → error。 */
    private static final String EMULATOR_SENSOR_MARK = "goldfish";

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            // 遍历所有 sensor:N 原始字段,统计
            int sensorCount = 0;
            int fifoZeroCount = 0;
            int wakeupCount = 0;
            boolean emulatorSensor = false;

            Iterator<String> keys = raw.keys();
            while (keys.hasNext()) {
                String key = keys.next();
                if (!key.startsWith("sensor:")) continue;
                JSONObject item = raw.optJSONObject(key);
                if (item == null) continue;
                String rawValue = item.optString("value", "");
                String[] fields = rawValue.split(",");
                if (fields.length < 7) continue;

                sensorCount++;
                int fifoMax = parseIntSafe(fields[IDX_FIFO_MAX]);
                int fifoReserved = parseIntSafe(fields[IDX_FIFO_RESERVED]);
                if (fifoMax == 0 && fifoReserved == 0) fifoZeroCount++;
                if ("1".equals(fields[IDX_IS_WAKEUP])) wakeupCount++;
                // 模拟器传感器检测:name 含 goldfish(qemu 模拟器传感器实现,真机绝无)
                if (fields[IDX_NAME].toLowerCase(java.util.Locale.ROOT).contains(EMULATOR_SENSOR_MARK)) {
                    emulatorSensor = true;
                }
            }

            // 数据缺失(无任何 sensor:N)→ 无风险判定(fail-open 而非误报)
            if (sensorCount == 0 && fifoZeroCount == 0 && wakeupCount == 0) {
                return out.toString();
            }

            // 计算风险位
            boolean fifoEmpty = sensorCount > 0 && fifoZeroCount >= sensorCount;
            boolean wakeupTooFew = wakeupCount < WAKEUP_TOO_FEW_THRESHOLD;
            boolean countLow = sensorCount < SENSOR_COUNT_LOW_THRESHOLD;
            // DELAY_UNIFORM 在 C++ 端从未实现,此处不计算(迁移保持原行为)

            // 计算评分(迁移自 calculateRiskScore)
            int score = 0;
            if (fifoEmpty) score += SCORE_FIFO_EMPTY;
            if (countLow) score += SCORE_COUNT_LOW;
            if (wakeupTooFew) score += SCORE_WAKEUP_TOO_FEW;
            if (wakeupTooFew && fifoEmpty) score += SCORE_COMBO;
            if (score > SCORE_MAX) score = SCORE_MAX;

            if (score <= 0) {
                return out.toString(); // 无风险,不输出
            }

            String level = score > SCORE_ERROR_THRESHOLD ? "error" : "warn";

            if (fifoEmpty) {
                out.put("fifo", new JSONObject()
                        .put("risk", level)
                        .put("explain", "sensor fifo is empty"));
            }
            if (wakeupTooFew) {
                out.put("wakeup_sensor", new JSONObject()
                        .put("risk", level)
                        .put("explain", "wakeup sensors too few"));
            }
            if (countLow) {
                out.put("count", new JSONObject()
                        .put("risk", level)
                        .put("explain", "sensor count is too low"));
            }

            // 模拟器传感器检测:独立发射(不参与评分),命中即 error
            if (emulatorSensor) {
                out.put("emulator_sensor", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "emulator sensor (goldfish)"));
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
}