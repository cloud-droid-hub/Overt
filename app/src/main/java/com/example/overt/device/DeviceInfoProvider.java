package com.example.overt.device;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.cardview.widget.CardView;

import com.example.overt.R;
import com.example.overt.system_info.system_info;
import com.example.overt.tee_info.TEEStatus;

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

        device_info.put("tee_info", TEEStatus.get_tee_info());
        device_info.put("system_info", system_info.get_system_info(context));

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
                textView.setTextColor(context.getResources().getColor(android.R.color.black));
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

} 