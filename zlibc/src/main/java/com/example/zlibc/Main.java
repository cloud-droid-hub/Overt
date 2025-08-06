package com.example.zlibc;

import android.app.Application;

public class Main extends Application {
    static {
        System.loadLibrary("zLibc");
    }
}
