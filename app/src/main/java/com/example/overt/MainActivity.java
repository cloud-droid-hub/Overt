package com.example.overt;

import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.widget.NestedScrollView;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    
    private InfoCardContainer cardContainer;

    // Native方法声明
    native Map<String, Map<String, Map<String, String>>> get_device_info();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 直接从native获取设备信息数据
        Map<String, Map<String, Map<String, String>>> deviceInfo = get_device_info();
        
        // 创建卡片容器（简化调用）
        cardContainer = new InfoCardContainer(this, deviceInfo);
        
        // 直接使用容器View作为根布局（支持滚动）
        setContentView(cardContainer.getContainerView());
    }
}