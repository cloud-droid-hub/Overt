package com.example.overt;

import android.os.BatteryManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.widget.NestedScrollView;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Map;

/**
 * 主活动类 - Overt安全检测工具的主界面
 * 
 * 功能说明：
 * 1. 作为应用程序的主入口点，负责初始化用户界面
 * 2. 管理信息卡片容器，显示各种安全检测结果
 * 3. 处理Native层的回调，实现数据更新和UI刷新
 * 4. 提供线程安全的UI更新机制
 * 
 * 设计特点：
 * - 采用静态方法回调机制，允许Native层直接调用
 * - 使用Handler确保UI更新在主线程执行
 * - 支持动态卡片更新，实时显示检测结果
 * - 具备异常处理和状态管理能力
 */
public class MainActivity extends AppCompatActivity {
    private static final String TAG = "overt_" + MainActivity.class.getSimpleName();
    
    // UI组件
    private InfoCardContainer cardContainer;    // 信息卡片容器，管理所有检测结果卡片
    private TextView titleBar;                  // 标题栏，显示应用名称
    private FrameLayout cardContainerLayout;    // 卡片容器布局，承载可滚动的内容
    
    // 线程管理
    static private Handler mainHandler;         // 主线程Handler，用于UI更新
    
    // 静态引用，用于native层调用静态方法时访问当前Activity实例
    // 注意：这是必要的，因为Native层无法直接获取Activity实例
    private static MainActivity currentInstance = null;

    /**
     * Activity创建时的回调方法
     * 
     * 执行流程：
     * 1. 调用父类onCreate方法完成基础初始化
     * 2. 设置静态实例引用，供Native层回调使用
     * 3. 加载主界面布局文件
     * 4. 初始化UI组件和Handler
     * 5. 创建信息卡片容器并添加到布局中
     * 
     * 注意事项：
     * - 静态实例引用必须在Native库加载前设置
     * - Handler必须在主线程中创建
     * - 卡片容器初始化为空，等待Native层数据更新
     * 
     * @param savedInstanceState 保存的实例状态，用于Activity重建
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 设置当前实例的静态引用，供Native层回调使用
        // 这个引用必须在Native库加载前设置，否则回调会失败
        currentInstance = this;
        
        // 设置主界面布局
        setContentView(R.layout.activity_main);
        
        // 获取并初始化UI组件
        titleBar = findViewById(R.id.title_bar);              // 标题栏组件
        cardContainerLayout = findViewById(R.id.card_container); // 卡片容器布局
        
        // 初始化主线程Handler，用于安全的UI更新
        // 确保所有UI操作都在主线程中执行
        mainHandler = new Handler(Looper.getMainLooper());
        
        // 创建信息卡片容器，初始数据为空
        // 卡片内容将由Native层通过回调动态更新
        cardContainer = new InfoCardContainer(this, null);
        
        // 将卡片容器的滚动视图添加到主布局中
        // 这样用户就可以滚动查看所有的检测结果
        NestedScrollView scrollView = cardContainer.getContainerView();
        cardContainerLayout.addView(scrollView);
    }
    
    /**
     * 静态回调方法 - 由Native层调用
     * 
     * 功能说明：
     * 1. 接收Native层发送的设备信息更新通知
     * 2. 处理线程同步问题，确保UI更新在主线程执行
     * 3. 提供超时机制，避免无限等待
     * 4. 安全地更新UI，防止空指针异常
     * 
     * 调用时机：
     * - Native层完成某项安全检测后
     * - 检测结果发生变化时
     * - 定期更新检测状态时
     * 
     * 线程安全：
     * - 此方法在Native线程中调用
     * - 通过Handler切换到主线程更新UI
     * - 使用超时机制避免死锁
     * 
     * @param title 卡片标题，标识检测类型（如"root_state_info"）
     * @param newCardInfo 新的卡片信息，JSON格式的检测结果
     */
    public static void onCardInfoUpdated(String title, String newCardInfo) {
        Log.d(TAG, "Received device info update notification from native (static method)");

        // 等待机制：Native层初始化可能比Activity创建更早
        // 如果Activity还未创建完成，需要等待一段时间
        final long deadline = System.currentTimeMillis() + 1000L; // 1秒超时
        while ((currentInstance == null || mainHandler == null) && System.currentTimeMillis() < deadline) {
            try {
                Thread.sleep(30);            // 每30ms检查一次，避免CPU占用过高
            } catch (InterruptedException ignore) {
                // 线程被中断，直接返回
                Thread.currentThread().interrupt();
                Log.e(TAG, "Cannot update UI - currentInstance: " + currentInstance);
                Log.e(TAG, "Cannot update UI - mainHandler: " + mainHandler);
                return;
            }
        }

        // 检查等待结果
        if (currentInstance == null || mainHandler == null) {
            Log.e(TAG, "Failed to get Activity instance or Handler after timeout");
            return;
        }

        // 保存当前实例的引用，避免在post执行时被清空
        final MainActivity instance = currentInstance;

        // 切换到主线程更新UI
        // 使用Handler确保UI更新操作在主线程中执行
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                instance.updateUIWithNewCardInfo(title, newCardInfo);
            }
        });
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
                // JSON解析失败，抛出运行时异常
                // 这通常表示Native层传递的数据格式有问题
                Log.e(TAG, "Failed to parse JSON data: " + newCardInfo, e);
                throw new RuntimeException("JSON parsing failed", e);
            }
        } else {
            Log.w(TAG, "Card container is null, cannot update UI");
        }
    }
    
    /**
     * Activity销毁时的回调方法
     * 
     * 功能说明：
     * 1. 调用父类onDestroy完成基础清理
     * 2. 清除静态实例引用，防止内存泄漏
     * 3. 记录销毁日志
     * 
     * 注意事项：
     * - 必须清除静态引用，否则可能导致内存泄漏
     * - 只有在当前实例是静态引用时才清除
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        // 清除静态引用，防止内存泄漏
        // 只有在当前实例是静态引用时才清除
        if (currentInstance == this) {
            currentInstance = null;
            Log.d(TAG, "Static instance reference cleared");
        }
        
        Log.d(TAG, "Device info monitor stopped");
    }

    @Override
    protected void onResume() {
        super.onResume();
        ServerStarter.start(this);
    }
}