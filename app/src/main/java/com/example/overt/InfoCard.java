package com.example.overt;

import android.content.Context;
import android.graphics.drawable.GradientDrawable;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.cardview.widget.CardView;
import androidx.core.content.ContextCompat;
import java.util.Map;
import android.view.View;

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
    
    // 边框属性
    private int borderWidth = 1; // 边框宽度，单位dp
    private int lightBorderColor = 0xFF888888; // 浅色模式边框颜色
    private int darkBorderColor = 0xFF333333; // 夜间模式边框颜色
    
    // 字体颜色属性
    private int lightTextColor = 0xFF000000; // 浅色模式字体颜色（黑色）
    private int darkTextColor = 0xFFE0E0E0; // 夜间模式字体颜色（暗白色）

    /**
     * 构造函数（使用默认边框设置）
     * @param context 上下文
     * @param title 标题
     * @param info 信息数据
     */
    public InfoCard(Context context, String title, Map<String, Map<String, String>> info) {
        this.context = context;
        this.title = title;
        this.info = info;
        // 使用默认边框设置
        this.borderWidth = 1;
        this.lightBorderColor = 0xFF888888;
        this.darkBorderColor = 0xFF333333;
        initCardView();
    }

    /**
     * 构造函数（带边框设置）
     * @param context 上下文
     * @param title 标题
     * @param info 信息数据
     * @param borderWidth 边框宽度（dp）
     * @param lightBorderColor 浅色模式边框颜色
     * @param darkBorderColor 夜间模式边框颜色
     * @param lightTextColor 浅色模式字体颜色
     * @param darkTextColor 夜间模式字体颜色
     */
    public InfoCard(Context context, String title, Map<String, Map<String, String>> info, 
                   int borderWidth, int lightBorderColor, int darkBorderColor,
                   int lightTextColor, int darkTextColor) {
        this.context = context;
        this.title = title;
        this.info = info;
        this.borderWidth = borderWidth;
        this.lightBorderColor = lightBorderColor;
        this.darkBorderColor = darkBorderColor;
        this.lightTextColor = lightTextColor;
        this.darkTextColor = darkTextColor;
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
        
        // 设置边框
        setupBorder();
        
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
            // 使用字体颜色属性，自动适配夜间模式
            titleView.setTextColor(getTextColor());
        }
    }

    /**
     * 设置风险等级样式
     */
    private void setupRiskLevelStyle() {
        // 统一使用主题背景色，不随风险等级变化
        cardView.setCardBackgroundColor(getSurfaceColor());
    }

    /**
     * 设置边框
     */
    private void setupBorder() {
        // 创建动态边框drawable
        GradientDrawable borderDrawable = new GradientDrawable();
        borderDrawable.setShape(GradientDrawable.RECTANGLE);
        borderDrawable.setCornerRadius(dpToPx(4)); // 4dp圆角
        borderDrawable.setColor(android.graphics.Color.TRANSPARENT); // 透明背景
        borderDrawable.setStroke(dpToPx(borderWidth), getBorderColor()); // 使用getBorderColor()动态获取边框颜色
        
        // 应用到LinearLayout
        if (cardContent != null) {
            cardContent.setBackground(borderDrawable);
        }
    }

    /**
     * 设置边框宽度
     * @param width 边框宽度（dp）
     */
    public void setBorderWidth(int width) {
        this.borderWidth = width;
        setupBorder();
    }

    /**
     * 设置边框颜色（同时设置浅色和夜间模式颜色）
     * @param lightColor 浅色模式边框颜色
     * @param darkColor 夜间模式边框颜色
     */
    public void setBorderColor(int lightColor, int darkColor) {
        this.lightBorderColor = lightColor;
        this.darkBorderColor = darkColor;
        setupBorder();
    }

    /**
     * 设置边框颜色（使用相同颜色作为浅色和夜间模式）
     * @param color 边框颜色
     */
    public void setBorderColor(int color) {
        this.lightBorderColor = color;
        this.darkBorderColor = color;
        setupBorder();
    }

    /**
     * 获取浅色模式边框颜色
     * @return 浅色模式边框颜色
     */
    public int getLightBorderColor() {
        return lightBorderColor;
    }

    /**
     * 获取夜间模式边框颜色
     * @return 夜间模式边框颜色
     */
    public int getDarkBorderColor() {
        return darkBorderColor;
    }

    /**
     * 设置浅色模式边框颜色
     * @param color 浅色模式边框颜色
     */
    public void setLightBorderColor(int color) {
        this.lightBorderColor = color;
        setupBorder();
    }

    /**
     * 设置夜间模式边框颜色
     * @param color 夜间模式边框颜色
     */
    public void setDarkBorderColor(int color) {
        this.darkBorderColor = color;
        setupBorder();
    }

    /**
     * 获取浅色模式字体颜色
     * @return 浅色模式字体颜色
     */
    public int getLightTextColor() {
        return lightTextColor;
    }

    /**
     * 获取夜间模式字体颜色
     * @return 夜间模式字体颜色
     */
    public int getDarkTextColor() {
        return darkTextColor;
    }

    /**
     * 设置浅色模式字体颜色
     * @param color 浅色模式字体颜色
     */
    public void setLightTextColor(int color) {
        this.lightTextColor = color;
        refreshTextColors();
    }

    /**
     * 设置夜间模式字体颜色
     * @param color 夜间模式字体颜色
     */
    public void setDarkTextColor(int color) {
        this.darkTextColor = color;
        refreshTextColors();
    }

    /**
     * 设置字体颜色（同时设置浅色和夜间模式颜色）
     * @param lightColor 浅色模式字体颜色
     * @param darkColor 夜间模式字体颜色
     */
    public void setTextColor(int lightColor, int darkColor) {
        this.lightTextColor = lightColor;
        this.darkTextColor = darkColor;
        refreshTextColors();
    }

    /**
     * 设置字体颜色（使用相同颜色作为浅色和夜间模式）
     * @param color 字体颜色
     */
    public void setTextColor(int color) {
        this.lightTextColor = color;
        this.darkTextColor = color;
        refreshTextColors();
    }

    /**
     * 获取当前字体颜色（根据主题自动返回对应颜色）
     * @return 当前字体颜色
     */
    public int getTextColor() {
        if (isDarkMode()) {
            return darkTextColor;
        } else {
            return lightTextColor;
        }
    }

    /**
     * 刷新文字颜色（用于主题切换时重新应用文字颜色）
     */
    public void refreshTextColors() {
        setupTitle();
        refreshInfoItems();
    }

    /**
     * 获取边框宽度
     * @return 边框宽度（dp）
     */
    public int getBorderWidth() {
        return borderWidth;
    }

    /**
     * 获取边框颜色（支持夜间模式适配）
     * @return 边框颜色
     */
    public int getBorderColor() {
        // 根据夜间模式动态调整边框颜色
        if (isDarkMode()) {
            // 夜间模式使用夜间模式边框颜色
            return darkBorderColor;
        } else {
            // 浅色模式使用浅色模式边框颜色
            return lightBorderColor;
        }
    }

    /**
     * 刷新边框（用于主题切换时重新应用边框颜色）
     */
    public void refreshBorder() {
        setupBorder();
    }

    /**
     * dp转px
     * @param dp dp值
     * @return px值
     */
    private int dpToPx(int dp) {
        float density = context.getResources().getDisplayMetrics().density;
        return Math.round(dp * density);
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
                riskSymbol = "× "; // 红色叉号
            } else if ("warn".equals(risk)) {
                riskSymbol = "⚠ "; // 警告符号
            } else if ("safe".equals(risk)){
                riskSymbol = "√ "; // 绿色对勾
            }else{
                continue;
            }
            
            textView.setText(riskSymbol + entry.getKey() + ": " + risk + ": " + explain);
            textView.setTextSize(14);

            // 根据风险等级设置文字颜色，使用主题颜色适配夜间模式
            int textColor;
            if ("error".equals(risk)) {
                textColor = ContextCompat.getColor(context, R.color.risk_error);
            } else if ("warn".equals(risk)) {
                textColor = ContextCompat.getColor(context, R.color.risk_warn);
            } else {
                // 安全状态使用主题文字颜色，自动适配夜间模式
                textColor = getTextColor();
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
     * 刷新信息项的文字颜色
     */
    private void refreshInfoItems() {
        if (cardContent == null) return;

        for (int i = 0; i < cardContent.getChildCount(); i++) {
            View child = cardContent.getChildAt(i);
            if (child instanceof TextView) {
                TextView textView = (TextView) child;
                String text = textView.getText().toString();
                
                // 从文本中提取信息项名称
                String itemName = "";
                if (text.contains(": ")) {
                    String[] parts = text.split(": ");
                    if (parts.length > 0) {
                        // 移除风险符号前缀
                        String firstPart = parts[0];
                        if (firstPart.startsWith("× ") || firstPart.startsWith("⚠ ") || firstPart.startsWith("√ ")) {
                            itemName = firstPart.substring(2);
                        } else {
                            itemName = firstPart;
                        }
                    }
                }
                
                // 获取风险等级并设置颜色
                if (info.containsKey(itemName)) {
                    String risk = info.get(itemName).get("risk");
                    int textColor;
                    if ("error".equals(risk)) {
                        textColor = ContextCompat.getColor(context, R.color.risk_error);
                    } else if ("warn".equals(risk)) {
                        textColor = ContextCompat.getColor(context, R.color.risk_warn);
                    } else {
                        textColor = getTextColor();
                    }
                    textView.setTextColor(textColor);
                }
            }
        }
    }

    /**
     * 获取主题背景颜色（适配夜间模式）
     * @return 背景颜色
     */
    private int getSurfaceColor() {
        if (isDarkMode()) {
            return ContextCompat.getColor(context, R.color.dark_surface);
        } else {
            return ContextCompat.getColor(context, R.color.white);
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