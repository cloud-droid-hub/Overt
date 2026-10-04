package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * SensorInfoAnalyzer 硬编码数据单测(输入为 sensor:N 原始字段,统计+判定全在 analyzer)。
 * 字段: name,type,minDelay,maxDelay,fifoMax,fifoReserved,isWakeUp
 * 判定: COUNT_LOW(sensor_count<20)+30, FIFO_EMPTY(全为0)+30,
 *        WAKEUP_TOO_FEW(<2)+20, 组合+20, 封顶100。 >60 error / >0 warn。
 */
public class SensorInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SensorInfoAnalyzer();

    /** 构造 N 个 sensor 的 raw JSON;fifoZero 个 fifo 为0, wakeup 个 isWakeUp=1。 */
    private String buildRaw(int total, int fifoZero, int wakeup) {
        StringBuilder sb = new StringBuilder("{");
        for (int i = 0; i < total; i++) {
            if (i > 0) sb.append(",");
            boolean fifoEmpty = i < fifoZero;
            boolean isWake = i < wakeup;
            String fifoMax = fifoEmpty ? "0" : "10000";
            String fifoRes = fifoEmpty ? "0" : "5000";
            String wake = isWake ? "1" : "0";
            sb.append("\"sensor:").append(i).append("\":{\"value\":\"s")
              .append(i).append(",1,200000,10000000,")
              .append(fifoMax).append(",").append(fifoRes).append(",")
              .append(wake).append("\"}");
        }
        sb.append("}");
        return sb.toString();
    }

    @Test
    public void countLow_only_isWarn() throws Exception {
        // 15 个传感器(<20 → COUNT_LOW +30),wakeup 足够(3),fifo 非空
        String raw = buildRaw(15, 0, 3);
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("count").getString("risk"));
        assertEquals("sensor count is too low", out.getJSONObject("count").getString("explain"));
        assertTrue(!out.has("fifo"));
        assertTrue(!out.has("wakeup_sensor"));
    }

    @Test
    public void fifoEmpty_only_isWarn() throws Exception {
        // 25 个传感器全部 FIFO=0(FIFO_EMPTY +30),wakeup 足够(3)
        String raw = buildRaw(25, 25, 3);
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("fifo").getString("risk"));
        assertEquals("sensor fifo is empty", out.getJSONObject("fifo").getString("explain"));
    }

    @Test
    public void wakeupTooFew_only_isWarn() throws Exception {
        // wakeup=1(<2 → +20)
        String raw = buildRaw(30, 0, 1);
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("wakeup_sensor").getString("risk"));
    }

    @Test
    public void combo_highScore_isError() throws Exception {
        // 10 传感器 + FIFO 全空 + wakeup 1 → COUNT_LOW(30)+FIFO_EMPTY(30)+WAKEUP(20)+COMBO(20)=100 → error
        String raw = buildRaw(10, 10, 1);
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("count").getString("risk"));
        assertEquals("error", out.getJSONObject("fifo").getString("risk"));
        assertEquals("error", out.getJSONObject("wakeup_sensor").getString("risk"));
    }

    @Test
    public void healthySensors_emitsEmpty() throws Exception {
        // 30 传感器,fifo 非空,wakeup 4 → 无风险
        String raw = buildRaw(30, 0, 4);
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("健康传感器应无输出: " + out, out.length() == 0);
    }

    @Test
    public void missingFields_emitsEmpty() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{}"));
        assertTrue("缺字段默认安全: " + out, out.length() == 0);
    }

    /** 模拟器传感器 name 含 goldfish → 独立 emulator_sensor error。 */
    @Test
    public void goldfishSensor_isEmulatorError() throws Exception {
        // 用真实模拟器实测数据: Goldfish 3-axis Accelerometer,type=1,minDelay=10000
        String raw = "{" +
                "\"sensor:0\":{\"value\":\"Goldfish 3-axis Accelerometer,1,10000,0,0,0,0\"}," +
                "\"sensor:1\":{\"value\":\"Goldfish Proximity sensor,8,0,0,0,0,1\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("emulator_sensor").getString("risk"));
        assertEquals("emulator sensor (goldfish)", out.getJSONObject("emulator_sensor").getString("explain"));
    }

    /** 健康真实传感器(非 goldfish)→ 无 emulator_sensor。 */
    @Test
    public void realSensor_noEmulatorFlag() throws Exception {
        String raw = "{" +
                "\"sensor:0\":{\"value\":\"LSM6DSO Accelerometer,1,10000,1000000,100,50,0\"}" +
                "}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertTrue("真机传感器不应报 emulator: " + out, !out.has("emulator_sensor"));
    }
}