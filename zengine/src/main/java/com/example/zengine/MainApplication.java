package com.example.zengine;

import com.example.zengine.analyzer.ClassInfoAnalyzer;
import com.example.zengine.analyzer.ClassLoaderAnalyzer;
import com.example.zengine.analyzer.FingerInfoAnalyzer;
import com.example.zengine.analyzer.LinkerInfoAnalyzer;
import com.example.zengine.analyzer.LocalNetworkAnalyzer;
import com.example.zengine.analyzer.LogcatInfoAnalyzer;
import com.example.zengine.analyzer.PackageInfoAnalyzer;
import com.example.zengine.analyzer.PortInfoAnalyzer;
import com.example.zengine.analyzer.ProcInfoAnalyzer;
import com.example.zengine.analyzer.RiskFileAnalyzer;
import com.example.zengine.analyzer.SensorInfoAnalyzer;
import com.example.zengine.analyzer.SideChannelAnalyzer;
import com.example.zengine.analyzer.SignatureInfoAnalyzer;
import com.example.zengine.analyzer.SslInfoAnalyzer;
import com.example.zengine.analyzer.SystemPropAnalyzer;
import com.example.zengine.analyzer.SystemSettingAnalyzer;
import com.example.zengine.analyzer.TeeInfoAnalyzer;
import com.example.zengine.analyzer.TimeInfoAnalyzer;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * zengine 分析引擎入口。
 * <p>
 * 查杀分离的核心：采集端(C++ zinfo)只收集全量原始数据并上传(notice_java)，
 * 所有风险判定(黑名单/阈值)都在这套 Java 分析器中完成。
 * 已迁移的类别由对应 analyzer 分析；未迁移的类别原样透传(其采集端仍输出 {risk, explain})。
 * <p>
 * 纯逻辑类(不 extends Application、零 Android 依赖)，可被 app 通过 sourceSets
 * 源码共享，也可在本模块内以硬编码数据独立单测。
 */
public final class MainApplication {

    /** 单一分析器接口：入参 rawJson = {item: {value: ...}} 原始数据，返回 {item: {risk, explain}} 分析结果 JSON。 */
    public interface Analyzer {
        String analyze(String rawJson);
    }

    private static final Map<String, Analyzer> ANALYZERS = new LinkedHashMap<>();

    static {
        // 查杀分离：全部 19 类检测的 risk 判定都在此注册(zManager 任务名 → analyzer)
        register("finger_info", new FingerInfoAnalyzer());
        register("linker_info", new LinkerInfoAnalyzer());
        register("proc_info", new ProcInfoAnalyzer());
        register("risk_file_info", new RiskFileAnalyzer());
        register("tee_info", new TeeInfoAnalyzer());
        register("class_loader_info", new ClassLoaderAnalyzer());
        register("class_info", new ClassInfoAnalyzer());
        register("package_info", new PackageInfoAnalyzer());
        register("system_setting_info", new SystemSettingAnalyzer());
        register("system_prop_info", new SystemPropAnalyzer());
        register("signature_info", new SignatureInfoAnalyzer());
        register("port_info", new PortInfoAnalyzer());
        register("time_info", new TimeInfoAnalyzer());
        register("ssl_info", new SslInfoAnalyzer());
        register("local_network_info", new LocalNetworkAnalyzer());
        register("logcat_info", new LogcatInfoAnalyzer());
        register("side_channel_info", new SideChannelAnalyzer());
        // isoloated_process_info 复用 proc_info 的分析规则(隔离进程侧返回同一原始结构)
        register("isoloated_process_info", new ProcInfoAnalyzer());
        register("sensor_info", new SensorInfoAnalyzer());
    }

    /** 注册(或覆盖)指定检测类别的分析器。 */
    public static void register(String category, Analyzer analyzer) {
        ANALYZERS.put(category, analyzer);
    }

    /**
     * 分析入口：app 传入检测类别与原始数据 JSON，返回分析后的 {item: {risk, explain}} JSON。
     *
     * @param category 检测类别(与 zManager 任务名一致，如 "risk_file_info")
     * @param rawJson  采集端原始数据 JSON({item: {value: ...}} 或已分析 {item: {risk, explain}})
     * @return 分析结果 JSON；已注册类别 → analyzer 判定结果；未注册类别 → 原样透传 rawJson
     *         (未迁移的采集器仍输出 {risk, explain},透传保持现状 UI,避免卡片消失)
     */
    public static String analyze(String category, String rawJson) {
        Analyzer analyzer = ANALYZERS.get(category);
        return (analyzer == null) ? rawJson : analyzer.analyze(rawJson);
    }

    private MainApplication() {
    }
}