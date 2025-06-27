package com.example.overt.device;

import static com.example.overt.device.system_info.get_system_info;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.SystemClock;
import android.util.Log;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.cardview.widget.CardView;

import com.example.overt.R;
import com.example.overt.tee_info.TEEStatus;

import java.util.Calendar;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.text.SimpleDateFormat;

public class DeviceInfoProvider {
    private static final String TAG = "lxz_" + DeviceInfoProvider.class.getSimpleName();
    private LinearLayout mainContainer;
    private Context context;

    native public Map<String, String> get_root_file_info();
    native public Map<String, String> get_package_info();
    native public Map<String, String> get_mounts_info();
    native public Map<String, String> get_system_prop_info();
    native public Map<String, String> get_class_loader_info();
    native public Map<String, String> get_class_info();
    native public Map<String, String> get_time_info();
    native public Map<String, String> get_linker_info();

    public DeviceInfoProvider(Context context, LinearLayout mainContainer) {
        this.context = context;
        this.mainContainer = mainContainer;
        Log.d(TAG, "DeviceInfoProvider initialized with context and container");

        this.addInfoCard("system_info", get_system_info(context));

        this.addInfoCard("package_info", get_package_info());
        this.addInfoCard("time_info", get_time_info());
        this.addInfoCard("root_file_info", get_root_file_info());
        this.addInfoCard("mounts_info", get_mounts_info());
        this.addInfoCard("system_prop_info", get_system_prop_info());
        this.addInfoCard("tee_info", TEEStatus.getInstance().get_tee_info());
        this.addInfoCard("class_loader_info", get_class_loader_info());
        this.addInfoCard("class_info", get_class_info());
        this.addInfoCard("linker_info", get_linker_info());

    }

    public void addInfoCard(String title, Map<String, String> info) {
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
            for (Map.Entry<String, String> entry : info.entrySet()) {
                TextView textView = new TextView(context);
                textView.setText(entry.getKey() + ": " + entry.getValue());
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