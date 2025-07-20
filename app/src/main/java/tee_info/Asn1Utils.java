package tee_info;

import org.bouncycastle.asn1.ASN1Boolean;
import org.bouncycastle.asn1.ASN1Encodable;
import org.bouncycastle.asn1.ASN1Enumerated;
import org.bouncycastle.asn1.ASN1Integer;
import org.bouncycastle.asn1.DEROctetString;

import java.math.BigInteger;
import java.security.cert.CertificateParsingException;

public class Asn1Utils {
    public static int getIntegerFromAsn1(ASN1Encodable asn1Value)
            throws CertificateParsingException {
        if (asn1Value instanceof ASN1Integer) {
            return bigIntegerToInt(((ASN1Integer) asn1Value).getValue());
        } else if (asn1Value instanceof ASN1Enumerated) {
            return bigIntegerToInt(((ASN1Enumerated) asn1Value).getValue());
        } else {
            throw new CertificateParsingException(
                    "Integer value expected, " + asn1Value.getClass().getName() + " found.");
        }
    }

    public static byte[] getByteArrayFromAsn1(ASN1Encodable asn1Encodable)
            throws CertificateParsingException {
        if (!(asn1Encodable instanceof DEROctetString)) {
            throw new CertificateParsingException("Expected DEROctetString");
        }
        return ((DEROctetString) asn1Encodable).getOctets();
    }

    public static boolean getBooleanFromAsn1(ASN1Encodable value)
            throws CertificateParsingException {
        if (!(value instanceof ASN1Boolean)) {
            throw new CertificateParsingException(
                    "Expected boolean, found " + value.getClass().getName());
        }
        ASN1Boolean booleanValue = (ASN1Boolean) value;
        if (booleanValue.equals(ASN1Boolean.TRUE)) {
            return true;
        } else if (booleanValue.equals((ASN1Boolean.FALSE))) {
            return false;
        }

        throw new CertificateParsingException(
                "DER-encoded boolean values must contain either 0x00 or 0xFF");
    }

    private static int bigIntegerToInt(BigInteger bigInt) throws CertificateParsingException {
        if (bigInt.compareTo(BigInteger.valueOf(Integer.MAX_VALUE)) > 0
                || bigInt.compareTo(BigInteger.ZERO) < 0) {
            throw new CertificateParsingException("INTEGER out of bounds");
        }
        return bigInt.intValue();
    }
} 