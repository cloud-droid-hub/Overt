package com.example.overt;

import android.app.Application;
import android.content.Context;
import android.util.Log;

/**
 * 主应用程序类 - Overt安全检测工具的Application入口
 * 
 * 功能说明：
 * 1. 作为应用程序的全局入口点，负责应用级别的初始化
 * 2. 加载Native库（overt.so），启动Native层检测功能
 * 3. 管理应用程序的生命周期
 * 4. 提供全局的应用程序上下文
 * 
 * 设计特点：
 * - 在静态代码块中加载Native库，确保库在应用启动时就被加载
 * - 避免在attachBaseContext中加载库，防止启动卡顿
 * - 提供详细的日志记录，便于调试和监控
 */
public class MainApplication extends Application {
    private static final String TAG = "overt_" + MainApplication.class.getSimpleName();
    
    /**
     * 静态代码块 - 在类加载时执行
     * 
     * 功能说明：
     * 1. 加载Native库（overt.so）
     * 2. 启动Native层的初始化流程
     * 3. 确保库在应用启动时就被加载
     * 
     * 注意事项：
     * - 不能在attachBaseContext中加载，否则会导致应用卡住
     * - 静态代码块在类加载时执行，早于任何实例方法
     * - 加载失败会导致应用崩溃，这是预期的行为
     */
    static {
        Log.i(TAG, "MainApplication static - loading native library 'overt'");
        System.loadLibrary("overt"); // 加载Native库，启动检测功能
        Log.i(TAG, "MainApplication static - native library loaded successfully");
    }

    /**
     * 附加基础上下文时的回调方法
     * 
     * 功能说明：
     * 1. 调用父类方法完成基础上下文设置
     * 2. 记录上下文附加日志
     * 
     * 执行时机：
     * - 在Application构造函数之后
     * - 在onCreate方法之前
     * - 在Activity创建之前
     * 
     * @param base 基础上下文，通常是ContextImpl实例
     */
    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        Log.i(TAG, "MainApplication attachBaseContext - context attached");
    }

    /**
     * 应用程序创建时的回调方法
     * 
     * 功能说明：
     * 1. 调用父类onCreate完成基础初始化
     * 2. 记录应用程序创建日志
     * 3. 执行应用程序级别的初始化工作
     * 
     * 执行时机：
     * - 在attachBaseContext之后
     * - 在第一个Activity创建之前
     * - 整个应用程序生命周期中只执行一次
     * 
     * 注意事项：
     * - 不要在此方法中执行耗时操作
     * - 可以在此处初始化全局变量和单例对象
     */
    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "MainApplication onCreate - application created");
        // 应用程序级别的初始化可以在这里进行
        // 例如：初始化全局配置、设置默认值等
    }
}
