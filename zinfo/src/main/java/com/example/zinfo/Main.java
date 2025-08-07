package com.example.zinfo;

import android.app.Application;

public class Main extends Application {
    static {
        System.loadLibrary("zInfo");
    }
}
