package com.example.zcore;

import android.app.Application;

public class Main extends Application {
    static {
        System.loadLibrary("zCore");
    }
}
