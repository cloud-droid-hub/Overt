package com.example.overt.tee_info;

import android.util.Log;
import org.bouncycastle.asn1.ASN1InputStream;
import org.bouncycastle.asn1.ASN1OctetString;
import org.bouncycastle.asn1.ASN1Sequence;
import org.bouncycastle.asn1.ASN1Primitive;
import org.bouncycastle.asn1.ASN1TaggedObject;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.security.cert.CertificateParsingException;
import java.security.cert.X509Certificate;

public class Attestation {
    private static final String TAG = "TEE_Attestation";
    public static final int KM_SECURITY_LEVEL_SOFTWARE = 0;
    public static final int KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT = 1;
    public static final int KM_SECURITY_LEVEL_STRONG_BOX = 2;

    private static final String ASN1_OID = "1.3.6.1.4.1.11129.2.1.17";

    private final RootOfTrust rootOfTrust;
    private final int attestationSecurityLevel;
    private final AuthorizationList softwareEnforced;
    private final AuthorizationList teeEnforced;

    public Attestation(RootOfTrust rootOfTrust, int attestationSecurityLevel,
                      AuthorizationList softwareEnforced, AuthorizationList teeEnforced) {
        this.rootOfTrust = rootOfTrust;
        this.attestationSecurityLevel = attestationSecurityLevel;
        this.softwareEnforced = softwareEnforced;
        this.teeEnforced = teeEnforced;
    }

    public RootOfTrust getRootOfTrust() {
        return rootOfTrust;
    }

    public int getAttestationSecurityLevel() {
        return attestationSecurityLevel;
    }

    public AuthorizationList getSoftwareEnforced() {
        return softwareEnforced;
    }

    public AuthorizationList getTeeEnforced() {
        return teeEnforced;
    }

    private static void dumpASN1Structure(ASN1Primitive primitive, String prefix) {
        if (primitive instanceof ASN1Sequence) {
            ASN1Sequence sequence = (ASN1Sequence) primitive;
            Log.d(TAG, prefix + "Sequence with " + sequence.size() + " elements:");
            for (int i = 0; i < sequence.size(); i++) {
                Log.d(TAG, prefix + "  Element " + i + ":");
                dumpASN1Structure(sequence.getObjectAt(i).toASN1Primitive(), prefix + "    ");
            }
        } else if (primitive instanceof ASN1TaggedObject) {
            ASN1TaggedObject tagged = (ASN1TaggedObject) primitive;
            Log.d(TAG, prefix + "Tagged object [tag=" + tagged.getTagNo() + "]:");
            dumpASN1Structure(tagged.getObject().toASN1Primitive(), prefix + "  ");
        } else {
            Log.d(TAG, prefix + primitive.getClass().getSimpleName() + ": " + primitive);
        }
    }

    public static Attestation loadFromCertificate(X509Certificate cert) throws CertificateParsingException {
        try {
            // 从证书中提取 RootOfTrust 信息
            byte[] extensionValue = cert.getExtensionValue(ASN1_OID);
            if (extensionValue == null) {
                Log.e(TAG, "No attestation extension found in certificate");
                throw new CertificateParsingException("No attestation extension found");
            }

            // 解析扩展值
            ASN1InputStream asn1InputStream = new ASN1InputStream(new ByteArrayInputStream(extensionValue));
            ASN1OctetString asn1OctetString = (ASN1OctetString) asn1InputStream.readObject();
            ASN1InputStream asn1InputStream2 = new ASN1InputStream(new ByteArrayInputStream(asn1OctetString.getOctets()));
            ASN1Sequence asn1Sequence = (ASN1Sequence) asn1InputStream2.readObject();

            Log.d(TAG, "Parsing attestation extension with " + asn1Sequence.size() + " elements");
            for (int i = 0; i < asn1Sequence.size(); i++) {
                Log.d(TAG, "Element " + i + " type: " + asn1Sequence.getObjectAt(i).getClass().getName());
            }

            // 提取安全级别
            int securityLevel;
            try {
                securityLevel = Asn1Utils.getIntegerFromAsn1(asn1Sequence.getObjectAt(1));
                Log.d(TAG, "Security level: " + securityLevel);
            } catch (Exception e) {
                Log.e(TAG, "Failed to parse security level", e);
                throw new CertificateParsingException("Failed to parse security level: " + e.getMessage());
            }

            // 解析授权列表
            AuthorizationList softwareEnforced;
            AuthorizationList teeEnforced;
            try {
                softwareEnforced = new AuthorizationList(asn1Sequence.getObjectAt(6));
                teeEnforced = new AuthorizationList(asn1Sequence.getObjectAt(7));
                Log.d(TAG, "Successfully parsed authorization lists");
            } catch (Exception e) {
                Log.e(TAG, "Failed to parse authorization lists", e);
                throw new CertificateParsingException("Failed to parse authorization lists: " + e.getMessage());
            }

            // 从授权列表中获取 RootOfTrust
            RootOfTrust rootOfTrust = teeEnforced.getRootOfTrust();
            if (rootOfTrust == null) {
                rootOfTrust = softwareEnforced.getRootOfTrust();
            }
            if (rootOfTrust == null) {
                throw new CertificateParsingException("No RootOfTrust found in authorization lists");
            }

            return new Attestation(rootOfTrust, securityLevel, softwareEnforced, teeEnforced);
        } catch (IOException e) {
            Log.e(TAG, "Failed to parse attestation extension", e);
            throw new CertificateParsingException("Failed to parse attestation extension: " + e.getMessage());
        }
    }

    public static String securityLevelToString(int attestationSecurityLevel) {
        switch (attestationSecurityLevel) {
            case KM_SECURITY_LEVEL_SOFTWARE:
                return "Software";
            case KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT:
                return "TEE";
            case KM_SECURITY_LEVEL_STRONG_BOX:
                return "StrongBox";
            default:
                return "Unknown (" + attestationSecurityLevel + ")";
        }
    }
}
