package com.example.overt;

import android.app.Application;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import java.util.List;

public class MainApplication extends Application {
    private static final String TAG = "overt_" + MainApplication.class.getSimpleName();
    static {
        Log.i(TAG, "MainApplication static System.loadLibrary overt");
         System.loadLibrary("overt"); // 注意不能在 attachBaseContext 中加载，否则会卡住
    }

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        Log.i(TAG, "MainApplication attachBaseContext");
    }


    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "MainApplication onCreate");
    }
}
