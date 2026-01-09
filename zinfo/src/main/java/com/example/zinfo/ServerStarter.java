package com.example.zinfo;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.util.Log;

public final class ServerStarter {

    private static final String TAG = "lxz_ServerStarter";
    private static volatile boolean sStarted = false;

    private ServerStarter() {}

    /** 只调用一次即可（建议 Activity.onCreate） */
    public static synchronized void start(Context context) {
        if (sStarted) {
            return;
        }
        sStarted = true;
        try {
            Context app = context.getApplicationContext();
            String pkg = app.getPackageName();
            Intent intent = new Intent();
            intent.setComponent(new ComponentName(pkg, pkg + ".Server"));
            sConnection = createConnection(context);
            boolean ok = app.bindService(intent, sConnection, Context.BIND_AUTO_CREATE);
            Log.d(TAG, "bindService result=" + ok);
        } catch (Throwable t) {
            Log.e(TAG, "start failed", t);
        }
    }

    private static ServiceConnection createConnection(final Context context) {
        return new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                Log.i(TAG, "Server connected: " + name);
                
                // 获取共享内存 fd 并发送给 isolated 进程
                int fd = getSharedMemoryFd();
                if (fd < 0) {
                    Log.e(TAG, "No shared memory fd available");
                    return;
                }
                
                // 通过 Binder 传递 fd
                ISharedMemoryService serviceInterface = ISharedMemoryService.Stub.asInterface(service);
                if (serviceInterface != null) {
                    try {
                        ParcelFileDescriptor pfd = ParcelFileDescriptor.fromFd(fd);
                        serviceInterface.receiveFd(pfd);
                        Log.i(TAG, "Shared memory fd sent to isolated process");
                        // native 层的消息循环线程会自动开始发送消息
                    } catch (Exception e) {
                        Log.e(TAG, "Failed to communicate with isolated process", e);
                    }
                } else {
                    Log.e(TAG, "Failed to get ISharedMemoryService interface");
                }
            }
            
            @Override
            public void onServiceDisconnected(ComponentName name) {
                Log.w(TAG, "Server disconnected: " + name);
            }
        };
    }
    
    private static ServiceConnection sConnection;
    // Native 方法声明
    static public native int getSharedMemoryFd();
}
