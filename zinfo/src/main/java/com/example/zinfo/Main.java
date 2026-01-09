package com.example.zinfo;

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;


public class Main extends Application {

    final public static String TAG = "lxz_Main";
    private static boolean sBound = false;

    static {
         System.loadLibrary("zInfo");
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }


}
