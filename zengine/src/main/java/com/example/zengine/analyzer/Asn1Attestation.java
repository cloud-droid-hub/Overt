/*
 * Copyright (C) 2016/2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.zengine.analyzer;

import org.bouncycastle.asn1.ASN1Encodable;
import org.bouncycastle.asn1.ASN1Sequence;
import org.bouncycastle.asn1.ASN1TaggedObject;

import java.security.cert.CertificateParsingException;
import java.security.cert.X509Certificate;
import java.util.Enumeration;

/**
 * TEE Attestation 扩展解析器——抄自 KeyAttestation 项目 Asn1Attestation.java / AuthorizationList.java
 * (AOSP Apache-2.0),去掉 Guava/android 依赖。
 * <p>
 * 从叶子证书的 TEE Attestation 扩展(OID 1.3.6.1.4.1.11129.2.1.17)中提取：
 * - attestationSecurityLevel
 * - attestationChallenge
 * - softwareEnforced / teeEnforced 的 RootOfTrust(deviceLocked / verifiedBootState / verifiedBootKey)
 * - osVersion / osPatchLevel / bootPatchLevel
 */
public final class Asn1Attestation {

    /** TEE 认证扩展 OID。 */
    public static final String ASN1_OID = "1.3.6.1.4.1.11129.2.1.17";

    /** Keymaster 安全级别。 */
    public static final int KM_SECURITY_LEVEL_SOFTWARE = 0;
    public static final int KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT = 1;
    public static final int KM_SECURITY_LEVEL_STRONG_BOX = 2;

    // Keymaster AuthorizationList 标签 —— 实机验证(模拟器 TEE)ASN.1 里的 tag 就是【裸值】:
//   RootOfTrust=704(0x2C0) / OS_VERSION=705(0x2C1) / OS_PATCHLEVEL=706(0x2C2) / BOOT_PATCHLEVEL=719(0x2CF)
//   不带类型位;与 zTeeCert.h 原始定义一致。比对时必须用裸值。
    private static final int KM_TAG_ROOT_OF_TRUST = 704;
    private static final int KM_TAG_OS_VERSION = 705;
    private static final int KM_TAG_OS_PATCHLEVEL = 706;
    private static final int KM_TAG_BOOT_PATCHLEVEL = 719;

    // attestation 记录内索引
    private static final int ATTESTATION_SECURITY_LEVEL_INDEX = 1;
    private static final int ATTESTATION_CHALLENGE_INDEX = 4;
    private static final int SW_ENFORCED_INDEX = 6;
    private static final int TEE_ENFORCED_INDEX = 7;

    /** 与 KeyAttestation 一致：tag 号低 28 位是裸值,高位是类型位。 */
    private static final int KEYMASTER_TAG_TYPE_MASK = 0x0FFFFFFF;

    private final int attestationSecurityLevel;
    private final byte[] attestationChallenge;
    private final RootOfTrust softwareRootOfTrust;
    private final RootOfTrust teeRootOfTrust;
    private final long swOsVersion, swOsPatchLevel, swBootPatchLevel;

    /**
     * 从 X509 叶子证书解析 TEE Attestation 扩展。
     */
    public Asn1Attestation(X509Certificate x509Cert) throws CertificateParsingException {
        byte[] extBytes = x509Cert.getExtensionValue(ASN1_OID);
        if (extBytes == null || extBytes.length == 0) {
            throw new CertificateParsingException("Did not find extension with OID " + ASN1_OID);
        }
        ASN1Sequence seq = Asn1Utils.getAsn1SequenceFromBytes(extBytes);

        attestationSecurityLevel =
                Asn1Utils.getIntegerFromAsn1(seq.getObjectAt(ATTESTATION_SECURITY_LEVEL_INDEX));
        attestationChallenge =
                Asn1Utils.getByteArrayFromAsn1(seq.getObjectAt(ATTESTATION_CHALLENGE_INDEX));

        softwareRootOfTrust = parseAuthorizationList(seq.getObjectAt(SW_ENFORCED_INDEX));
        teeRootOfTrust = parseAuthorizationList(seq.getObjectAt(TEE_ENFORCED_INDEX));

        // 从 softwareEnforced 提取 os 版本(可选字段,失败返回 0)
        swOsVersion = parseOsVersion(seq.getObjectAt(SW_ENFORCED_INDEX), KM_TAG_OS_VERSION);
        swOsPatchLevel = parseOsVersion(seq.getObjectAt(SW_ENFORCED_INDEX), KM_TAG_OS_PATCHLEVEL);
        swBootPatchLevel = parseOsVersion(seq.getObjectAt(SW_ENFORCED_INDEX), KM_TAG_BOOT_PATCHLEVEL);
    }

    public int getAttestationSecurityLevel() { return attestationSecurityLevel; }
    public byte[] getAttestationChallenge() { return attestationChallenge; }

    /** RootOfTrust：优先 TEE Enforced，其次 Software Enforced(与原 C++ 逻辑一致)。 */
    public RootOfTrust getRootOfTrust() {
        if (teeRootOfTrust != null) return teeRootOfTrust;
        return softwareRootOfTrust;
    }

    public long getSwOsVersion() { return swOsVersion; }
    public long getSwOsPatchLevel() { return swOsPatchLevel; }
    public long getSwBootPatchLevel() { return swBootPatchLevel; }

    /**
     * 解析 AuthorizationList，返回其中的 RootOfTrust(可能为 null)。
     * 照抄 KeyAttestation：列表的每个元素是 ASN1TaggedObject，tag 号来自 getTagNo()，
     * 值来自 getBaseObject()。(绝不能用 SEQUENCE 内读 integer 当 tag —— 那是错的。)
     */
    private static RootOfTrust parseAuthorizationList(ASN1Encodable list)
            throws CertificateParsingException {
        if (!(list instanceof ASN1Sequence)) {
            return null;
        }
        ASN1Sequence authList = (ASN1Sequence) list;
        Enumeration<?> objects = authList.getObjects();
        while (objects.hasMoreElements()) {
            ASN1Encodable obj = (ASN1Encodable) objects.nextElement();
            if (!(obj instanceof ASN1TaggedObject)) continue;
            ASN1TaggedObject tagged = (ASN1TaggedObject) obj;
            int tag = tagged.getTagNo();
            if (tag == (KM_TAG_ROOT_OF_TRUST & KEYMASTER_TAG_TYPE_MASK)) {
                return new RootOfTrust(tagged.getBaseObject().toASN1Primitive());
            }
        }
        return null;
    }

    /** 解析 AuthorizationList 中指定 tag 的 LONG 值(失败返回 -1)。 */
    private static long parseOsVersion(ASN1Encodable list, int wantedTag) {
        try {
            if (!(list instanceof ASN1Sequence)) return -1;
            ASN1Sequence authList = (ASN1Sequence) list;
            Enumeration<?> objects = authList.getObjects();
            while (objects.hasMoreElements()) {
                ASN1Encodable obj = (ASN1Encodable) objects.nextElement();
                if (!(obj instanceof ASN1TaggedObject)) continue;
                ASN1TaggedObject tagged = (ASN1TaggedObject) obj;
                int tag = tagged.getTagNo();
                if (tag == (wantedTag & KEYMASTER_TAG_TYPE_MASK)) {
                    return Asn1Utils.getLongFromAsn1(tagged.getBaseObject().toASN1Primitive());
                }
            }
        } catch (CertificateParsingException ignored) {
            // 可选字段解析失败，返回 -1
        }
        return -1;
    }
}