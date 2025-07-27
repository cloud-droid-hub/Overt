package com.example.overt;

import android.os.Bundle;
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

    // Native方法声明
    native Map<String, Map<String, Map<String, String>>> get_device_info();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 设置布局
        setContentView(R.layout.activity_main);
        
        // 获取视图引用
        titleBar = findViewById(R.id.title_bar);
        cardContainerLayout = findViewById(R.id.card_container);
        
        // 直接从native获取设备信息数据
        Map<String, Map<String, Map<String, String>>> deviceInfo = get_device_info();
        
        // 创建卡片容器
        cardContainer = new InfoCardContainer(this, deviceInfo);
        
        // 将卡片容器添加到布局中
        NestedScrollView scrollView = cardContainer.getContainerView();
        cardContainerLayout.addView(scrollView);
    }
}