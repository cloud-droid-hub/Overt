# Overt 卡片系统使用指南

## 概述

Overt卡片系统是一个模块化的Android UI组件，用于显示设备信息和安全状态。系统采用简化的架构设计，支持夜间模式适配，具有灵活的边框和字体颜色管理。

## 架构设计

### 简化架构
```
MainActivity
    ↓
Native Code (JNI) - get_device_info()
    ↓
InfoCardContainer (全屏容器)
    ↓
InfoCard (单个卡片)
    ↓
CardView + LinearLayout (UI组件)
```

### 数据流
```
Native Code (JNI)
    ↓
Map<String, Map<String, Map<String, String>>> (数据)
    ↓
InfoCardContainer (容器)
    ↓
LinearLayout (View)
    ↓
MainActivity (绑定)
```

### 极简调用流程
```
1. MainActivity声明native方法
2. 调用get_device_info()获取数据
3. 创建InfoCardContainer并传入数据
4. 直接使用容器View作为根布局
```

## 核心组件

### InfoCardContainer 类
全屏容器，构造时直接传递数据，返回View供Activity直接绑定：
```java
public class InfoCardContainer {
    // 构造函数 - 全屏容器
    public InfoCardContainer(Context context, String mainTitle, Map<String, Map<String, Map<String, String>>> cardData)
    
    // 构造函数 - 使用默认标题
    public InfoCardContainer(Context context, Map<String, Map<String, Map<String, String>>> cardData)
    
    // 获取容器View - 供Activity直接绑定（返回NestedScrollView支持滚动）
    public NestedScrollView getContainerView()
    
    // 刷新容器（主题切换时使用）
    public void refresh()
    
    // 获取卡片
    public InfoCard getCard(int index)
    public List<InfoCard> getAllCards()
}
```

### InfoCard 类
单个卡片组件，支持双边框颜色和双字体颜色系统：
```java
public class InfoCard {
    // 构造函数
    public InfoCard(Context context, String title, Map<String, Map<String, String>> info)
    public InfoCard(Context context, String title, Map<String, Map<String, String>> info, 
                   int borderWidth, int lightBorderColor, int darkBorderColor,
                   int lightTextColor, int darkTextColor)
    
    // 边框管理
    public void setBorderWidth(int width)
    public void setBorderColor(int lightColor, int darkColor)
    public void setBorderColor(int color)
    public void setLightBorderColor(int color)
    public void setDarkBorderColor(int color)
    public int getBorderWidth()
    public int getBorderColor() // 根据当前主题自动返回对应颜色
    public int getLightBorderColor()
    public int getDarkBorderColor()
    public void refreshBorder() // 刷新边框（主题切换时使用）
    
    // 字体颜色管理
    public void setTextColor(int lightColor, int darkColor)
    public void setTextColor(int color)
    public void setLightTextColor(int color)
    public void setDarkTextColor(int color)
    public int getTextColor() // 根据当前主题自动返回对应颜色
    public int getLightTextColor()
    public int getDarkTextColor()
    public void refreshTextColors() // 刷新文字颜色（主题切换时使用）
    
    // 显示方法
    public CardView show()
    
    // Getter方法
    public CardView getCardView()
    public String getTitle()
    public Map<String, Map<String, String>> getInfo()
}
```

## 使用示例

### 1. 基本使用（推荐）
```java
// 在MainActivity中
public class MainActivity extends AppCompatActivity {
    // Native方法声明
    native Map<String, Map<String, Map<String, String>>> get_device_info();

    static {
        System.loadLibrary("overt");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 直接从native获取设备信息数据
        Map<String, Map<String, Map<String, String>>> deviceInfo = get_device_info();
        
        // 创建卡片容器（简化调用）
        cardContainer = new InfoCardContainer(this, deviceInfo);
        
        // 直接使用容器View作为根布局（支持滚动）
        setContentView(cardContainer.getContainerView());
    }
}
```

**注意**: ActionBar默认不显示，通过主题配置实现：
- `values/themes.xml`: `Theme.MaterialComponents.DayNight.NoActionBar`
- `values-night/themes.xml`: `Theme.MaterialComponents.DayNight.NoActionBar`

**滚动功能**: 容器使用NestedScrollView实现，支持上下滑动查看所有内容。包含测试内容以确保滚动功能正常工作。

### 2. 自定义标题
```java
// 使用自定义标题
InfoCardContainer cardContainer = new InfoCardContainer(this, "安全检测报告", deviceInfo);
```

### 3. 自定义边框和字体颜色
```java
// 创建卡片时自定义边框和字体颜色
InfoCard card = new InfoCard(context, title, info, 
                           2, 0xFF888888, 0xFF333333,  // 边框宽度和颜色
                           0xFF000000, 0xFFE0E0E0);   // 字体颜色

// 动态修改边框
card.setBorderWidth(3);
card.setBorderColor(0xFF666666, 0xFF222222);

// 动态修改字体颜色
card.setTextColor(0xFF333333, 0xFFCCCCCC);
```

### 4. 主题切换支持
```java
// 主题切换时刷新容器
cardContainer.refresh();

// 或者单独刷新卡片
for (InfoCard card : cardContainer.getAllCards()) {
    card.refreshBorder();
    card.refreshTextColors();
}
```

## 设计特点

### 简化架构
- **一键创建**: 构造时直接传递数据，自动创建完整UI
- **直接绑定**: 返回View供Activity直接绑定，无需复杂配置
- **数据驱动**: 纯数据输入，自动生成UI组件
- **滚动支持**: 使用ScrollView实现，支持上下滑动查看所有内容

### 双颜色系统
- **边框颜色**: 支持浅色和夜间模式分别设置
- **字体颜色**: 支持浅色和夜间模式分别设置
- **自动适配**: 根据主题自动切换对应颜色

### 夜间模式适配
- **自动检测**: 系统自动检测当前主题
- **实时切换**: 支持主题切换时实时更新
- **完美兼容**: 与系统主题完全兼容

### 扁平化设计
- **简洁边框**: 动态管理的边框系统
- **无阴影**: 扁平化现代风格
- **小圆角**: 4dp圆角保持现代感

## 优势特点

1. **极简调用**: 一行代码创建完整UI
2. **数据分离**: UI和数据完全分离
3. **直接获取**: 直接从native代码获取数据，无需中间层
4. **主题适配**: 完美的夜间模式支持
5. **灵活配置**: 支持自定义边框和字体颜色
6. **性能优化**: 避免重复创建视图和中间层
7. **易于维护**: 模块化设计，职责清晰
8. **向后兼容**: 保持API稳定性
9. **类型安全**: 使用强类型数据结构
10. **架构简洁**: 移除不必要的中间层，直接native到UI

## 数据格式

### 卡片数据结构
```java
Map<String, Map<String, Map<String, String>>>
// 外层Map: 卡片标题 -> 卡片内容
// 中层Map: 信息项名称 -> 信息项详情
// 内层Map: 属性名 -> 属性值
```

### 信息项格式
```java
Map<String, String> infoItem = new HashMap<>();
infoItem.put("risk", "error|warn|safe|info");  // 风险等级
infoItem.put("explain", "详细说明");           // 详细说明
```

### 风险等级说明
- **error**: 错误状态，显示红色"×"符号
- **warn**: 警告状态，显示橙色"⚠"符号
- **safe**: 安全状态，显示绿色"√"符号
- **info**: 信息状态，显示绿色"√"符号