package com.example.overt;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import androidx.annotation.Nullable;

public class Server extends Service {
    private static final String TAG = "lxz_Server";

    @Override
    public void onCreate() {
        super.onCreate();
        System.loadLibrary("overt");
        Log.i(TAG, "Server onCreate, isolated process started");
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "Server onBind, returning Binder");
        return mBinder;
    }

    private final ISharedMemoryService.Stub mBinder = new ISharedMemoryService.Stub() {
        @Override
        public void receiveFd(ParcelFileDescriptor pfd) {
            Log.i(TAG, "Received shared memory fd from main process");
            if (pfd != null) {
                int fd = pfd.getFd();
                Log.i(TAG, "fd=" + fd);
                // 调用 native 方法启动 fd 监听器（内部会映射共享内存并启动消息循环线程）
                int ret = startFdListener(fd);
                if (ret == 0) {
                    Log.i(TAG, "Successfully started fd listener");
                    // native 层的消息循环线程会自动开始处理消息
                } else {
                    Log.e(TAG, "Failed to start fd listener");
                }
                // 注意：不要关闭 pfd，fd 生命周期由主进程管理
            } else {
                Log.e(TAG, "Received null ParcelFileDescriptor");
            }
        }
    };

    // Native 方法声明
    private native int startFdListener(int fd);
}
