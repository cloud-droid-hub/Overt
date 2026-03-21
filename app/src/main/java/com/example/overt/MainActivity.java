package com.example.overt;

import android.os.Bundle;
import android.util.Log;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.widget.NestedScrollView;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * 主活动类 - Overt安全检测工具的主界面
 *
 * 功能说明：
 * 1. 作为应用程序的主入口点，负责初始化用户界面
 * 2. 管理信息卡片容器，显示各种安全检测结果
 * 3. 订阅NativeUpdateBus接收Native层检测结果
 * 4. 在主线程更新UI并处理异常数据
 *
 * 设计特点：
 * - 使用消息总线解耦Native回调与Activity生命周期
 * - 支持动态卡片更新，实时显示检测结果
 * - 具备异常处理和状态管理能力
 */
public class MainActivity extends AppCompatActivity {
    private static final String TAG = "overt_" + MainActivity.class.getSimpleName();
    
    // UI组件
    private InfoCardContainer cardContainer;    // 信息卡片容器，管理所有检测结果卡片
    private TextView titleBar;                  // 标题栏，显示应用名称
    private FrameLayout cardContainerLayout;    // 卡片容器布局，承载可滚动的内容
    
    private final NativeUpdateBus.Listener nativeUpdateListener = new NativeUpdateBus.Listener() {
        @Override
        public void onCardInfoUpdated(String title, String newCardInfo) {
            updateUIWithNewCardInfo(title, newCardInfo);
        }
    };

    /**
     * Activity创建时的回调方法
     *
     * 执行流程：
     * 1. 调用父类onCreate方法完成基础初始化
     * 2. 加载主界面布局文件
     * 3. 初始化UI组件
     * 4. 创建信息卡片容器并添加到布局中
     *
     * 注意事项：
     * - 卡片容器初始化为空，等待Native层数据更新
     *
     * @param savedInstanceState 保存的实例状态，用于Activity重建
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 设置主界面布局
        setContentView(R.layout.activity_main);
        
        // 获取并初始化UI组件
        titleBar = findViewById(R.id.title_bar);              // 标题栏组件
        cardContainerLayout = findViewById(R.id.card_container); // 卡片容器布局
        
        // 创建信息卡片容器，初始数据为空
        // 卡片内容将由Native层通过回调动态更新
        cardContainer = new InfoCardContainer(this, null);
        
        // 将卡片容器的滚动视图添加到主布局中
        // 这样用户就可以滚动查看所有的检测结果
        NestedScrollView scrollView = cardContainer.getContainerView();
        cardContainerLayout.addView(scrollView);
    }
    
    /**
     * 更新UI中的卡片信息
     * 
     * 功能说明：
     * 1. 解析Native层传来的JSON格式数据
     * 2. 更新或创建对应的信息卡片
     * 3. 处理JSON解析异常
     * 
     * 执行环境：
     * - 在主线程中执行，确保UI操作安全
     * - 由onCardInfoUpdated方法调用
     * 
     * @param title 卡片标题，用于标识卡片类型
     * @param newCardInfo JSON格式的卡片数据
     */
    private void updateUIWithNewCardInfo(String title, String newCardInfo) {
        if (cardContainer != null) {
            try {
                // 解析JSON格式的卡片数据
                JSONObject jsonObject = new JSONObject(newCardInfo);
                
                // 更新或创建卡片
                // createIfNotExists=true 表示如果卡片不存在则创建新卡片
                cardContainer.updateCard(title, jsonObject, true);
            } catch (JSONException e) {
                // JSON解析失败，通常表示Native层传递的数据格式有问题
                Log.e(TAG, "Failed to parse JSON data: " + newCardInfo, e);
                // 避免因为单条数据异常导致整个应用崩溃
                // 仅记录错误并跳过本次更新
            }
        } else {
            Log.w(TAG, "Card container is null, cannot update UI");
        }
    }
    
    @Override
    protected void onStart() {
        super.onStart();
        NativeUpdateBus.subscribe(nativeUpdateListener);
    }

    @Override
    protected void onStop() {
        NativeUpdateBus.unsubscribe(nativeUpdateListener);
        super.onStop();
    }

    @Override
    protected void onResume() {
        super.onResume();
        ServerStarter.start(this);
    }
}
