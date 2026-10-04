package com.example.zengine.analyzer;

import com.example.zengine.MainApplication;

import org.json.JSONObject;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * 应用包信息分析器。
 * <p>
 * 「查杀分离」：C++ 采集端(zPackageInfo.cpp)只保留一份【探测包名列表】(probe_package_map)，
 * 负责探测这些包是否安装并上报安装方式；【黑名单名单】与【白名单名单】全部内置在此分析器，
 * 判定只对名单内包生效——非黑非白的探测对象即使已安装也不报错。
 * <p>
 * 迁移自 zinfo zPackageInfo.cpp get_package_info 的内联判断：
 * - 黑名单包已安装(value 为 "pms"/"file"/"path_hole"/"shell_hole" 之一)→ error
 * - 白名单包(微信/支付宝)value=="0" 未安装 → warn
 */
public final class PackageInfoAnalyzer implements MainApplication.Analyzer {

    /** 黑名单包名 → 应用名(内置判定名单,与采集端探测列表对应)。 */
    private static final Map<String, String> BLACK_MAP = new HashMap<>();

    /** 白名单包名 → 应用名(应正常安装的官方应用,缺失时 warn)。 */
    private static final Map<String, String> WHITE_MAP = new HashMap<>();

    static {
        // ---------------- 黑名单：Root管理工具 ----------------
        BLACK_MAP.put("me.weishu.kernelsu", "KernelSU");
        BLACK_MAP.put("com.topjohnwu.magisk", "Magisk");
        BLACK_MAP.put("io.github.vvb2060.magisk", "Magisk Alpha");
        BLACK_MAP.put("io.github.huskydg.magisk", "Kitsune Mask");
        BLACK_MAP.put("io.github.cycle1337.kernelsu", "KernelSU");
        BLACK_MAP.put("com.rifsxd.ksunext", "KernelSU Next");
        BLACK_MAP.put("com.sukisu.ultra", "SukiSU Ultra");
        BLACK_MAP.put("me.bmax.apatch", "APatch");
        BLACK_MAP.put("org.lsposed.manager", "LSPosed");
        BLACK_MAP.put("de.robv.android.xposed.installer", "Xposed Installer");

        // ---------------- 黑名单：调试工具 ----------------
        BLACK_MAP.put("com.zhenxi.hunter", "Hunter");
        BLACK_MAP.put("com.juqing.catchpackhelper", "抓包帮手");
        BLACK_MAP.put("bin.mt.plus", "MT 管理器");
        BLACK_MAP.put("moe.haruue.wadb", "无线 adb");
        BLACK_MAP.put("org.hapjs.debuger", "快应用调试器");
        BLACK_MAP.put("org.hapjs.mockup", "快应用预览版");
        BLACK_MAP.put("com.coolapk.market", "DNA-Android");
        BLACK_MAP.put("com.atominvention.rootchecker", "Root 测试工具");
        BLACK_MAP.put("io.github.vvb2060.xposeddetector", "密钥认证");
        BLACK_MAP.put("io.github.huskydg.memorydetector", "MemoryDetector");
        BLACK_MAP.put("com.byxiaorun.detector", "Ruru");
        BLACK_MAP.put("icu.nullptr.nativetest", "Native Test");
        BLACK_MAP.put("io.github.vvb2060.mahoshojo", "Momo");
        BLACK_MAP.put("luna.safe.luna", "Luna");

        BLACK_MAP.put("ru.maximoff.apktool", "Apktool");
        BLACK_MAP.put("top.niunaijun.blackdexa64", "BlackDex64");
        BLACK_MAP.put("formatfa.xposed.Fdex2", "Fdex2");
        BLACK_MAP.put("me.weishu.exp", "微术实验");
        BLACK_MAP.put("org.autojs.autojs", "Auto.js");
        BLACK_MAP.put("net.dinglisch.android.taskerm", "Tasker");
        BLACK_MAP.put("app.greyshirts.sslcapture", "SSL Capture");
        BLACK_MAP.put("cn.trinea.android.developertools", "开发者工具");
        BLACK_MAP.put("com.lefan.apkanaly", "Lefan APK分析");

        // ---------------- 黑名单：VPN与代理工具 ----------------
        BLACK_MAP.put("com.guoshi.httpcanary", "HttpCanary");
        BLACK_MAP.put("org.charlesproxy.charles", "Charles代理");
        BLACK_MAP.put("de.blinkt.openvpn", "OpenVPN");
        BLACK_MAP.put("net.openvpn.openvpn", "OpenVPN Connect");
        BLACK_MAP.put("com.github.kr328.clash", "Clash for Android");
        BLACK_MAP.put("com.crosserr.trojan", "Trojan代理");
        BLACK_MAP.put("com.qi.tiaozhuan", "跳转工具");
        BLACK_MAP.put("com.qi.huguanproxy", "代理工具");
        BLACK_MAP.put("com.qi.staticsproxy", "静态代理");
        BLACK_MAP.put("com.qi.earthnutproxy", "Earthnut代理");
        BLACK_MAP.put("com.qi.hjproxy", "HJ代理");
        BLACK_MAP.put("com.linghang520.iphaidtnet", "领航VPN");
        BLACK_MAP.put("com.linghang520.iphainet", "领航VPN");
        BLACK_MAP.put("com.linghang520.jlipnet", "领航VPN");
        BLACK_MAP.put("com.linghang520.ipmnqdtnet", "领航模拟器VPN");
        BLACK_MAP.put("com.linghang520.lhdtnet", "领航VPN");
        BLACK_MAP.put("com.whitebunny.vpn", "WhiteBunny VPN");
        BLACK_MAP.put("com.fishervpn.freevpn", "Fisher VPN");
        BLACK_MAP.put("com.fastfun.vpn", "FastFun VPN");
        BLACK_MAP.put("com.dmvpn.vpnfree", "DM VPN");
        BLACK_MAP.put("com.daxiang.vpn", "大象VPN");
        BLACK_MAP.put("com.birdvpn.app", "BirdVPN");
        BLACK_MAP.put("com.avira.vpn", "Avira VPN");
        BLACK_MAP.put("cn.hm.vpn", "华盟VPN");
        BLACK_MAP.put("com.ichano.deepipconverter", "Deep IP转换器");
        BLACK_MAP.put("com.tuziip.tuzi", "兔子IP");
        BLACK_MAP.put("tool.seagull.v", "海鸥工具");
        BLACK_MAP.put("com.shansulian.IP.app", "闪速连IP");
        BLACK_MAP.put("com.hongtuwuyou.wyip", "宏图无忧IP");
        BLACK_MAP.put("com.fvcorp.flyclient", "飞鱼客户端");
        BLACK_MAP.put("com.v2cross.shadowrocket", "Shadowrocket");
        BLACK_MAP.put("com.v2cross.shadowshare", "Shadowsocks Share");
        BLACK_MAP.put("com.vpnarea", "VPNArea");
        BLACK_MAP.put("hideme.android.vpn", "HideMe VPN");
        BLACK_MAP.put("com.free.vpn.proxy.master.app", "VPN Master");
        BLACK_MAP.put("com.xiaobei.shenlongjiasu", "神龙加速");
        BLACK_MAP.put("com.tianqiip.sstp", "天齐IP");
        BLACK_MAP.put("com.fvcorp.android.aijiasuclient", "爱加速客户端");
        BLACK_MAP.put("com.lishun.flyfish", "飞鱼VPN");
        BLACK_MAP.put("com.dongguo.feiyu", "东国飞鱼");
        BLACK_MAP.put("com.longene.cake", "Cake");
        BLACK_MAP.put("com.xiyan.xiniu", "西燕犀牛");
        BLACK_MAP.put("org.skylineacc.android.client", "Skyline ACC");
        BLACK_MAP.put("com.getlantern.lantern", "Lantern（蓝灯）");
        BLACK_MAP.put("com.psiphon3.subscription", "Psiphon");
        BLACK_MAP.put("com.shadowrocket", "Shadowrocket");
        BLACK_MAP.put("com.qiuyou.network", "秋友网络");
        BLACK_MAP.put("com.v2ray.angel", "V2Ray Angel");
        BLACK_MAP.put("com.kiwi.vpn", "Kiwi VPN");
        BLACK_MAP.put("com.cloudvpn", "Cloud VPN");
        BLACK_MAP.put("com.coolvpn", "Cool VPN");
        BLACK_MAP.put("com.yunti", "云梯");
        BLACK_MAP.put("com.fastvpn", "Fast VPN");
        BLACK_MAP.put("com.vpn.android", "VPN Android");
        BLACK_MAP.put("com.accelerator.vpn", "加速器VPN");
        BLACK_MAP.put("com.earthvpn", "Earth VPN");
        BLACK_MAP.put("com.rocketvpn", "Rocket VPN");
        BLACK_MAP.put("com.quickvpn", "Quick VPN");
        BLACK_MAP.put("com.bigcannon.vpn", "Big Cannon VPN");
        BLACK_MAP.put("com.cloudshield.vpn", "CloudShield VPN");
        BLACK_MAP.put("com.securebrowser", "安全浏览器");
        BLACK_MAP.put("com.freevpn", "Free VPN");
        BLACK_MAP.put("com.superspeed.vpn", "SuperSpeed VPN");
        BLACK_MAP.put("com.shieldvpn", "Shield VPN");
        BLACK_MAP.put("com.invisiblevpn", "Invisible VPN");
        BLACK_MAP.put("com.freenet", "FreeNet");
        BLACK_MAP.put("com.aispeed", "AI Speed");
        BLACK_MAP.put("com.v2ray.ang", "V2RayNG");
        BLACK_MAP.put("com.expressvpn.vpn", "ExpressVPN");
        BLACK_MAP.put("ch.protonvpn.android", "ProtonVPN");
        BLACK_MAP.put("com.github.shadowsocks", "Shadowsocks");
        BLACK_MAP.put("com.surfshark.vpnclient.android", "Surfshark");
        BLACK_MAP.put("com.windscribe.vpn", "Windscribe");
        BLACK_MAP.put("com.nordvpn.android", "NordVPN");
        BLACK_MAP.put("com.freevpnintouch", "FreeVPNInTouch");
        BLACK_MAP.put("hotspotshield.android.vpn", "Hotspot Shield");
        BLACK_MAP.put("com.goldenfrog.vyprvpn.app", "VyprVPN");
        BLACK_MAP.put("com.xy.vpn", "XY VPN");
        BLACK_MAP.put("com.wl.ufovpn", "UFO VPN");
        BLACK_MAP.put("com.ifast.virtualvpn", "iFast Virtual VPN");
        BLACK_MAP.put("com.suxxt.vpnanonymity", "VPN Anonymity");
        BLACK_MAP.put("com.njh.biubiu", "Biubiu VPN");
        BLACK_MAP.put("com.netease.uu", "网易UU加速器");
        BLACK_MAP.put("com.xiongmao886.tun", "熊猫TUN");
        BLACK_MAP.put("com.zhima.aurora", "芝麻极光");

        // ---------------- 白名单：应正常安装的官方应用 ----------------
        WHITE_MAP.put("com.tencent.mm", "微信");
        WHITE_MAP.put("com.eg.android.AlipayGphone", "支付宝");
    }

    @Override
    public String analyze(String rawJson) {
        try {
            JSONObject raw = new JSONObject(rawJson);
            JSONObject out = new JSONObject();
            Iterator<String> keys = raw.keys();

            while (keys.hasNext()) {
                String pkg = keys.next();
                JSONObject item = raw.optJSONObject(pkg);
                if (item == null) continue;
                String value = item.optString("value", "");

                // 白名单包:value=="0" → 未安装警告(微信/支付宝)
                String whiteName = WHITE_MAP.get(pkg);
                if (whiteName != null) {
                    if ("0".equals(value)) {
                        out.put(pkg, new JSONObject()
                                .put("risk", "warn")
                                .put("explain", "white package name but uninstall " + whiteName));
                    }
                    continue;
                }

                // 黑名单包:已安装(任意方式)→ error(仅在名单内判定)
                String blackName = BLACK_MAP.get(pkg);
                if (blackName != null && !"0".equals(value) && !value.isEmpty()) {
                    out.put(pkg, new JSONObject()
                            .put("risk", "error")
                            .put("explain", "black package name but install[" + value + "] " + blackName));
                }
            }
            return out.toString();
        } catch (Exception e) {
            return "{}";
        }
    }
}