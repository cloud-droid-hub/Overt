package com.example.zinfo;

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.hardware.fingerprint.FingerprintManager;
import android.os.IBinder;
import android.util.Log;
import androidx.biometric.BiometricManager;
import androidx.core.content.ContextCompat;

import android.os.Build;

public class Main extends Application {

    final public static String TAG = "lxz_Main";

    static {
         System.loadLibrary("zInfo");
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }
}
