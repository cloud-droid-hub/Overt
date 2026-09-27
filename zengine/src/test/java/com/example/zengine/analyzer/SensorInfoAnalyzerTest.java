package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * SensorInfoAnalyzer 硬编码数据单测(等价于 zSensorInfo.cpp 行为)。
 * riskBits: FIFO_EMPTY=1, WAKEUP_TOO_FEW=2, DELAY_UNIFORM=4, COUNT_LOW=8
 */
public class SensorInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new SensorInfoAnalyzer();

    @Test
    public void highScore_withBits_isError() throws Exception {
        String raw = "{\"sensor_score\":{\"value\":\"70\"},\"sensor_bits\":{\"value\":\"3\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("fifo").getString("risk"));
        assertEquals("error", out.getJSONObject("wakeup_sensor").getString("risk"));
        assertEquals("sensor fifo is empty", out.getJSONObject("fifo").getString("explain"));
        assertTrue(!out.has("delay"));
        assertTrue(!out.has("count"));
    }

    @Test
    public void lowScore_withBits_isWarn() throws Exception {
        String raw = "{\"sensor_score\":{\"value\":\"30\"},\"sensor_bits\":{\"value\":\"12\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("warn", out.getJSONObject("delay").getString("risk"));
        assertEquals("warn", out.getJSONObject("count").getString("risk"));
    }

    @Test
    public void zeroScore_emitsEmpty() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"sensor_score\":{\"value\":\"0\"},\"sensor_bits\":{\"value\":\"0\"}}"));
        assertTrue(out.length() == 0);
    }
}