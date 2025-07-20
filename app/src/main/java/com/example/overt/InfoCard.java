package com.example.overt;

import android.content.Context;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.cardview.widget.CardView;
import androidx.core.content.ContextCompat;
import java.util.Map;

/**
 * 信息卡片类
 * 负责单个卡片的显示和样式设置
 */
public class InfoCard {
    private Context context;
    private String title;
    private Map<String, Map<String, String>> info;
    private CardView cardView;
    private LinearLayout cardContent;
    private TextView titleView;

    /**
     * 构造函数
     * @param context 上下文
     * @param title 卡片标题
     * @param info 卡片信息数据
     */
    public InfoCard(Context context, String title, Map<String, Map<String, String>> info) {
        this.context = context;
        this.title = title;
        this.info = info;
        initCardView();
    }

    /**
     * 初始化卡片视图
     */
    private void initCardView() {
        // 加载卡片布局
        cardView = (CardView) LayoutInflater.from(context).inflate(R.layout.item_info_card, null, false);
        cardContent = cardView.findViewById(R.id.card_content);
        titleView = cardView.findViewById(R.id.card_title);

        // 设置标题
        setupTitle();
        
        // 设置风险等级样式
        setupRiskLevelStyle();
        
        // 添加信息项
        addInfoItems();
    }

    /**
     * 设置标题
     */
    private void setupTitle() {
        if (titleView != null) {
            titleView.setText(title);
            titleView.setTextSize(18); // 标题文字比普通文字大
        }
    }

    /**
     * 设置风险等级样式
     */
    private void setupRiskLevelStyle() {
        // 统计风险等级
        boolean hasError = false;
        boolean hasWarn = false;
        for (Map.Entry<String, Map<String, String>> entry : info.entrySet()) {
            String risk = entry.getValue().get("risk");
            if ("error".equals(risk)) {
                hasError = true;
            } else if ("warn".equals(risk)) {
                hasWarn = true;
            }
        }

        // 根据最高风险等级设置卡片背景色
        if (hasError) {
            // 有error等级，设置红色背景
            cardView.setCardBackgroundColor(ContextCompat.getColor(context, R.color.risk_error_bg));
        } else if (hasWarn) {
            // 有warn等级，设置黄色背景
            cardView.setCardBackgroundColor(ContextCompat.getColor(context, R.color.risk_warn_bg));
        }
    }

    /**
     * 添加信息项
     */
    private void addInfoItems() {
        if (cardContent == null) return;

        for (Map.Entry<String, Map<String, String>> entry : info.entrySet()) {
            TextView textView = new TextView(context);

            String risk = entry.getValue().get("risk");
            String explain = entry.getValue().get("explain");
            
            // 格式化显示文本，添加风险等级符号
            String riskSymbol = "";
            if ("error".equals(risk)) {
                riskSymbol = "✘ "; // 红色叉号
            } else if ("warn".equals(risk)) {
                riskSymbol = "⚠ "; // 警告符号
            } else {
                riskSymbol = "✔ "; // 绿色对勾
            }
            
            textView.setText(riskSymbol + entry.getKey() + ": " + explain);
            textView.setTextSize(14);

            // 根据风险等级设置文字颜色
            int textColor;
            if ("error".equals(risk)) {
                textColor = ContextCompat.getColor(context, R.color.risk_error);
            } else if ("warn".equals(risk)) {
                textColor = ContextCompat.getColor(context, R.color.risk_warn);
            } else {
                textColor = isDarkMode() ? ContextCompat.getColor(context, R.color.white) : ContextCompat.getColor(context, R.color.black);
            }
            textView.setTextColor(textColor);

            // 设置间距和样式
            textView.setPadding(8, 2, 8, 2);
            textView.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ));
            
            cardContent.addView(textView);
        }
    }

    /**
     * 检测当前是否为深色模式
     * @return true表示深色模式，false表示浅色模式
     */
    private boolean isDarkMode() {
        android.util.TypedValue typedValue = new android.util.TypedValue();
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

    /**
     * 显示卡片
     * @return 卡片的CardView对象
     */
    public CardView show() {
        return cardView;
    }

    /**
     * 获取卡片视图
     * @return CardView对象
     */
    public CardView getCardView() {
        return cardView;
    }

    /**
     * 获取标题
     * @return 卡片标题
     */
    public String getTitle() {
        return title;
    }

    /**
     * 获取信息数据
     * @return 信息数据Map
     */
    public Map<String, Map<String, String>> getInfo() {
        return info;
    }
} 