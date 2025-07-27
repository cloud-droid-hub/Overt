# Overt - Android设备信息检测工具

## 项目概述

Overt是一个Android应用，用于检测和显示设备的详细信息，包括系统信息、安全状态、网络配置等。

## 最新更新

### UI布局优化 (2025-01-27)

**问题**: 原来的UI设计中，标题栏会随着内容滚动而移出主界面，影响用户体验。

**解决方案**: 将界面重新设计为两个独立的部分：

1. **固定标题栏**: 位于界面顶部，始终可见
2. **可滚动内容区域**: 位于标题栏下方，包含所有信息卡片

**技术实现**:
- 修改了 `activity_main.xml` 布局文件，使用 `LinearLayout` 垂直排列两个组件
- 标题栏使用 `TextView` 组件，设置为固定高度
- 内容区域使用 `FrameLayout` 容器，通过 `layout_weight="1"` 占据剩余空间
- 重构了 `InfoCardContainer` 类，移除了标题创建功能，专注于卡片容器管理
- 更新了 `MainActivity` 以支持新的布局结构

**优势**:
- 更好的用户体验：标题始终可见
- 更好的封装性：标题和内容区域独立管理
- 更好的可维护性：组件职责分离

## 项目结构

```
app/
├── src/main/
│   ├── java/com/example/overt/
│   │   ├── MainActivity.java          # 主活动，管理整体布局
│   │   ├── InfoCardContainer.java     # 卡片容器，管理滚动内容
│   │   ├── InfoCard.java              # 单个信息卡片
│   │   └── MainApplication.java       # 应用程序类
│   ├── res/
│   │   ├── layout/
│   │   │   ├── activity_main.xml      # 主布局（固定标题+滚动内容）
│   │   │   └── item_info_card.xml     # 卡片项布局
│   │   └── values/
│   │       ├── colors.xml             # 颜色资源
│   │       └── strings.xml            # 字符串资源
│   └── cpp/                           # Native代码
└── build.gradle                       # 应用级构建配置
```

## 构建和运行

1. 确保已安装Android Studio和Android SDK
2. 克隆项目到本地
3. 在项目根目录运行：
   ```bash
   ./gradlew assembleDebug
   ```
4. 将生成的APK安装到Android设备上

## 功能特性

- **设备信息检测**: 系统版本、硬件信息、安全状态等
- **网络配置**: WiFi、移动网络、代理设置等
- **应用信息**: 已安装应用、权限等
- **安全检测**: Root检测、调试模式等
- **响应式设计**: 支持浅色/深色主题切换
- **固定标题栏**: 标题始终可见，提升用户体验

## 技术栈

- **语言**: Java, C++
- **UI框架**: Android原生组件
- **构建工具**: Gradle
- **Native开发**: CMake, NDK

## 许可证

本项目采用MIT许可证。
