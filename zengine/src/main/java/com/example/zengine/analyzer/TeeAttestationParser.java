package com.example.zengine.analyzer;

import java.io.ByteArrayInputStream;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;

/**
 * TEE Attestation 证书链解析门面(zengine)。
 * <p>
 * 接收 C++ 采集端上报的 base64 完整证书链(tee_cert_chain,逗号分隔),提供：
 * - {@link #verify(String)}:验签(链自洽 + 根公钥 ∈ 可信集合)
 * - {@link #parse(String)}:解析叶子证书的 TEE Attestation 扩展,提取字段
 * <p>
 * 查杀分离:验签/解析/判定全在 Java。
 */
public final class TeeAttestationParser {

    /**
     * 验签完整链。返回 {@link TeeAttestationVerifier} 的结果字符串(OK / ERR_*)。
     */
    public static String verify(String chainB64) {
        return TeeAttestationVerifier.verify(chainB64);
    }

    /**
     * 解析叶子证书的 TEE Attestation 扩展。
     *
     * @param chainB64 逗号分隔的 base64 DER 证书链(叶子在前)
     * @return Asn1Attestation(叶子为 null / 解析失败返回 null)
     */
    public static Asn1Attestation parse(String chainB64) {
        try {
            List<X509Certificate> chain = decodeChain(chainB64);
            if (chain.isEmpty()) return null;
            X509Certificate leaf = chain.get(0);
            return new Asn1Attestation(leaf);
        } catch (Exception e) {
            return null;
        }
    }

    /** base64 链 → X509Certificate 列表。 */
    private static List<X509Certificate> decodeChain(String chainB64) throws Exception {
        CertificateFactory cf = CertificateFactory.getInstance("X.509");
        List<X509Certificate> result = new ArrayList<>();
        if (chainB64 == null || chainB64.isEmpty()) return result;
        String[] parts = chainB64.split(",");
        for (String part : parts) {
            byte[] der = Base64.getDecoder().decode(part.trim());
            result.add((X509Certificate) cf.generateCertificate(new ByteArrayInputStream(der)));
        }
        return result;
    }

    private TeeAttestationParser() {
    }
}