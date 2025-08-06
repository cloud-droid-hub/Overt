package com.example.zstd;

import android.app.Application;

public class Main extends Application {
    static {
        System.loadLibrary("zStd");
    }
}
