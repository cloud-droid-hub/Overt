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
    private static final String TAG = "MainActivity";
    
    private InfoCardContainer cardContainer;
    private TextView titleBar;
    private FrameLayout cardContainerLayout;
    static private Handler mainHandler;
    
    // 静态引用，用于native层调用静态方法时访问当前Activity实例
    private static MainActivity currentInstance = null;

    // Native方法声明
    native Map<String, Map<String, Map<String, String>>> get_device_info();

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
        Map<String, Map<String, Map<String, String>>> initialDeviceInfo = get_device_info();
        cardContainer = new InfoCardContainer(this, initialDeviceInfo);
        
        // 将卡片容器添加到布局中
        NestedScrollView scrollView = cardContainer.getContainerView();
        cardContainerLayout.addView(scrollView);
    }
    
    /**
     * 静态回调方法 - 由native层调用
     * 这个方法可以在没有对象引用的情况下被调用
     */
    public static void onDeviceInfoUpdated() {
        Log.d(TAG, "Received device info update notification from native (static method)");
        
        if (currentInstance == null) {
            Log.e(TAG, "Current instance is null, cannot update UI");
            return;
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
                // 使用保存的实例引用，避免重复检查
                Map<String, Map<String, Map<String, String>>> newDeviceInfo = instance.get_device_info();
                instance.updateUIWithNewData(newDeviceInfo);
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