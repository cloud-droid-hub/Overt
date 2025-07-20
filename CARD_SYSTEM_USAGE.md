# 卡片系统使用指南

## 🏗️ 架构概述

新的卡片系统采用了模块化设计，包含三个主要组件：

1. **InfoCard** - 单个卡片类
2. **InfoCardContainer** - 卡片容器类（支持总标题管理）
3. **DeviceInfoProvider** - 设备信息提供者类

## 🎨 设计特点

### 扁平化设计
- **简洁边框**: 使用1dp的浅灰色边框 (#E0E0E0)，通过drawable实现
- **无阴影**: 移除卡片阴影，采用扁平化风格
- **小圆角**: 4dp的圆角，保持现代感
- **白色背景**: 简洁的白色背景

### 总标题管理
- 容器类自动管理总标题显示
- 支持动态设置和获取总标题
- 标题样式统一，24sp粗体居中显示

### 风险等级图标
- **Error (×)**: 红色叉号，表示错误
- **Warn (⚠)**: 警告符号，表示警告
- **Safe (√)**: 绿色对勾，表示安全

## 📋 类结构

### InfoCard 类
```java
public class InfoCard {
    // 构造函数
    public InfoCard(Context context, String title, Map<String, Map<String, String>> info)
    
    // 显示方法
    public CardView show()
    
    // Getter方法
    public CardView getCardView()
    public String getTitle()
    public Map<String, Map<String, String>> getInfo()
}
```

### InfoCardContainer 类
```java
public class InfoCardContainer {
    // 构造函数
    public InfoCardContainer(Context context, LinearLayout container)
    public InfoCardContainer(Context context, LinearLayout container, String mainTitle)
    public InfoCardContainer(Context context, LinearLayout container, 
                           int cardMarginTop, int cardMarginBottom, 
                           int cardMarginStart, int cardMarginEnd)
    public InfoCardContainer(Context context, LinearLayout container, String mainTitle,
                           int cardMarginTop, int cardMarginBottom, 
                           int cardMarginStart, int cardMarginEnd)
    
    // 总标题管理
    public void setMainTitle(String mainTitle)
    public String getMainTitle()
    
    // 卡片管理
    public void addCard(String title, Map<String, Map<String, String>> info)
    public void addCard(InfoCard card)
    public void addCards(Map<String, Map<String, Map<String, String>>> cardData)
    
    // 显示方法
    public void show()
    
    // 间距控制
    public void setCardMargins(int top, int bottom, int start, int end)
    public void setCardMargins(int margin)
    
    // 其他管理方法
    public void clear()
    public void refresh()
    public int getCardCount()
    public InfoCard getCard(int index)
    public List<InfoCard> getAllCards()
    public void removeCard(int index)
    public void removeCard(String title)
}
```

### DeviceInfoProvider 类
```java
public class DeviceInfoProvider {
    // 构造函数
    public DeviceInfoProvider(Context context, LinearLayout mainContainer)
    public DeviceInfoProvider(Context context, LinearLayout mainContainer, String mainTitle)
    
    // 总标题管理
    public void setMainTitle(String mainTitle)
    public String getMainTitle()
    
    // 显示方法
    public void showDeviceInfo()
    public void refresh()
    
    // 间距控制
    public void setCardMargins(int top, int bottom, int start, int end)
    public void setCardMargins(int margin)
    
    // Getter方法
    public InfoCardContainer getCardContainer()
    public LinearLayout getMainContainer()
    public Context getContext()
}
```

## 🚀 使用示例

### 1. 基本使用（推荐）
```java
// 在MainActivity中
LinearLayout mainContainer = findViewById(R.id.main);
DeviceInfoProvider deviceInfoProvider = new DeviceInfoProvider(this, mainContainer);

// 自动获取并显示所有设备信息，包含默认总标题"设备信息检测"
```

### 2. 自定义总标题
```java
LinearLayout mainContainer = findViewById(R.id.main);
DeviceInfoProvider deviceInfoProvider = new DeviceInfoProvider(this, mainContainer, "安全检测报告");

// 或者动态设置总标题
deviceInfoProvider.setMainTitle("设备安全分析");
deviceInfoProvider.refresh();
```

### 3. 自定义间距和总标题
```java
LinearLayout container = findViewById(R.id.main);

// 创建容器，设置总标题和间距
InfoCardContainer cardContainer = new InfoCardContainer(this, container, "自定义标题", 0, 1, 4, 4);

// 添加卡片
Map<String, Map<String, String>> cardInfo = new HashMap<>();
cardInfo.put("test_item", new HashMap<String, String>() {{
    put("risk", "error");
    put("explain", "This is a test error");
}});
cardContainer.addCard("测试卡片", cardInfo);

// 显示所有卡片（包含总标题）
cardContainer.show();
```

### 4. 动态管理总标题
```java
InfoCardContainer container = new InfoCardContainer(this, layout);

// 设置总标题
container.setMainTitle("动态标题");

// 获取总标题
String title = container.getMainTitle();

// 刷新显示
container.refresh();
```

## 🎨 间距控制

### 容器类间距设置
```java
InfoCardContainer container = new InfoCardContainer(context, layout, "标题");

// 方法1: 分别设置四个方向的间距
container.setCardMargins(0, 1, 4, 4); // top, bottom, start, end

// 方法2: 统一设置所有方向的间距
container.setCardMargins(2); // 所有方向都是2dp

// 方法3: 构造函数中设置
InfoCardContainer container = new InfoCardContainer(context, layout, "标题", 0, 1, 4, 4);
```

### 间距配置说明
- **紧凑布局**: `setCardMargins(0, 1, 4, 4)` - 顶部无间距，底部1dp，左右4dp
- **标准布局**: `setCardMargins(4)` - 所有方向4dp
- **宽松布局**: `setCardMargins(8)` - 所有方向8dp

## 🔧 高级功能

### 动态管理卡片和标题
```java
InfoCardContainer container = new InfoCardContainer(context, layout);

// 设置总标题
container.setMainTitle("安全检测报告");

// 添加卡片
container.addCard("新卡片", cardInfo);

// 获取卡片
InfoCard card = container.getCard(0);
String title = card.getTitle();

// 移除卡片
container.removeCard(0); // 按索引移除
container.removeCard("卡片标题"); // 按标题移除

// 清空所有卡片
container.clear();

// 刷新显示（包含总标题）
container.refresh();
```

### 获取容器信息
```java
InfoCardContainer container = new InfoCardContainer(context, layout);

// 获取总标题
String mainTitle = container.getMainTitle();

// 获取卡片数量
int count = container.getCardCount();

// 获取所有卡片
List<InfoCard> allCards = container.getAllCards();

// 获取容器视图
LinearLayout containerView = container.getContainer();
```

## 📱 实际应用场景

### 场景1: 设备信息检测
```java
// 自动检测并显示所有设备信息
DeviceInfoProvider provider = new DeviceInfoProvider(this, mainContainer, "设备安全检测");
// 完成！所有检测结果会自动显示为卡片，包含总标题
```

### 场景2: 自定义检测结果
```java
InfoCardContainer container = new InfoCardContainer(this, mainContainer, "自定义检测");

// 添加自定义检测结果
Map<String, Map<String, String>> customResult = new HashMap<>();
customResult.put("自定义检测", new HashMap<String, String>() {{
    put("risk", "error");
    put("explain", "发现安全问题");
}});

container.addCard("自定义检测", customResult);
container.show(); // 显示总标题和所有卡片
```

### 场景3: 动态更新标题
```java
DeviceInfoProvider provider = new DeviceInfoProvider(this, mainContainer);

// 用户点击刷新按钮
refreshButton.setOnClickListener(v -> {
    provider.setMainTitle("实时检测 - " + new Date().toString());
    provider.refresh(); // 重新检测并更新显示
});
```

## ✅ 优势特点

1. **模块化设计**: 每个类职责单一，易于维护
2. **总标题管理**: 容器类统一管理总标题显示
3. **扁平化设计**: 简洁现代的UI风格
4. **灵活间距控制**: 容器类统一管理卡片间距
5. **类型安全**: 使用强类型的数据结构
6. **易于扩展**: 可以轻松添加新的卡片类型
7. **性能优化**: 避免重复创建视图
8. **代码复用**: 卡片类可以在不同场景中使用

## 🔄 迁移指南

### 从旧版本迁移
```java
// 旧版本
DeviceInfoProvider provider = new DeviceInfoProvider(context, container);
// 自动处理所有逻辑

// 新版本 - 保持相同的使用方式
DeviceInfoProvider provider = new DeviceInfoProvider(context, container);
// 内部使用新的卡片系统，自动显示总标题，对外接口保持不变
```

新版本完全向后兼容，现有代码无需修改即可使用新的卡片系统和总标题功能！

## 🎨 设计规范

### 卡片样式
- **边框**: 1dp 浅灰色 (#E0E0E0)，通过drawable实现
- **圆角**: 4dp
- **阴影**: 无阴影（扁平化设计）
- **背景**: 白色
- **内边距**: 12dp

### 边框实现
边框通过 `card_border_background.xml` drawable文件实现：
```xml
<shape android:shape="rectangle">
    <solid android:color="@android:color/white" />
    <stroke android:width="1dp" android:color="#E0E0E0" />
    <corners android:radius="4dp" />
</shape>
```

### 文字样式
- **总标题**: 24sp 粗体 居中 黑色
- **卡片标题**: 18sp 粗体 黑色
- **内容文字**: 14sp 正常 根据风险等级着色

### 总标题样式
- **字体大小**: 24sp
- **字体粗细**: 粗体
- **对齐方式**: 居中
- **颜色**: 黑色
- **间距**: 上下8dp

### 风险等级图标和颜色
- **Error (×)**: 红色叉号 (#FF0000)
- **Warn (⚠)**: 警告符号，橙色 (#FF8C00)
- **Safe (√)**: 绿色对勾，默认颜色（黑色/白色）

### 风险等级背景色
- **Error**: 淡红色背景 (R.color.risk_error_bg)
- **Warn**: 淡橙色背景 (R.color.risk_warn_bg)
- **Safe**: 白色背景 