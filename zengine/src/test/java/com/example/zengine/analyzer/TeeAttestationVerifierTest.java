package com.example.zengine.analyzer;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * TeeAttestationVerifier 单测。
 * <p>
 * 注意：完整证书链的端到端验签(真实 Keymaster 链)只能在实机验证；
 * 这里覆盖错误路径(fail-closed)与空链/乱码解析。
 */
public class TeeAttestationVerifierTest {

    @Test
    public void emptyChain_isError() {
        assertEquals(TeeAttestationVerifier.ERR_EMPTY_CHAIN, TeeAttestationVerifier.verify(""));
        assertEquals(TeeAttestationVerifier.ERR_EMPTY_CHAIN, TeeAttestationVerifier.verify(null));
    }

    @Test
    public void garbageBase64_isParseError() {
        assertEquals(TeeAttestationVerifier.ERR_PARSE, TeeAttestationVerifier.verify("not-a-real-chain"));
    }

    @Test
    public void singleGarbageCert_isNotOk() {
        // 只有一个无效 DER 的"链"(解析后可能为空/失败),不应返回 ok
        String fakeB64 = "QUJDRA=="; // "ABCD"
        String result = TeeAttestationVerifier.verify(fakeB64);
        // 解析失败或自签失败,都绝不能是 ok
        assertEquals(false, TeeAttestationVerifier.OK.equals(result));
    }

    @Test
    public void forgedChain_isNotOk() {
        // 伪造的"证书"无法自洽(无真实 Keymaster 签名),应 fail-closed
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
        String result = TeeAttestationVerifier.verify(fakeCert);
        // 伪造证书(非自签/公钥不在可信根)→ 绝不为 ok
        assertEquals(false, TeeAttestationVerifier.OK.equals(result));
    }
}