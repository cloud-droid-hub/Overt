package com.example.overt;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.util.HashMap;
import java.util.Map;

import tee_info.TEEStatus;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "lxz_MainActivity";
    private DeviceInfoProvider deviceInfoProvider;
    private LinearLayout mainContainer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.e(TAG, "onCreate started");

        try {
            // Set the content view first
            setContentView(R.layout.activity_main);

            TEEStatus.get_tee_info();

            // Initialize main container
            mainContainer = findViewById(R.id.main);
            if (mainContainer == null) {
                Log.e(TAG, "Failed to find main container");
                return;
            }
            Log.e(TAG, "Main container found successfully");

            // 先测试基本显示
            testBasicDisplay();

            // 再测试卡片系统
            testCardSystem();

            // Test our TEE parsing function directly
            Log.e(TAG, "Testing TEE parsing function...");
            deviceInfoProvider = new DeviceInfoProvider(this, mainContainer);
            Log.e(TAG, "DeviceInfoProvider created successfully");
            
            // 添加一些调试信息
            if (deviceInfoProvider != null) {
                Log.e(TAG, "DeviceInfoProvider is not null");
                Log.e(TAG, "Main title: " + deviceInfoProvider.getMainTitle());
                Log.e(TAG, "Card container: " + (deviceInfoProvider.getCardContainer() != null ? "not null" : "null"));
            } else {
                Log.e(TAG, "DeviceInfoProvider is null");
            }

            Log.e(TAG, "onCreate completed successfully");

        } catch (Exception e) {
            Log.e(TAG, "Error in onCreate", e);
            Toast.makeText(this, "初始化失败: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    /**
     * 测试基本显示功能
     */
    private void testBasicDisplay() {
        try {
            Log.e(TAG, "Testing basic display...");
            
            // 添加一个简单的TextView
            TextView testView = new TextView(this);
            testView.setText("测试显示 - 如果你能看到这个文字，说明基本显示正常");
            testView.setTextSize(18);
            testView.setTextColor(0xFF000000); // 黑色
            testView.setPadding(16, 16, 16, 16);
            
            mainContainer.addView(testView);
            
            Log.e(TAG, "Basic display test completed");
            
        } catch (Exception e) {
            Log.e(TAG, "Error testing basic display", e);
            Toast.makeText(this, "基本显示测试失败: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    /**
     * 测试卡片系统是否正常工作
     */
    private void testCardSystem() {
        try {
            Log.e(TAG, "Testing card system...");
            
            // 创建测试数据
            Map<String, Map<String, String>> testInfo = new HashMap<>();
            testInfo.put("测试项目", new HashMap<String, String>() {{
                put("risk", "error");
                put("explain", "这是一个测试错误");
            }});
            testInfo.put("安全项目", new HashMap<String, String>() {{
                put("risk", "safe");
                put("explain", "这是安全的");
            }});

            // 创建卡片容器
            InfoCardContainer container = new InfoCardContainer(this, mainContainer, "测试标题");
            container.addCard("测试卡片", testInfo);
            container.show();
            
            Log.e(TAG, "Card system test completed successfully");
            
        } catch (Exception e) {
            Log.e(TAG, "Error testing card system", e);
            Toast.makeText(this, "卡片系统测试失败: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }
}