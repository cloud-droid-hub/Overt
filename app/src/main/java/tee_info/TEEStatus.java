package tee_info;

import android.os.Build;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Log;

import java.security.KeyPairGenerator;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.security.spec.ECGenParameterSpec;
import java.util.HashMap;
import java.util.Map;

public class TEEStatus {
    private static final String TAG = "lxz_" + TEEStatus.class.getSimpleName();
    
    // 单例实例
    private static volatile TEEStatus instance;

    public static final int KM_VERIFIED_BOOT_VERIFIED = 0;
    public static final int KM_VERIFIED_BOOT_SELF_SIGNED = 1;
    public static final int KM_VERIFIED_BOOT_UNVERIFIED = 2;
    public static final int KM_VERIFIED_BOOT_FAILED = 3;

    // 状态属性
    private boolean isDeviceLocked;        // 设备是否锁定
    private int verifiedBootState;         // 启动验证状态
    private int securityLevel;             // 安全级别
    private String verifiedBootKey;
    private String status;

    // 私有构造函数
    private TEEStatus() {
        try {
            // 获取 KeyStore 实例
            KeyStore keyStore = KeyStore.getInstance("AndroidKeyStore");
            keyStore.load(null);

            // 创建密钥生成参数
            KeyGenParameterSpec.Builder builder = null;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                builder = new KeyGenParameterSpec.Builder(
                        "tee_check_key",
                        KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                        .setAlgorithmParameterSpec(new ECGenParameterSpec("secp256r1"))
                        .setDigests(KeyProperties.DIGEST_SHA256)
                        .setAttestationChallenge("tee_check".getBytes());
            }

            // 生成密钥对
            KeyPairGenerator keyPairGenerator = KeyPairGenerator.getInstance(
                    KeyProperties.KEY_ALGORITHM_EC, "AndroidKeyStore");
            keyPairGenerator.initialize(builder.build());
            keyPairGenerator.generateKeyPair();

            // 获取证书链
            Certificate[] certs = keyStore.getCertificateChain("tee_check_key");
            if (certs == null || certs.length == 0) {
                Log.e(TAG, "无法获取证书链");
            }

            // 解析证书
            X509Certificate attestationCert = (X509Certificate) certs[0];
            Attestation attestation = Attestation.loadFromCertificate(attestationCert);

            // 获取 RootOfTrust 信息
            RootOfTrust rootOfTrust = attestation.getRootOfTrust();
            if (rootOfTrust == null) {
                Log.e(TAG, "无法获取 RootOfTrust 信息");
            }

            // 获取安全级别
            isDeviceLocked = rootOfTrust.isDeviceLocked();
            securityLevel = attestation.getAttestationSecurityLevel();
            verifiedBootState = rootOfTrust.getVerifiedBootState();
            verifiedBootKey = bytesToHex(rootOfTrust.getVerifiedBootKey());

            // 构建状态描述
            if (isDeviceLocked) {
                status = "设备已解锁";
            } else if (verifiedBootState != RootOfTrust.KM_VERIFIED_BOOT_VERIFIED) {
                status = "启动验证未通过";
            } else {
                status = "设备已锁定且启动已验证";
            }

        } catch (Exception e) {
            Log.e(TAG, "检测失败: " + e.getMessage());
        } finally {
            try {
                // 清理生成的密钥
                KeyStore keyStore = KeyStore.getInstance("AndroidKeyStore");
                keyStore.load(null);
                keyStore.deleteEntry("tee_check_key");
            } catch (Exception e) {
                // 忽略清理错误
            }
        }
    }

    // 获取单例实例的公共方法
    public static TEEStatus getInstance() {
        if (instance == null) {
            synchronized (TEEStatus.class) {
                if (instance == null) {
                    instance = new TEEStatus();
                }
            }
        }
        return instance;
    }



    // Getter 方法
    public boolean isDeviceLocked() {
        return isDeviceLocked;
    }

    public String getDeviceLockedString() {
        if (isDeviceLocked){
            return "DeviceLocked";
        }
        return "DeviceUnLocked";
    }

    public int getVerifiedBootState() {
        return verifiedBootState;
    }

    public String getVerifiedBootStateString() {
        switch (verifiedBootState) {
            case KM_VERIFIED_BOOT_VERIFIED:
                return "Verified";      // 已证实的
            case KM_VERIFIED_BOOT_SELF_SIGNED:
                return "Self-signed";   // 自行签名
            case KM_VERIFIED_BOOT_UNVERIFIED:
                return "Unverified";    // 未经证实的
            case KM_VERIFIED_BOOT_FAILED:
                return "Failed";        // 失败
            default:
                return "Unknown (" + verifiedBootState + ")";
        }
    }

    public int getSecurityLevel() {
        return securityLevel;
    }

    public String getVerifiedBootKey() {
        return verifiedBootKey;
    }

    public String getStatus() {
        return status;
    }

    // 工具方法
    private static String bytesToHex(byte[] bytes) {
        StringBuilder hexString = new StringBuilder();
        for (byte b : bytes) {
            String hex = Integer.toHexString(0xff & b);
            if (hex.length() == 1) {
                hexString.append('0');
            }
            hexString.append(hex);
        }
        return hexString.toString();
    }


    public static Map<String, Map<String, String>> get_tee_info() {
        Map<String, Map<String, String>> map = new HashMap<String, Map<String, String>>();  // 使用 HashMap 实现 Map 接口

        if(TEEStatus.getInstance().getDeviceLockedString() != "DeviceLocked"){
            map.put("DeviceLocked:" + TEEStatus.getInstance().getDeviceLockedString(), new HashMap<String, String>() {{
                put("risk", "error");  // 传递默认参数
                put("explain", "tee is not locked");  // 传递默认参数
            }});
        }
        if(TEEStatus.getInstance().getVerifiedBootStateString() != "Verified"){
            map.put("VerifiedBootState:" + TEEStatus.getInstance().getVerifiedBootStateString(), new HashMap<String, String>() {{
                put("risk", "error");  // 传递默认参数
                put("explain", "tee is not verified");  // 传递默认参数
            }});
        }

        return map;
    }
}
