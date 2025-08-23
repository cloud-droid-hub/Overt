package com.example.overt;

import android.app.Application;
import android.content.Context;
import android.util.Log;

public class MainApplication extends Application {
    private static final String TAG = "overt_" + MainApplication.class.getSimpleName();
    static {
        Log.e(TAG, "MainApplication static System.loadLibrary overt");
        System.loadLibrary("overt"); // 注意不能在 attachBaseContext 中加载，否则会卡住
    }

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        Log.e(TAG, "MainApplication attachBaseContext");

    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.e(TAG, "MainApplication onCreate");

    }
}
