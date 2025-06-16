package com.example.overt.device;

import android.app.KeyguardManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.os.BatteryManager;
import android.os.Build;
import android.os.SystemClock;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.Log;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;

public class system_info {

    static public HashMap<String, String> get_system_info(Context context) {
        HashMap<String, String> hashMap = new HashMap<String, String>();

        if(isDeveloperModeEnabled(context)){
            hashMap.put("isDeveloperModeEnabled", String.valueOf(isDeveloperModeEnabled(context)));
        }
        if(isUsbDebugEnabled(context)){
            hashMap.put("isUsbDebugEnabled", String.valueOf(isUsbDebugEnabled(context)));
        }
        if(isAdbEnabled(context)){
            hashMap.put("isAdbEnabled", String.valueOf(isAdbEnabled(context)));
        }
        if(!isSimExist(context)){
            hashMap.put("isSimExist", String.valueOf(isSimExist(context)));
        }
        if(isVPN(context)){
            hashMap.put("isVPN", String.valueOf(isVPN(context)));
        }
        if(isProxyEnabled()){
            hashMap.put("isProxyEnabled", String.valueOf(isProxyEnabled()));
        }
        if(!isPasswordLocked(context)){
            hashMap.put("isPasswordLocked", String.valueOf(isPasswordLocked(context)));
        }
        if(isAdbInstall(context)){
            hashMap.put("isAdbInstall", String.valueOf(isAdbInstall(context)));
        }
        if(isCharging(context)){
            hashMap.put("isCharging", String.valueOf(isCharging(context)));
        }
        return hashMap;
    }

    public static boolean isCharging(Context context) {
        if (context == null) return false;

        IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent batteryStatus = context.registerReceiver(null, filter);
        if (batteryStatus == null) return false;

        int status = batteryStatus.getIntExtra(BatteryManager.EXTRA_STATUS, -1);
        return status == BatteryManager.BATTERY_STATUS_CHARGING
                || status == BatteryManager.BATTERY_STATUS_FULL;
    }

    static public boolean isAdbInstall(Context context) {
        PackageManager pm = context.getPackageManager();
        String installer = pm.getInstallerPackageName(context.getPackageName());
        if (installer == null) {
            return true;
        } else {
            return false;
        }
    }

    public static boolean isSimExist(Context context) {
        try {
            TelephonyManager tm = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
            if (tm == null) return false;
            int simState = tm.getSimState();
            return simState != TelephonyManager.SIM_STATE_ABSENT
                    && simState != TelephonyManager.SIM_STATE_UNKNOWN;
        } catch (SecurityException e) {
            Log.e("system_info", "No READ_PHONE_STATE permission", e);
        } catch (Exception e) {
            Log.e("system_info", "getSimState failed", e);
        }
        return false;
    }

    static public boolean isDeveloperModeEnabled(Context context) {
        return Settings.Global.getInt(context.getContentResolver(),
                Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 0) == 1;
    }

    static public boolean isUsbDebugEnabled(Context context) {
        return Settings.Global.getInt(context.getContentResolver(),
                Settings.Global.ADB_ENABLED, 0) == 1;
    }

    static public boolean isAdbEnabled(Context context) {
        return Settings.Global.getInt(context.getContentResolver(),
                Settings.Global.ADB_ENABLED, 0) == 1;
    }

    static public boolean isProxyEnabled() {
        String proxyHost = System.getProperty("http.proxyHost");
        String proxyPort = System.getProperty("http.proxyPort");
        return proxyHost != null && !proxyHost.isEmpty() && proxyPort != null && !proxyPort.isEmpty();
    }

    static public boolean isVPN(Context context) {
        ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Network network = connectivityManager.getActiveNetwork();
            if (network != null) {
                NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(network);
                if (capabilities != null) {
                    if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN)) {
                        return true;
                    }
                }
            }
        } else {
            NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();
            if (activeNetwork != null) {
                if (activeNetwork.getType() == ConnectivityManager.TYPE_VPN) {
                    return true;
                }
            }
        }
        return false;
    }

    // 是否有锁屏密码
    static public boolean isPasswordLocked(Context context) {
        KeyguardManager keyguardManager = (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
        if (keyguardManager == null) {
            return false;
        }
        // 检查是否启用了锁屏
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            return keyguardManager.isKeyguardSecure();
        } else {
            return keyguardManager.isKeyguardLocked();
        }
    }

}
