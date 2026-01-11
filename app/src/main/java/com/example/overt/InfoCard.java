package com.example.overt;

import android.content.Context;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.text.LineBreaker;
import android.text.Layout;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.cardview.widget.CardView;
import androidx.core.content.ContextCompat;
import java.util.Map;
import android.view.View;
import org.json.JSONObject;
import java.util.Iterator;

/**
 * 信息卡片类 - 安全检测结果的可视化展示组件
 * 
 * 功能说明：
 * 1. 负责单个检测结果卡片的显示和样式设置
 * 2. 支持动态主题切换（浅色/夜间模式）
 * 3. 根据风险等级显示不同的颜色和符号
 * 4. 提供灵活的样式定制功能
 * 5. 自动适配不同屏幕尺寸和密度
 * 
 * 设计特点：
 * - 采用Material Design卡片设计风格
 * - 支持风险等级可视化（安全√、警告⚠、错误×）
 * - 自动适配系统主题（浅色/夜间模式）
 * - 提供丰富的样式定制选项
 * - 支持动态内容更新
 * 
 * 使用场景：
 * - 显示Root检测结果
 * - 展示调试工具检测状态
 * - 呈现系统完整性检查结果
 * - 展示网络和进程检测信息
 * 
 * 数据格式：
 * 接收JSONObject格式的数据，每个检测项包含：
 * - risk: 风险等级（"safe"、"warn"、"error"）
 * - explain: 检测结果说明
 */
public class InfoCard {
    // 基础组件
    private Context context;                    // Android上下文，用于资源访问
    private String title;                       // 卡片标题，标识检测类型
    private JSONObject info;                    // 卡片数据，JSON格式的检测结果
    private CardView cardView;                  // 卡片视图容器
    private LinearLayout cardContent;           // 卡片内容布局
    private TextView titleView;                 // 标题文本视图
    
    // 边框样式属性
    private int borderWidth = 1;                // 边框宽度，单位dp
    private int lightBorderColor = 0xFF888888;  // 浅色模式边框颜色（灰色）
    private int darkBorderColor = 0xFF333333;   // 夜间模式边框颜色（深灰色）
    
    // 文字颜色属性
    private int lightTextColor = 0xFF000000;    // 浅色模式字体颜色（黑色）
    private int darkTextColor = 0xFFE0E0E0;     // 夜间模式字体颜色（浅白色）

    /**
     * 构造函数（使用默认样式设置）
     * 
     * 功能说明：
     * 1. 创建信息卡片实例，使用默认的样式配置
     * 2. 自动初始化卡片视图和内容
     * 3. 应用默认的边框和颜色设置
     * 
     * 使用场景：
     * - 快速创建标准样式的卡片
     * - 不需要自定义样式的场景
     * - 批量创建多个卡片时
     * 
     * @param context Android上下文，用于资源访问和视图创建
     * @param title 卡片标题，用于标识检测类型（如"Root检测"）
     * @param info 检测结果数据，JSON格式，包含各项检测结果
     */
    public InfoCard(Context context, String title, JSONObject info) {
        this.context = context;
        this.title = title;
        this.info = info;
        
        // 使用默认样式设置
        this.borderWidth = 1;                    // 1dp边框宽度
        this.lightBorderColor = 0xFF888888;      // 浅色模式：灰色边框
        this.darkBorderColor = 0xFF333333;       // 夜间模式：深灰色边框
        
        // 初始化卡片视图
        initCardView();
    }

    /**
     * 构造函数（支持自定义样式设置）
     * 
     * 功能说明：
     * 1. 创建信息卡片实例，支持完全自定义的样式配置
     * 2. 允许指定边框宽度、颜色和文字颜色
     * 3. 支持浅色和夜间模式的不同样式设置
     * 
     * 使用场景：
     * - 需要特殊样式的卡片
     * - 主题定制需求
     * - 品牌色彩要求
     * 
     * @param context Android上下文，用于资源访问和视图创建
     * @param title 卡片标题，用于标识检测类型
     * @param info 检测结果数据，JSON格式
     * @param borderWidth 边框宽度，单位dp（建议1-3dp）
     * @param lightBorderColor 浅色模式边框颜色，ARGB格式
     * @param darkBorderColor 夜间模式边框颜色，ARGB格式
     * @param lightTextColor 浅色模式文字颜色，ARGB格式
     * @param darkTextColor 夜间模式文字颜色，ARGB格式
     */
    public InfoCard(Context context, String title, JSONObject info, 
                   int borderWidth, int lightBorderColor, int darkBorderColor,
                   int lightTextColor, int darkTextColor) {
        this.context = context;
        this.title = title;
        this.info = info;
        
        // 应用自定义样式设置
        this.borderWidth = borderWidth;
        this.lightBorderColor = lightBorderColor;
        this.darkBorderColor = darkBorderColor;
        this.lightTextColor = lightTextColor;
        this.darkTextColor = darkTextColor;
        
        // 初始化卡片视图
        initCardView();
    }

    /**
     * 初始化卡片视图
     * 
     * 功能说明：
     * 1. 加载卡片布局文件，创建基础视图结构
     * 2. 获取各个子组件的引用
     * 3. 按顺序执行样式设置和内容填充
     * 4. 完成卡片的完整初始化
     * 
     * 执行流程：
     * 1. 加载布局 → 2. 获取组件引用 → 3. 设置标题 → 4. 应用样式 → 5. 填充内容
     * 
     * 注意事项：
     * - 必须在构造函数中调用
     * - 依赖context和info数据
     * - 会触发所有子组件的初始化
     */
    private void initCardView() {
        // 1. 加载卡片布局文件
        // 使用LayoutInflater从XML布局文件创建视图层次结构
        cardView = (CardView) LayoutInflater.from(context).inflate(R.layout.item_info_card, null, false);
        
        // 2. 获取子组件引用
        cardContent = cardView.findViewById(R.id.card_content);  // 内容容器
        titleView = cardView.findViewById(R.id.card_title);      // 标题文本视图

        // 3. 设置卡片标题
        setupTitle();
        
        // 4. 设置风险等级相关的样式
        setupRiskLevelStyle();
        
        // 5. 设置卡片边框样式
        setupBorder();
        
        // 6. 添加检测结果信息项
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
        if (cardContent == null || info == null) return;

        try {
            Iterator<String> keys = info.keys();
            while (keys.hasNext()) {
                String key = keys.next();
                Object value = info.get(key);
                
                if (value instanceof JSONObject) {
                    JSONObject itemData = (JSONObject) value;
                    TextView textView = new TextView(context);

                    String risk = itemData.optString("risk", "");
                    String explain = itemData.optString("explain", "");
                    
                    // 格式化显示文本，添加风险等级符号
                    String riskSymbol = "";
                    if ("error".equals(risk)) {
                        riskSymbol = "× "; // 红色叉号
                    } else if ("warn".equals(risk)) {
                        riskSymbol = "⚠ "; // 警告符号
                    } else if ("safe".equals(risk)){
                        riskSymbol = "√ "; // 绿色对勾
                    } else {
                        continue;
                    }

                    // breakByChar的效果，每个字符都可以作为换行点 行一定会“塞满才换”
                    textView.setText(breakByChar(riskSymbol + key + ": " + risk + ": " + explain));
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
        } catch (Exception e) {
            android.util.Log.e("InfoCard", "Error adding info items: " + e.getMessage());
        }
    }

    /**
     * 刷新信息项的文字颜色
     */
    private void refreshInfoItems() {
        if (cardContent == null || info == null) return;

        try {
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
                    if (info.has(itemName)) {
                        JSONObject itemData = info.getJSONObject(itemName);
                        String risk = itemData.optString("risk", "");
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
        } catch (Exception e) {
            android.util.Log.e("InfoCard", "Error refreshing info items: " + e.getMessage());
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
     * @return 信息数据JSONObject
     */
    public JSONObject getInfo() {
        return info;
    }

    private static String breakByChar(String src) {
        StringBuilder sb = new StringBuilder(src.length() * 2);
        for (int i = 0; i < src.length(); i++) {
            sb.append(src.charAt(i));
            sb.append('\u200B'); // 零宽断点
        }
        return sb.toString();
    }

} 