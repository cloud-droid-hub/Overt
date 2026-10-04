package com.example.zengine.analyzer;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

/**
 * SslFingerprintFetcher 持久化(date:cert)单测。
 * 验证:当天缓存命中不请求网络;首次请求写入当天 date:cert;历史保留。
 */
public class SslFingerprintFetcherTest {

    private File tmpDir;

    @Before
    public void setUp() throws Exception {
        tmpDir = Files.createTempDirectory("ssl_fp_test").toFile();
        SslFingerprintFetcher.setStorageDir(tmpDir.getAbsolutePath());
    }

    @After
    public void tearDown() throws Exception {
        SslFingerprintFetcher.setStorageDir(null);
        SslFingerprintFetcher.setProvider(null); // 恢复默认网络实现,避免串扰
        if (tmpDir != null) {
            File[] files = tmpDir.listFiles();
            if (files != null) for (File f : files) f.delete();
            tmpDir.delete();
        }
    }

    @Test
    public void firstFetch_writesTodayDateCert() throws Exception {
        // 第一次:provider 返回 "AAA...";应写入 date:cert 文件
        SslFingerprintFetcher.setProvider(url -> "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        String fp = SslFingerprintFetcher.getExpectedFingerprint("https://www.baidu.com");
        assertEquals("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", fp);

        File[] files = tmpDir.listFiles();
        assertNotNull("应生成缓存文件", files);
        assertTrue("应有 ssl_fp_ 文件", files.length == 1 && files[0].getName().startsWith("ssl_fp_"));

        String content = new String(Files.readAllBytes(files[0].toPath()), StandardCharsets.UTF_8);
        String today = new java.text.SimpleDateFormat("yyyyMMdd", java.util.Locale.ROOT)
                .format(new java.util.Date());
        assertTrue("应写入 date:cert 行: " + content,
                content.contains(today + ":AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
    }

    @Test
    public void todayCached_doesNotRequestNetwork() throws Exception {
        // 第一次 provider 返回 A;第二次改成返回 B(若命中缓存则仍返回 A,证明没请求)
        SslFingerprintFetcher.setProvider(url -> "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        String first = SslFingerprintFetcher.getExpectedFingerprint("https://www.baidu.com");
        assertEquals("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", first);

        // 换成会返回 B 的 provider;当天缓存命中应仍返回 A(没有重新请求)
        SslFingerprintFetcher.setProvider(url -> "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
        String second = SslFingerprintFetcher.getExpectedFingerprint("https://www.baidu.com");
        assertEquals("当天缓存应命中,不再请求", first, second);
    }

    @Test
    public void providerFailure_returnsNullWithoutWrite() throws Exception {
        SslFingerprintFetcher.setProvider(url -> null);
        String fp = SslFingerprintFetcher.getExpectedFingerprint("https://www.baidu.com");
        assertEquals(null, fp);
        // 不应写文件(失败的指纹不缓存)
        File[] files = tmpDir.listFiles();
        assertTrue("失败不应生成缓存文件", files == null || files.length == 0);
    }

    @Test
    public void multipleDays_keepHistory() throws Exception {
        // 模拟两天:同 URL 不同天 → 应保留两条 date:cert
        SslFingerprintFetcher.setProvider(url -> "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        SslFingerprintFetcher.getExpectedFingerprint("https://www.baidu.com");

        // 手工把文件里今天的行改成"昨天"(伪造历史),再触发一次"新的一天"写入
        File[] files = tmpDir.listFiles();
        assertNotNull(files);
        File f = files[0];
        String content = new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8);
        // 把今天的行改成 20260101(过去日期),模拟历史存在
        String today = new java.text.SimpleDateFormat("yyyyMMdd", java.util.Locale.ROOT)
                .format(new java.util.Date());
        String yesterdayLine = content.replace(today, "20260101");
        Files.write(f.toPath(), yesterdayLine.getBytes(StandardCharsets.UTF_8));

        // 再取一次(仍今天,provider 返回 A;因今天无缓存 → 请求 → 写今天)
        SslFingerprintFetcher.setProvider(url -> "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        SslFingerprintFetcher.getExpectedFingerprint("https://www.baidu.com");

        String after = new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8);
        assertTrue("应保留历史(20260101)", after.contains("20260101:AAAA"));
        assertTrue("应写入今天", after.contains(today + ":AAAA"));
    }

    @Test
    public void noStorageDir_fallsBackInMemory() throws Exception {
        // 未注入 storageDir 且无 Context → 纯内存,仍能工作(不持久化)
        SslFingerprintFetcher.setStorageDir(null);
        com.example.zengine.ContextProvider.setProvider(() -> null); // 确保 JVM 测试拿不到 Context
        SslFingerprintFetcher.setProvider(url -> "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
        String fp = SslFingerprintFetcher.getExpectedFingerprint("https://www.baidu.com");
        assertEquals("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", fp);
        com.example.zengine.ContextProvider.setProvider(null); // 恢复
    }
}