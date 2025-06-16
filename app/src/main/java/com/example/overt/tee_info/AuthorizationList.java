package com.example.overt.tee_info;

import android.util.Log;
import org.bouncycastle.asn1.ASN1Encodable;
import org.bouncycastle.asn1.ASN1Sequence;
import org.bouncycastle.asn1.ASN1TaggedObject;
import org.bouncycastle.asn1.ASN1Primitive;

import java.security.cert.CertificateParsingException;

public class AuthorizationList {
    private static final String TAG = "TEE_AuthorizationList";

    // Keymaster tag classes
    private static final int KM_ENUM = 1 << 28;
    private static final int KM_ENUM_REP = 2 << 28;
    private static final int KM_UINT = 3 << 28;
    private static final int KM_UINT_REP = 4 << 28;
    private static final int KM_ULONG = 5 << 28;
    private static final int KM_DATE = 6 << 28;
    private static final int KM_BOOL = 7 << 28;
    private static final int KM_BYTES = 9 << 28;
    private static final int KM_ULONG_REP = 10 << 28;

    // Tag class removal mask
    private static final int KEYMASTER_TAG_TYPE_MASK = 0x0FFFFFFF;

    // Keymaster tags
    private static final int KM_TAG_ROOT_OF_TRUST = KM_BYTES | 704;

    private RootOfTrust rootOfTrust;

    public AuthorizationList(ASN1Encodable asn1Encodable) throws CertificateParsingException {
        if (!(asn1Encodable instanceof ASN1Sequence)) {
            throw new CertificateParsingException("Expected sequence for authorization list, found "
                    + asn1Encodable.getClass().getName());
        }

        ASN1Sequence sequence = (ASN1Sequence) asn1Encodable;
        Log.d(TAG, "Parsing authorization list with " + sequence.size() + " elements");

        for (int i = 0; i < sequence.size(); i++) {
            ASN1Encodable element = sequence.getObjectAt(i);
            if (!(element instanceof ASN1TaggedObject)) {
                throw new CertificateParsingException(
                        "Expected tagged object, found " + element.getClass().getName());
            }

            ASN1TaggedObject taggedObject = (ASN1TaggedObject) element;
            int tag = taggedObject.getTagNo();
            ASN1Primitive value = taggedObject.getBaseObject().toASN1Primitive();
            Log.d(TAG, "Parsing tag: [" + tag + "], value: [" + value + "]");

            switch (tag & KEYMASTER_TAG_TYPE_MASK) {
                case KM_TAG_ROOT_OF_TRUST & KEYMASTER_TAG_TYPE_MASK:
                    try {
                        rootOfTrust = new RootOfTrust(value);
                        Log.d(TAG, "Successfully parsed RootOfTrust from authorization list");
                    } catch (Exception e) {
                        Log.e(TAG, "Failed to parse RootOfTrust", e);
                        throw new CertificateParsingException("Failed to parse RootOfTrust: " + e.getMessage());
                    }
                    break;
                default:
                    // Ignore other tags
                    break;
            }
        }
    }

    public RootOfTrust getRootOfTrust() {
        return rootOfTrust;
    }
} 