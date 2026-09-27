package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * TeeInfoAnalyzer 硬编码数据单测(纯链驱动语义)。
 * <p>
 * 采集端只上报 tee_cert_chain(base64 完整链),analyzer 验签+解析+判定全在 Java。
 * 单测无法生成真实 Keymaster 链：
 * - 无链 → tee_statue error(fail-closed)
 * - 伪造链 → tee_verify error(fail-closed)
 * - 真实"验签通过后的 device_locked/verified_boot_state 判定"只能在真机覆盖。
 */
public class TeeInfoAnalyzerTest {

    private final MainApplication.Analyzer analyzer = new TeeInfoAnalyzer();

    @Test
    public void noChain_isTeeStatueError() throws Exception {
        JSONObject out = new JSONObject(analyzer.analyze("{\"nothing\":\"x\"}"));
        assertEquals("error", out.getJSONObject("tee_statue").getString("risk"));
    }

    @Test
    public void emptyChain_isTeeStatueError() throws Exception {
        String raw = "{\"tee_cert_chain\":{\"value\":\"\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("tee_statue").getString("risk"));
    }

    @Test
    public void garbageChain_isVerifyError() throws Exception {
        // 无效 base64/非法 DER → 验签非 ok → tee_verify error
        String raw = "{\"tee_cert_chain\":{\"value\":\"not-a-real-chain\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("tee_verify").getString("risk"));
    }

    @Test
    public void forgedChain_isVerifyError() throws Exception {
        // 伪造"证书"(无法自洽/根不可信)→ tee_verify error
        String fakeCert =
                "MIIC8zCCAdmgAwIBAgIJAKGZxZ+Xn3ePMA0GCSqGSIb3DQEBCwUAMBIxEDAOBgNV" +
                "BAMMB2Zha2Vyb290MB4XDTI2MDEwMTAwMDAwMFoXDTM2MDEwMTAwMDAwMFowEjEQ" +
                "MA4GA1UEAwwHZmFrZXJvb3QwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB" +
                "AQDAkIs9tk/fYlRrXkAKQBhLQ9aYQo9PC1m0nfTHdJeVl2d5iQmxM2VVHGSTvwhL" +
                "oLnGi8i3P+d1dQlBYmZwvDGqLYVfVY65nUpS0PKC1vkKqwDcPctrpOFGBijjsSHB" +
                "bYQNml3Q4L2OsMIhBNOjOLqq2DQWVkFnhSVBDmSa2WM1TSG0OGYUOaK8crr61lNq" +
                "P1Q7YRqZTGpyZLn+wHSSQQQ/T6l8MmxF2Om1Bl2nT1bV7eCz1YpmsC3vGCPXrh/2" +
                "8mvLfF8rz/6fXcY3u2cFQ2lAN0xZ5LQ8Y8o7Zihg9OIPSof+O9w3B8s4YhXjNV4m" +
                "6ny+kISY8NsQ18VTplRBD+fQw35NAgMBAAGjUzBRMB0GA1UdDgQWBBTgRg2C7Z0K" +
                "dQx4V8j9Gm1Y7/6yXDAfBgNVHSMEGDAWgBTgRg2C7Z0KdQx4V8j9Gm1Y7/6yXDAe" +
                "BgNVHREBAf8wDTALBglghkgBhv1sAAEwDQYJKoZIhvcNAQELBQADggEBADjK8fM5" +
                "y6TWlDpHO1XbY6nz7JqZlfqL9yQysK1bU5lbOtxwaYdUV+pzR2l4JpFGNqY6nz8v" +
                "Z1y6mWQGK0D3rV4KmZ8dJNfWJQKOXT/We9d4zi6AqxM2ZU7mPhIqSLp9KZ0IVTdI" +
                "kjTUwM6w9YgnP7UuW9e7YXnXHqql6ZgGwWq7L45Z1v1rG8n6mF7Y9ozJqxjmbZlX" +
                "T8S5R1g3pHM8E0qKpzsU1SX0pvL3q9n0p5tR4KV8XMxY0IuPJZGDdK6cKLlTdRUf" +
                "6Yf7iCJLOgGN0R2GdO8gT4Kv7aLZwZWSyC2M+Q1ZKJ4kW0HdBjjJvswKuk=";
        String raw = "{\"tee_cert_chain\":{\"value\":\"" + fakeCert + "\"}}";
        JSONObject out = new JSONObject(analyzer.analyze(raw));
        assertEquals("error", out.getJSONObject("tee_verify").getString("risk"));
    }

    @Test
    public void malformedJson_returnsFailClosedEmpty() {
        assertEquals("{}", analyzer.analyze("not json"));
    }
}