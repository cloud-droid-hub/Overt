package com.example.overt;

import android.app.Application;
import android.content.Context;
import android.util.Log;

public class MainApplication extends Application {

    static {
        Log.e("lxz", "MainApplication static System.loadLibrary overt");
        System.loadLibrary("overt"); // 注意不能在 attachBaseContext 中调用，否则会卡住
    }

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        Log.e("lxz", "MainApplication attachBaseContext");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.e("lxz", "MainApplication onCreate");

    }
}
