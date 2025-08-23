package com.example.overt;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.widget.NestedScrollView;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "overt_" + MainActivity.class.getSimpleName();
    
    private InfoCardContainer cardContainer;
    private TextView titleBar;
    private FrameLayout cardContainerLayout;
    static private Handler mainHandler;
    
    // 静态引用，用于native层调用静态方法时访问当前Activity实例
    private static MainActivity currentInstance = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 设置当前实例的静态引用
        currentInstance = this;
        
        // 设置布局
        setContentView(R.layout.activity_main);
        
        // 获取视图引用
        titleBar = findViewById(R.id.title_bar);
        cardContainerLayout = findViewById(R.id.card_container);
        
        // 初始化Handler用于在主线程更新UI
        mainHandler = new Handler(Looper.getMainLooper());
        
        // 创建卡片容器（初始数据）

        cardContainer = new InfoCardContainer(this, null);
        
        // 将卡片容器添加到布局中
        NestedScrollView scrollView = cardContainer.getContainerView();
        cardContainerLayout.addView(scrollView);
    }
    
    /**
     * 静态回调方法 - 由native层调用
     * 这个方法可以在没有对象引用的情况下被调用
     */

    public static void onCardInfoUpdated(String title, Map<String, Map<String, String>> newCardInfo) {
        Log.d(TAG, "Received device info update notification from native (static method)");

        if (currentInstance == null) {
            Log.e(TAG, "Current instance is null, cannot update UI");
            return;
        }

        /* ---------- 1 秒超时等待 mainHandler ---------- */
        final long timeout = 1000L;          // 1 秒
        final long deadline = System.currentTimeMillis() + timeout;

        while (mainHandler == null && System.currentTimeMillis() < deadline) {
            try {
                Thread.sleep(50);            // 每 50 ms 检查一次，不会把 CPU 吃满
            } catch (InterruptedException ignore) {
                // 有人打断就直接放弃
                Thread.currentThread().interrupt();
                return;
            }
        }

        if (mainHandler == null) {
            Log.e(TAG, "mainHandler is null, cannot update UI");
            return;
        }

        // 保存当前实例的引用，避免在post执行时被清空
        final MainActivity instance = currentInstance;

        // 切换到主线程更新UI
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                instance.updateUIWithNewCardInfo(title, newCardInfo);
            }
        });
    }
    
    private void updateUIWithNewData(Map<String, Map<String, Map<String, String>>> newDeviceInfo) {
        if (cardContainer != null) {
            Log.d(TAG, "Updating UI with new device info - " + newDeviceInfo.size() + " categories");
            // 更新卡片容器中的数据
            cardContainer.updateData(newDeviceInfo);
        }
    }

    private void updateUIWithNewCardInfo(String title, Map<String, Map<String, String>> newCardInfo) {
        if (cardContainer != null) {
            cardContainer.updateCard(title, newCardInfo);
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        // 清除静态引用
        if (currentInstance == this) {
            currentInstance = null;
        }
        Log.d(TAG, "Device info monitor stopped");
    }
}