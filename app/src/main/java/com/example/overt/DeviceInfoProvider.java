package com.example.overt;

import android.content.Context;
import android.util.Log;
import android.widget.LinearLayout;
import java.util.Map;

/**
 * 设备信息提供者类
 * 负责获取设备信息并使用卡片容器显示
 */
public class DeviceInfoProvider {
    private static final String TAG = "lxz_" + DeviceInfoProvider.class.getSimpleName();
    private LinearLayout mainContainer;
    private Context context;
    private InfoCardContainer cardContainer;
    private String mainTitle;
    
    native public Map<String, Map<String, Map<String, String>>> get_device_info();

    /**
     * 构造函数
     * @param context 上下文
     * @param mainContainer 主容器
     */
    public DeviceInfoProvider(Context context, LinearLayout mainContainer) {
        this(context, mainContainer, "设备信息检测");
    }

    /**
     * 构造函数（带总标题）
     * @param context 上下文
     * @param mainContainer 主容器
     * @param mainTitle 总标题
     */
    public DeviceInfoProvider(Context context, LinearLayout mainContainer, String mainTitle) {
        this.context = context;
        this.mainContainer = mainContainer;
        this.mainTitle = mainTitle != null ? mainTitle : "设备信息检测";
        
        // 创建卡片容器，设置紧凑的间距和总标题
        this.cardContainer = new InfoCardContainer(context, mainContainer, this.mainTitle, 0, 1, 4, 4);
        
        Log.d(TAG, "DeviceInfoProvider initialized with context and container");

        // 获取设备信息并显示
        showDeviceInfo();
    }

    /**
     * 显示设备信息
     */
    public void showDeviceInfo() {
        try {
            // 获取设备信息
            Map<String, Map<String, Map<String, String>>> device_info = get_device_info();
            
            // 清空容器
            cardContainer.clear();
            
            // 添加所有卡片
            cardContainer.addCards(device_info);
            
            // 显示所有卡片
            cardContainer.show();
            
            Log.d(TAG, "Successfully displayed device info with " + cardContainer.getCardCount() + " cards");
            
        } catch (Exception e) {
            Log.e(TAG, "Error displaying device info", e);
        }
    }

    /**
     * 设置总标题
     * @param mainTitle 总标题
     */
    public void setMainTitle(String mainTitle) {
        this.mainTitle = mainTitle;
        if (cardContainer != null) {
            cardContainer.setMainTitle(mainTitle);
        }
    }

    /**
     * 获取总标题
     * @return 总标题
     */
    public String getMainTitle() {
        return mainTitle;
    }

    /**
     * 设置卡片间距
     * @param top 顶部间距
     * @param bottom 底部间距
     * @param start 左侧间距
     * @param end 右侧间距
     */
    public void setCardMargins(int top, int bottom, int start, int end) {
        if (cardContainer != null) {
            cardContainer.setCardMargins(top, bottom, start, end);
        }
    }

    /**
     * 设置卡片间距（统一设置）
     * @param margin 统一间距值
     */
    public void setCardMargins(int margin) {
        if (cardContainer != null) {
            cardContainer.setCardMargins(margin);
        }
    }

    /**
     * 刷新显示
     */
    public void refresh() {
        showDeviceInfo();
    }

    /**
     * 获取卡片容器
     * @return InfoCardContainer对象
     */
    public InfoCardContainer getCardContainer() {
        return cardContainer;
    }

    /**
     * 获取主容器
     * @return LinearLayout主容器
     */
    public LinearLayout getMainContainer() {
        return mainContainer;
    }

    /**
     * 获取上下文
     * @return Context对象
     */
    public Context getContext() {
        return context;
    }
} 