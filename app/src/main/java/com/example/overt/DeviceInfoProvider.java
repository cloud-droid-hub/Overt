package com.example.overt;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.cardview.widget.CardView;
import java.util.Map;

public class DeviceInfoProvider {
    private static final String TAG = "lxz_" + DeviceInfoProvider.class.getSimpleName();
    private LinearLayout mainContainer;
    private Context context;
    native public Map<String, Map<String, Map<String, String>>> get_device_info();

    public DeviceInfoProvider(Context context, LinearLayout mainContainer) {
        this.context = context;
        this.mainContainer = mainContainer;
        Log.d(TAG, "DeviceInfoProvider initialized with context and container");

        Map<String, Map<String, Map<String, String>>> device_info = get_device_info();

        // 添加信息项
        for (Map.Entry<String, Map<String, Map<String, String>>> entry : device_info.entrySet()) {
            this.addInfoCard(entry.getKey(), entry.getValue());
        }

    }

    @SuppressLint("SetTextI18n")
    public void addInfoCard(String title, Map<String, Map<String, String>> info) {
        try {
            if (context == null || mainContainer == null) {
                Log.e(TAG, "Context or mainContainer is null");
                return;
            }

            // 加载卡片布局
            CardView cardView = (CardView) LayoutInflater.from(context).inflate(R.layout.item_info_card, mainContainer, false);
            if (cardView == null) {
                Log.e(TAG, "Failed to inflate card layout");
                return;
            }

            LinearLayout cardContent = cardView.findViewById(R.id.card_content);
            TextView titleView = cardView.findViewById(R.id.card_title);

            if (cardContent == null || titleView == null) {
                Log.e(TAG, "Failed to find required views in card layout");
                return;
            }

            // 设置标题
            titleView.setText(title);

            // 添加信息项
            for (Map.Entry<String, Map<String, String>> entry : info.entrySet()) {
                TextView textView = new TextView(context);

                String risk = entry.getValue().get("risk");
                String explain = entry.getValue().get("explain");
                textView.setText(entry.getKey() + ": " + risk + ": " + explain);

                textView.setTextSize(14);

                // 根据当前主题模式设置合适的文字颜色
                int textColor = isDarkMode(context) ? 0xFFFFFFFF : 0xFF000000;
                textView.setTextColor(textColor);

                // 设置间距
                textView.setPadding(0, 0, 0, 8);
                cardContent.addView(textView);
            }

            // 将卡片添加到主容器
            mainContainer.addView(cardView);
            Log.d(TAG, "Successfully added info card: " + title);
        } catch (Exception e) {
            Log.e(TAG, "Error adding info card: " + title, e);
        }
    }

    /**
     * 检测当前是否为深色模式
     * @param context 上下文
     * @return true表示深色模式，false表示浅色模式
     */
    private boolean isDarkMode(Context context) {
        TypedValue typedValue = new TypedValue();
        context.getTheme().resolveAttribute(android.R.attr.windowBackground, typedValue, true);
        int backgroundColor = typedValue.data;
        
        // 计算背景色的亮度
        int red = (backgroundColor >> 16) & 0xFF;
        int green = (backgroundColor >> 8) & 0xFF;
        int blue = backgroundColor & 0xFF;
        
        // 使用相对亮度公式
        double luminance = (0.299 * red + 0.587 * green + 0.114 * blue) / 255.0;
        
        return luminance < 0.5; // 亮度小于0.5认为是深色模式
    }

} 