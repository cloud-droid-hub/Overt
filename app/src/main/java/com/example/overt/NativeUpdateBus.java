package com.example.overt;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Native -> UI 更新总线。
 * 所有分发都在主线程执行，避免直接静态持有 Activity。
 */
public final class NativeUpdateBus {
    private static final String TAG = "overt_" + NativeUpdateBus.class.getSimpleName();
    private static final Handler MAIN_HANDLER = new Handler(Looper.getMainLooper());

    private static final List<Listener> LISTENERS = new ArrayList<>();
    private static final LinkedHashMap<String, String> LATEST_UPDATES = new LinkedHashMap<>();

    public interface Listener {
        void onCardInfoUpdated(String title, String newCardInfo);
    }

    private NativeUpdateBus() {
    }

    public static void subscribe(Listener listener) {
        if (listener == null) {
            return;
        }
        runOnMainThread(new Runnable() {
            @Override
            public void run() {
                if (!LISTENERS.contains(listener)) {
                    LISTENERS.add(listener);
                }
                // 新订阅者先回放各检测项最近一次状态，避免界面冷启动空白。
                for (Map.Entry<String, String> entry : LATEST_UPDATES.entrySet()) {
                    dispatchToListener(listener, entry.getKey(), entry.getValue());
                }
            }
        });
    }

    public static void unsubscribe(Listener listener) {
        if (listener == null) {
            return;
        }
        runOnMainThread(new Runnable() {
            @Override
            public void run() {
                LISTENERS.remove(listener);
            }
        });
    }

    /**
     * 由 Native 层通过 JNI 调用。
     */
    public static void onCardInfoUpdated(String title, String newCardInfo) {
        runOnMainThread(new Runnable() {
            @Override
            public void run() {
                LATEST_UPDATES.put(title, newCardInfo);
                for (Listener listener : LISTENERS) {
                    dispatchToListener(listener, title, newCardInfo);
                }
            }
        });
    }

    private static void dispatchToListener(Listener listener, String title, String newCardInfo) {
        try {
            listener.onCardInfoUpdated(title, newCardInfo);
        } catch (Throwable t) {
            Log.e(TAG, "Listener callback failed for title=" + title, t);
        }
    }

    private static void runOnMainThread(Runnable runnable) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            runnable.run();
        } else {
            MAIN_HANDLER.post(runnable);
        }
    }
}
