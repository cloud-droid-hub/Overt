package tee_info;

import android.util.Log;
import org.bouncycastle.asn1.ASN1Encodable;
import org.bouncycastle.asn1.ASN1Sequence;

import java.security.cert.CertificateParsingException;

public class RootOfTrust {
    private static final String TAG = "TEE_RootOfTrust";
    private static final int VERIFIED_BOOT_KEY_INDEX = 0;
    private static final int DEVICE_LOCKED_INDEX = 1;
    private static final int VERIFIED_BOOT_STATE_INDEX = 2;
    private static final int VERIFIED_BOOT_HASH_INDEX = 3;

    public static final int KM_VERIFIED_BOOT_VERIFIED = 0;
    public static final int KM_VERIFIED_BOOT_SELF_SIGNED = 1;
    public static final int KM_VERIFIED_BOOT_UNVERIFIED = 2;
    public static final int KM_VERIFIED_BOOT_FAILED = 3;

    private final byte[] verifiedBootKey;
    private final boolean deviceLocked;
    private final int verifiedBootState;
    private final byte[] verifiedBootHash;

    public RootOfTrust(ASN1Encodable asn1Encodable) throws CertificateParsingException {
        if (!(asn1Encodable instanceof ASN1Sequence)) {
            throw new CertificateParsingException("Expected sequence for root of trust, found "
                    + asn1Encodable.getClass().getName());
        }

        ASN1Sequence sequence = (ASN1Sequence) asn1Encodable;
        Log.d(TAG, "Parsing RootOfTrust sequence with " + sequence.size() + " elements");

        try {
            verifiedBootKey = Asn1Utils.getByteArrayFromAsn1(sequence.getObjectAt(VERIFIED_BOOT_KEY_INDEX));
            deviceLocked = Asn1Utils.getBooleanFromAsn1(sequence.getObjectAt(DEVICE_LOCKED_INDEX));
            verifiedBootState = Asn1Utils.getIntegerFromAsn1(sequence.getObjectAt(VERIFIED_BOOT_STATE_INDEX));
            
            if (sequence.size() > VERIFIED_BOOT_HASH_INDEX) {
                verifiedBootHash = Asn1Utils.getByteArrayFromAsn1(sequence.getObjectAt(VERIFIED_BOOT_HASH_INDEX));
            } else {
                verifiedBootHash = null;
            }
            
            Log.d(TAG, "Successfully parsed RootOfTrust: deviceLocked=" + deviceLocked 
                    + ", verifiedBootState=" + verifiedBootStateToString(verifiedBootState));
        } catch (Exception e) {
            Log.e(TAG, "Failed to parse RootOfTrust sequence", e);
            throw new CertificateParsingException("Failed to parse RootOfTrust sequence: " + e.getMessage());
        }
    }

    public static String verifiedBootStateToString(int verifiedBootState) {
        switch (verifiedBootState) {
            case KM_VERIFIED_BOOT_VERIFIED:
                return "Verified";
            case KM_VERIFIED_BOOT_SELF_SIGNED:
                return "Self-signed";
            case KM_VERIFIED_BOOT_UNVERIFIED:
                return "Unverified";
            case KM_VERIFIED_BOOT_FAILED:
                return "Failed";
            default:
                return "Unknown (" + verifiedBootState + ")";
        }
    }

    public byte[] getVerifiedBootKey() {
        return verifiedBootKey;
    }

    public String getVerifiedBootKeyHex() {
        return bytesToHex(verifiedBootKey);
    }

    public boolean isDeviceLocked() {
        return deviceLocked;
    }

    public int getVerifiedBootState() {
        return verifiedBootState;
    }

    public byte[] getVerifiedBootHash() {
        return verifiedBootHash;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder()
                .append("verifiedBootKey: ")
                .append(bytesToHex(verifiedBootKey))
                .append("\ndeviceLocked: ")
                .append(deviceLocked)
                .append("\nverifiedBootState: ")
                .append(verifiedBootStateToString(verifiedBootState));
        if (verifiedBootHash != null) {
            sb.append("\nverifiedBootHash: ")
                    .append(bytesToHex(verifiedBootHash));
        }
        return sb.toString();
    }

    private static String bytesToHex(byte[] bytes) {
        if (bytes == null) return "null";
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }
}
