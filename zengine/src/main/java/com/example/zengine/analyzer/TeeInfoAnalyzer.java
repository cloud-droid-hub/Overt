package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

/**
 * TEE 信息分析器(查杀分离:验签/解析/判定全在 Java)。
 * <p>
 * 采集端(zTeeInfo.cpp)只上报 tee_cert_chain(base64 完整证书链)；
 * 本分析器用 TeeAttestationParser:
 * 1. 验签(链自洽 + 根公钥 ∈ 可信集合) → 非 ok → tee_verify error
 * 2. 解析叶子证书的 TEE Attestation 扩展 → 提取 device_locked / verified_boot_state
 *    / attestation_security_level 等字段
 * 3. 判定:
 *    - 链为空/解析失败 → tee_statue error
 *    - device_locked != true → error
 *    - verified_boot_state != VERIFIED(0) → error
 */
public final class TeeInfoAnalyzer implements MainApplication.Analyzer {

    /** VERIFIED_BOOT_STATE_VERIFIED = 0。 */
    private static final String VERIFIED_BOOT_STATE = "0";

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();

            String chainB64 = valueOf(raw, "tee_cert_chain");
            if (chainB64.isEmpty()) {
                out.put("tee_statue", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "tee_statue is damage"));
                return out.toString();
            }

            // 1) 验签
            String verifyResult = TeeAttestationParser.verify(chainB64);
            if (!TeeAttestationVerifier.OK.equals(verifyResult)) {
                out.put("tee_verify", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "tee attestation verify failed: " + verifyResult));
            }

            // 2) 解析叶子证书的 TEE Attestation 扩展
            Asn1Attestation attestation = TeeAttestationParser.parse(chainB64);
            if (attestation == null) {
                out.put("tee_statue", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "tee_statue is damage"));
                return out.toString();
            }

            RootOfTrust rootOfTrust = attestation.getRootOfTrust();
            if (rootOfTrust == null) {
                out.put("tee_statue", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "root_of_trust_missing"));
                return out.toString();
            }

            // 一致性校验:证书内 challenge 必须等于采集端上报的 tee_challenge(本次随机值)。
            // 防重放/防简单篡改:若攻击者替换证书而未同步改上报的 challenge,此处拦截。
            String reportedChallengeB64 = valueOf(raw, "tee_challenge");
            byte[] certChallenge = attestation.getAttestationChallenge();
            if (reportedChallengeB64.isEmpty() || certChallenge == null ||
                    !java.util.Arrays.equals(
                            java.util.Base64.getDecoder().decode(reportedChallengeB64),
                            certChallenge)) {
                out.put("tee_verify", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "tee attestation challenge mismatch"));
            }

            // 3) 判定
            if (!rootOfTrust.isDeviceLocked()) {
                out.put("device_locked", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "device_locked is unsafe"));
            }
            if (!VERIFIED_BOOT_STATE.equals(String.valueOf(rootOfTrust.getVerifiedBootState()))) {
                out.put("verified_boot_state", new JSONObject()
                        .put("risk", "error")
                        .put("explain", "verified_boot_state is unsafe"));
            }

            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }

    private static String valueOf(JSONObject raw, String key) {
        JSONObject item = raw.optJSONObject(key);
        return (item == null) ? "" : item.optString("value", "");
    }
}