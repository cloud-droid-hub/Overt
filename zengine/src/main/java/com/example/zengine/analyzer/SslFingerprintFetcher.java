package com.example.zengine.analyzer;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import javax.net.ssl.HttpsURLConnection;
import java.net.URL;

/**
 * 动态获取 HTTPS 证书指纹(zengine,替代硬编码期望指纹)。
 * <p>
 * 查杀分离下的期望指纹获取:不写死某个指纹,而是由 zengine(Java)按需请求目标 URL,
 * 现场解析证书链叶子证书的 SHA-256 指纹作为"期望值",与 native 采集端上报的观察指纹对比。
 * <p>
 * 持久化(date:cert 键值对):
 * - 存储文件:<storageDir>/ssl_fp_<urlHash>.txt,每行 "date:cert"(当日覆盖同名,历史追加)
 * - 取值逻辑:先查【当天】date 的缓存指纹 → 有则直接用(不请求网络);
 *   当天没有 → 网络请求 → 写当天 date:cert → 用于对比
 * - storageDir 由 app 侧注入(context.getFilesDir());未注入则退化为纯内存(单测/无存储)。
 */
public final class SslFingerprintFetcher {

    /** 指纹获取函数接口(可注入,单测用假实现)。 */
    public interface FingerprintProvider {
        /** @return 叶子证书 SHA-256 指纹(hex 大写,无冒号);失败返回 null。 */
        String fetch(String url);
    }

    private static volatile FingerprintProvider provider = SslFingerprintFetcher::fetchFromNetwork;
    private static volatile String storageDir = null; // 显式注入(单测用);null 时尝试从 Context 自动获取
    private static final SimpleDateFormat DATE_FMT = new SimpleDateFormat("yyyyMMdd", Locale.ROOT);

    /** 显式注入持久化存储目录(单测用;null 恢复自动从 Context 获取)。 */
    public static void setStorageDir(String dir) {
        storageDir = dir;
    }

    /** 注入自定义获取器(单测用)。传入 null 恢复默认网络实现。 */
    public static void setProvider(FingerprintProvider p) {
        provider = (p == null) ? SslFingerprintFetcher::fetchFromNetwork : p;
    }

    /**
     * 获取当天的期望指纹(当天有缓存直接用;没有则网络请求并写入当天 date:cert)。
     *
     * @param url 目标 URL
     * @return 叶子证书 SHA-256 指纹(hex 大写);获取失败返回 null
     */
    public static String getExpectedFingerprint(String url) {
        if (url == null || url.isEmpty()) return null;
        String today = DATE_FMT.format(new Date());

        // 1) 先查【当天】持久化缓存
        String cached = readTodayFingerprint(url, today);
        if (cached != null) {
            return cached;
        }

        // 2) 当天没有 → 网络请求
        String fp = (provider != null) ? provider.fetch(url) : null;
        if (fp != null) {
            writeFingerprint(url, today, fp);
        }
        return fp;
    }

    /** 读取当天持久化指纹(从 date:cert 文件里找 date==today 的行)。 */
    private static String readTodayFingerprint(String url, String today) {
        File f = cacheFile(url);
        if (f == null || !f.exists()) return null;
        try {
            for (String line : Files.readAllLines(f.toPath(), StandardCharsets.UTF_8)) {
                int colon = line.indexOf(':');
                if (colon <= 0) continue;
                String date = line.substring(0, colon);
                String cert = line.substring(colon + 1).trim();
                if (today.equals(date) && !cert.isEmpty()) {
                    return cert;
                }
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    /** 写入一条 date:cert(当日覆盖同名日期的行,其他行保留;无则追加)。 */
    private static void writeFingerprint(String url, String date, String cert) {
        File f = cacheFile(url);
        if (f == null) return;
        try {
            StringBuilder sb = new StringBuilder();
            boolean replaced = false;
            if (f.exists()) {
                for (String line : Files.readAllLines(f.toPath(), StandardCharsets.UTF_8)) {
                    int colon = line.indexOf(':');
                    String lineDate = (colon > 0) ? line.substring(0, colon) : "";
                    // 覆盖今天的旧行,保留历史
                    if (date.equals(lineDate)) {
                        sb.append(date).append(':').append(cert).append('\n');
                        replaced = true;
                    } else {
                        sb.append(line).append('\n');
                    }
                }
            }
            if (!replaced) {
                // 没有今天的行 → 追加
                if (sb.length() > 0 && sb.charAt(sb.length() - 1) != '\n') sb.append('\n');
                sb.append(date).append(':').append(cert).append('\n');
            }
            try (FileOutputStream fos = new FileOutputStream(f, false)) {
                fos.write(sb.toString().getBytes(StandardCharsets.UTF_8));
            }
        } catch (Exception ignored) {
        }
    }

    /** 缓存文件路径:<storageDir>/ssl_fp_<urlHash>.txt。storageDir 未注入时尝试从 Context 获取 filesDir;都无 → 返回 null(纯内存)。 */
    private static File cacheFile(String url) {
        String dir = storageDir;
        if (dir == null) {
            // 从 ContextProvider 自动获取 app 私有目录
            Object ctx = com.example.zengine.ContextProvider.getContext();
            if (ctx != null) {
                try {
                    java.lang.reflect.Method getFilesDir =
                            ctx.getClass().getMethod("getFilesDir");
                    Object filesDir = getFilesDir.invoke(ctx);
                    if (filesDir != null) {
                        dir = filesDir.toString();
                    }
                } catch (Throwable ignored) {
                }
            }
        }
        if (dir == null) return null;
        String urlHash = Integer.toHexString(url.hashCode());
        return new File(dir, "ssl_fp_" + urlHash + ".txt");
    }

    /** 默认网络实现:HTTPS 请求目标,取叶子证书 SHA-256 指纹。 */
    private static String fetchFromNetwork(String urlString) {
        HttpsURLConnection conn = null;
        try {
            URL url = new URL(urlString);
            conn = (HttpsURLConnection) url.openConnection();
            conn.setConnectTimeout(5000);
            conn.setReadTimeout(5000);
            conn.setInstanceFollowRedirects(true);
            conn.connect();
            Certificate[] chain = conn.getServerCertificates();
            if (chain == null || chain.length == 0) return null;
            // 叶子证书 = 链第一个
            X509Certificate leaf = (X509Certificate) chain[0];
            return sha256Hex(leaf.getEncoded());
        } catch (Exception e) {
            android.util.Log.e("zengine_ssl", "fetchFromNetwork failed for " + urlString + ": " + e);
            return null;
        } finally {
            if (conn != null) conn.disconnect();
        }
    }

    private static String sha256Hex(byte[] data) throws Exception {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        byte[] digest = md.digest(data);
        StringBuilder sb = new StringBuilder();
        for (byte b : digest) {
            sb.append(String.format("%02X", b));
        }
        return sb.toString();
    }

    private SslFingerprintFetcher() {
    }
}