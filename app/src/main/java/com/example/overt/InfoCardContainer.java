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

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * 信息卡片容器类
 * 只负责创建可滚动的卡片容器，不包含标题
 */
public class InfoCardContainer {
    private static final String TAG = "overt_" + InfoCardContainer.class.getSimpleName();
    
    private Context context;
    private NestedScrollView scrollView;
    private LinearLayout container;
    private Map<String, InfoCard> cards;
    private Map<String, CardView> cardViewMap; // 标题到CardView的映射缓存
    
    // 卡片间距
    private int cardMarginTop = 4;
    private int cardMarginBottom = 4;
    private int cardMarginStart = 4;
    private int cardMarginEnd = 4;

    /**
     * 构造函数 - 只创建卡片容器
     * @param context 上下文
     * @param cardData 卡片数据 JSONObject
     */
    public InfoCardContainer(Context context, JSONObject cardData) {
        this.context = context;
        this.cards = new HashMap<>();
        this.cardViewMap = new HashMap<>();
        
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
    private void addAllCards(JSONObject cardData) {
        if (cardData == null) return;
        
        try {
            Iterator<String> keys = cardData.keys();
            while (keys.hasNext()) {
                String key = keys.next();
                Object value = cardData.get(key);
                if (value instanceof JSONObject) {
                    JSONObject cardInfo = (JSONObject) value;
                    InfoCard card = new InfoCard(context, key, cardInfo);
                    cards.put(key, card);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Error adding cards from JSON: " + e.getMessage());
        }
    }


    /**
     * 更新指定title的卡片
     * 如果卡片不存在，会自动创建新卡片
     * @param title 卡片标题
     * @param cardData 新的卡片数据
     */
    public void updateCard(String title, JSONObject cardData) {
        updateCard(title, cardData, true);
    }

    public void updateCard(String title, JSONObject cardData, boolean createIfNotExists) {
        if (title == null || cardData == null || container == null) return;

        Log.d(TAG, "Updating card with title: " + title + ", createIfNotExists: " + createIfNotExists);

        // 检查卡片是否存在
        if (!cards.containsKey(title)) {
            if (createIfNotExists) {
                Log.i(TAG, "Card with title '" + title + "' not found, creating new card");
                // 如果卡片不存在，创建新卡片
                addCard(title, cardData);
            } else {
                Log.w(TAG, "Card with title '" + title + "' not found, cannot update");
            }
            return;
        }

        // 获取现有的CardView
        CardView existingCardView = cardViewMap.get(title);
        if (existingCardView == null) {
            if (createIfNotExists) {
                Log.w(TAG, "CardView for title '" + title + "' not found in cache, recreating card");
                // 如果CardView不存在，重新创建卡片
                cards.remove(title);
                addCard(title, cardData);
            } else {
                Log.w(TAG, "CardView for title '" + title + "' not found in cache");
            }
            return;
        }

        // 创建新卡片
        InfoCard newCard = new InfoCard(context, title, cardData);

        // 更新Map中的卡片
        cards.put(title, newCard);

        // 获取新卡片的CardView
        CardView newCardView = newCard.show();

        // 设置相同的布局参数
        LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        cardParams.setMargins(cardMarginStart, cardMarginTop, cardMarginEnd, cardMarginBottom);
        newCardView.setLayoutParams(cardParams);

        // 找到现有CardView在容器中的位置
        int index = container.indexOfChild(existingCardView);
        if (index != -1) {
            // 替换容器中的视图
            container.removeViewAt(index);
            container.addView(newCardView, index);

            // 更新CardView映射缓存
            cardViewMap.put(title, newCardView);

            Log.d(TAG, "Successfully updated card: " + title + " at index: " + index);
        } else {
            if (createIfNotExists) {
                Log.w(TAG, "Could not find CardView in container for card: " + title + ", recreating card");
                // 如果找不到CardView，重新创建卡片
                cards.remove(title);
                cardViewMap.remove(title);
                addCard(title, cardData);
            } else {
                Log.w(TAG, "Could not find CardView in container for card: " + title);
            }
        }
    }


    /**
     * 显示所有卡片
     */
    private void showAllCards() {
        if (container == null) return;

        int totalHeight = 0;
        for (InfoCard card : cards.values()) {
            CardView cardView = card.show();

            // 设置卡片间距
            LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            );
            cardParams.setMargins(cardMarginStart, cardMarginTop, cardMarginEnd, cardMarginBottom);
            cardView.setLayoutParams(cardParams);

            // 添加到CardView映射缓存
            cardViewMap.put(card.getTitle(), cardView);

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
        for (InfoCard card : cards.values()) {
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
     * 根据标题获取卡片
     * @param title 卡片标题
     * @return InfoCard对象
     */
    public InfoCard getCard(String title) {
        return cards.get(title);
    }

    /**
     * 根据标题获取CardView
     * @param title 卡片标题
     * @return CardView对象
     */
    public CardView getCardView(String title) {
        return cardViewMap.get(title);
    }

    /**
     * 获取卡片在容器中的位置索引
     * @param title 卡片标题
     * @return 位置索引，-1表示未找到
     */
    public int getCardIndex(String title) {
        CardView cardView = cardViewMap.get(title);
        if (cardView != null && container != null) {
            return container.indexOfChild(cardView);
        }
        return -1;
    }

    /**
     * 获取所有卡片
     * @return 卡片列表
     */
    public List<InfoCard> getAllCards() {
        return new ArrayList<>(cards.values());
    }

    /**
     * 获取所有卡片标题
     * @return 标题列表
     */
    public List<String> getAllCardTitles() {
        return new ArrayList<>(cards.keySet());
    }

    /**
     * 检查指定标题的卡片是否存在
     * @param title 卡片标题
     * @return true表示存在，false表示不存在
     */
    public boolean hasCard(String title) {
        return cards.containsKey(title);
    }

    /**
     * 移除指定标题的卡片
     * @param title 卡片标题
     * @return true表示成功移除，false表示卡片不存在
     */
    public boolean removeCard(String title) {
        if (!cards.containsKey(title)) {
            return false;
        }

        // 从CardView映射缓存中获取CardView
        CardView cardView = cardViewMap.get(title);
        if (cardView != null) {
            // 从容器中移除视图
            if (container != null) {
                container.removeView(cardView);
            }
        }

        // 从Map中移除
        cards.remove(title);
        cardViewMap.remove(title);

        Log.d(TAG, "Removed card: " + title);
        return true;
    }

    /**
     * 添加单个卡片
     * @param title 卡片标题
     * @param cardData 卡片数据
     * @return true表示成功添加，false表示卡片已存在
     */
    public boolean addCard(String title, JSONObject cardData) {
        if (cards.containsKey(title)) {
            Log.w(TAG, "Card with title '" + title + "' already exists");
            return false;
        }
        
        InfoCard card = new InfoCard(context, title, cardData);
        cards.put(title, card);
        
        // 显示卡片
        CardView cardView = card.show();
        
        // 设置卡片间距
        LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        cardParams.setMargins(cardMarginStart, cardMarginTop, cardMarginEnd, cardMarginBottom);
        cardView.setLayoutParams(cardParams);
        
        // 添加到CardView映射缓存
        cardViewMap.put(title, cardView);
        
        // 添加卡片到容器
        if (container != null) {
            container.addView(cardView);
        }
        
        Log.d(TAG, "Added new card: " + title);
        return true;
    }

    /**
     * 更新数据并刷新UI
     * @param newCardData 新的卡片数据
     */
    public void updateData(JSONObject newCardData) {
        if (newCardData == null || container == null) return;

        Log.d(TAG, "Updating data with JSONObject");

        // 清除现有卡片
        container.removeAllViews();
        cards.clear();
        cardViewMap.clear();

        // 添加新卡片数据
        addAllCards(newCardData);

        // 显示所有新卡片
        showAllCards();

        Log.d(TAG, "Data update completed, now have " + cards.size() + " cards");
    }

} 