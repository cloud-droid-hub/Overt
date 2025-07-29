package com.example.overt;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.cardview.widget.CardView;
import androidx.core.content.ContextCompat;
import androidx.core.widget.NestedScrollView;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * 信息卡片容器类
 * 只负责创建可滚动的卡片容器，不包含标题
 */
public class InfoCardContainer {
    private static final String TAG = "InfoCardContainer";
    
    private Context context;
    private NestedScrollView scrollView;
    private LinearLayout container;
    private List<InfoCard> cards;
    
    // 卡片间距
    private int cardMarginTop = 4;
    private int cardMarginBottom = 4;
    private int cardMarginStart = 4;
    private int cardMarginEnd = 4;

    /**
     * 构造函数 - 只创建卡片容器
     * @param context 上下文
     * @param cardData 卡片数据 Map<String, Map<String, Map<String, String>>>
     */
    public InfoCardContainer(Context context, Map<String, Map<String, Map<String, String>>> cardData) {
        this.context = context;
        this.cards = new ArrayList<>();
        
        // 创建滚动容器
        createContainer();
        
        // 直接添加所有卡片数据
        addAllCards(cardData);
        
        // 显示所有卡片
        showAllCards();
    }

    /**
     * 创建滚动容器
     */
    private void createContainer() {
        // 创建ScrollView作为根容器
        scrollView = new NestedScrollView(context);
        scrollView.setLayoutParams(new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.MATCH_PARENT
        ));
        scrollView.setBackgroundColor(getBackgroundColor());
        scrollView.setFillViewport(true); // 确保内容填充整个视口
        scrollView.setOverScrollMode(android.view.View.OVER_SCROLL_ALWAYS); // 允许过度滚动
        
        // 创建LinearLayout作为内容容器
        container = new LinearLayout(context);
        container.setOrientation(LinearLayout.VERTICAL);
        // 关键：使用WRAP_CONTENT确保容器能够根据内容扩展
        LinearLayout.LayoutParams containerParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        container.setLayoutParams(containerParams);
        container.setPadding(16, 16, 16, 16);
        
        // 将LinearLayout添加到ScrollView
        scrollView.addView(container);
        
        Log.d(TAG, "Container created with ScrollView and LinearLayout");
    }

    /**
     * 获取背景颜色（适配夜间模式）
     * @return 背景颜色
     */
    private int getBackgroundColor() {
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
     * 添加所有卡片数据
     * @param cardData 卡片数据
     */
    private void addAllCards(Map<String, Map<String, Map<String, String>>> cardData) {
        if (cardData == null) return;
        
        for (Map.Entry<String, Map<String, Map<String, String>>> entry : cardData.entrySet()) {
            String title = entry.getKey();
            Map<String, Map<String, String>> info = entry.getValue();
            
            InfoCard card = new InfoCard(context, title, info);
            cards.add(card);
        }
    }

    /**
     * 显示所有卡片
     */
    private void showAllCards() {
        if (container == null) return;
        
        int totalHeight = 0;
        for (InfoCard card : cards) {
            CardView cardView = card.show();
            
            // 设置卡片间距
            LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            );
            cardParams.setMargins(cardMarginStart, cardMarginTop, cardMarginEnd, cardMarginBottom);
            cardView.setLayoutParams(cardParams);
            
            // 添加卡片到容器
            container.addView(cardView);
            
            // 计算预估高度
            totalHeight += 100; // 预估每个卡片高度
            Log.d(TAG, "Added card: " + card.getTitle() + ", estimated height: " + totalHeight);
        }
        
        // 添加调试信息
        Log.d(TAG, "Added " + cards.size() + " cards to container, total estimated height: " + totalHeight);
    }

    /**
     * 获取容器View - 供Activity绑定到card_container
     * @return 容器View (ScrollView)
     */
    public NestedScrollView getContainerView() {
        return scrollView;
    }

    /**
     * 刷新容器（主题切换时使用）
     */
    public void refresh() {
        if (scrollView == null) return;
        
        // 刷新背景色
        scrollView.setBackgroundColor(getBackgroundColor());
        
        // 刷新所有卡片
        for (InfoCard card : cards) {
            card.refreshBorder();
            card.refreshTextColors();
        }
    }

    /**
     * 获取卡片数量
     * @return 卡片数量
     */
    public int getCardCount() {
        return cards.size();
    }

    /**
     * 获取指定索引的卡片
     * @param index 卡片索引
     * @return InfoCard对象
     */
    public InfoCard getCard(int index) {
        if (index >= 0 && index < cards.size()) {
            return cards.get(index);
        }
        return null;
    }

    /**
     * 获取所有卡片
     * @return 卡片列表
     */
    public List<InfoCard> getAllCards() {
        return new ArrayList<>(cards);
    }
    
    /**
     * 更新数据并刷新UI
     * @param newCardData 新的卡片数据
     */
    public void updateData(Map<String, Map<String, Map<String, String>>> newCardData) {
        if (newCardData == null || container == null) return;
        
        Log.d(TAG, "Updating data with " + newCardData.size() + " categories");
        
        // 清除现有卡片
        container.removeAllViews();
        cards.clear();
        
        // 添加新卡片数据
        addAllCards(newCardData);
        
        // 显示所有新卡片
        showAllCards();
        
        Log.d(TAG, "Data update completed, now have " + cards.size() + " cards");
    }
} 