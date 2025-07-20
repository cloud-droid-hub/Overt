package com.example.overt;

import android.content.Context;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.cardview.widget.CardView;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * 信息卡片容器类
 * 负责管理多个卡片的显示和间距控制
 */
public class InfoCardContainer {
    private static final String TAG = "InfoCardContainer";
    
    private Context context;
    private LinearLayout container;
    private List<InfoCard> cards;
    private String mainTitle; // 总标题
    
    // 卡片间距配置
    private int cardMarginTop = 0;
    private int cardMarginBottom = 1;
    private int cardMarginStart = 4;
    private int cardMarginEnd = 4;

    /**
     * 构造函数
     * @param context 上下文
     * @param container 容器视图
     */
    public InfoCardContainer(Context context, LinearLayout container) {
        this.context = context;
        this.container = container;
        this.cards = new ArrayList<>();
        this.mainTitle = "信息检测"; // 默认标题
    }

    /**
     * 构造函数（带间距配置）
     * @param context 上下文
     * @param container 容器视图
     * @param cardMarginTop 卡片顶部间距
     * @param cardMarginBottom 卡片底部间距
     * @param cardMarginStart 卡片左侧间距
     * @param cardMarginEnd 卡片右侧间距
     */
    public InfoCardContainer(Context context, LinearLayout container, 
                           int cardMarginTop, int cardMarginBottom, 
                           int cardMarginStart, int cardMarginEnd) {
        this.context = context;
        this.container = container;
        this.cards = new ArrayList<>();
        this.cardMarginTop = cardMarginTop;
        this.cardMarginBottom = cardMarginBottom;
        this.cardMarginStart = cardMarginStart;
        this.cardMarginEnd = cardMarginEnd;
        this.mainTitle = "设备信息检测"; // 默认标题
    }

    /**
     * 构造函数（带总标题）
     * @param context 上下文
     * @param container 容器视图
     * @param mainTitle 总标题
     */
    public InfoCardContainer(Context context, LinearLayout container, String mainTitle) {
        this.context = context;
        this.container = container;
        this.cards = new ArrayList<>();
        this.mainTitle = mainTitle != null ? mainTitle : "设备信息检测";
    }

    /**
     * 构造函数（完整配置）
     * @param context 上下文
     * @param container 容器视图
     * @param mainTitle 总标题
     * @param cardMarginTop 卡片顶部间距
     * @param cardMarginBottom 卡片底部间距
     * @param cardMarginStart 卡片左侧间距
     * @param cardMarginEnd 卡片右侧间距
     */
    public InfoCardContainer(Context context, LinearLayout container, String mainTitle,
                           int cardMarginTop, int cardMarginBottom, 
                           int cardMarginStart, int cardMarginEnd) {
        this.context = context;
        this.container = container;
        this.cards = new ArrayList<>();
        this.mainTitle = mainTitle != null ? mainTitle : "设备信息检测";
        this.cardMarginTop = cardMarginTop;
        this.cardMarginBottom = cardMarginBottom;
        this.cardMarginStart = cardMarginStart;
        this.cardMarginEnd = cardMarginEnd;
    }

    /**
     * 设置总标题
     * @param mainTitle 总标题
     */
    public void setMainTitle(String mainTitle) {
        this.mainTitle = mainTitle != null ? mainTitle : "设备信息检测";
    }

    /**
     * 获取总标题
     * @return 总标题
     */
    public String getMainTitle() {
        return mainTitle;
    }

    /**
     * 添加卡片
     * @param title 卡片标题
     * @param info 卡片信息数据
     */
    public void addCard(String title, Map<String, Map<String, String>> info) {
        InfoCard card = new InfoCard(context, title, info);
        cards.add(card);
    }

    /**
     * 添加卡片（使用InfoCard对象）
     * @param card InfoCard对象
     */
    public void addCard(InfoCard card) {
        cards.add(card);
    }

    /**
     * 批量添加卡片
     * @param cardData 卡片数据Map
     */
    public void addCards(Map<String, Map<String, Map<String, String>>> cardData) {
        for (Map.Entry<String, Map<String, Map<String, String>>> entry : cardData.entrySet()) {
            addCard(entry.getKey(), entry.getValue());
        }
    }

    /**
     * 显示所有卡片
     */
    public void show() {
        if (container == null) {
            Log.e(TAG, "Container is null");
            return;
        }

        // 清空容器
        container.removeAllViews();

        // 添加总标题
        addMainTitle();

        // 显示所有卡片
        for (InfoCard card : cards) {
            CardView cardView = card.show();
            if (cardView != null) {
                // 设置卡片间距
                setCardMargins(cardView);
                
                // 添加到容器
                container.addView(cardView);
                Log.d(TAG, "Added card: " + card.getTitle());
            }
        }
    }

    /**
     * 添加总标题
     */
    private void addMainTitle() {
        if (mainTitle != null && !mainTitle.isEmpty()) {
            TextView titleView = new TextView(context);
            titleView.setText(mainTitle);
            titleView.setTextSize(24);
            titleView.setTextColor(context.getResources().getColor(android.R.color.black, context.getTheme()));
            titleView.setTypeface(null, android.graphics.Typeface.BOLD);
            titleView.setGravity(android.view.Gravity.CENTER);
            
            // 设置标题间距
            LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            );
            titleParams.setMargins(0, 8, 0, 8);
            titleView.setLayoutParams(titleParams);
            
            container.addView(titleView);
        }
    }

    /**
     * 设置卡片间距
     * @param cardView 卡片视图
     */
    private void setCardMargins(CardView cardView) {
        ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) cardView.getLayoutParams();
        if (params == null) {
            params = new ViewGroup.MarginLayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            );
        }
        
        params.setMargins(cardMarginStart, cardMarginTop, cardMarginEnd, cardMarginBottom);
        cardView.setLayoutParams(params);
    }

    /**
     * 设置卡片间距
     * @param top 顶部间距
     * @param bottom 底部间距
     * @param start 左侧间距
     * @param end 右侧间距
     */
    public void setCardMargins(int top, int bottom, int start, int end) {
        this.cardMarginTop = top;
        this.cardMarginBottom = bottom;
        this.cardMarginStart = start;
        this.cardMarginEnd = end;
    }

    /**
     * 设置卡片间距（统一设置）
     * @param margin 统一间距值
     */
    public void setCardMargins(int margin) {
        setCardMargins(margin, margin, margin, margin);
    }

    /**
     * 清空所有卡片
     */
    public void clear() {
        cards.clear();
        if (container != null) {
            container.removeAllViews();
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
     * 获取指定位置的卡片
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
     * 移除指定位置的卡片
     * @param index 卡片索引
     */
    public void removeCard(int index) {
        if (index >= 0 && index < cards.size()) {
            cards.remove(index);
        }
    }

    /**
     * 移除指定标题的卡片
     * @param title 卡片标题
     */
    public void removeCard(String title) {
        cards.removeIf(card -> title.equals(card.getTitle()));
    }

    /**
     * 刷新显示
     */
    public void refresh() {
        show();
    }

    /**
     * 获取容器视图
     * @return LinearLayout容器
     */
    public LinearLayout getContainer() {
        return container;
    }

    /**
     * 设置容器视图
     * @param container LinearLayout容器
     */
    public void setContainer(LinearLayout container) {
        this.container = container;
    }
} 