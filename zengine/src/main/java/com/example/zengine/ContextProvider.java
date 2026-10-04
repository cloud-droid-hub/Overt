package com.example.zengine;

/**
 * Context 提供器(zengine 引擎)。
 * <p>
 * zengine 保持零 Android 依赖以便纯 JVM 单测,但部分能力(持久化存储目录)需要 Context。
 * 通过可注入的 Provider 解耦:
 * - 真机默认实现:反射 ActivityThread.currentApplication() 获取全局 Application context
 *   (与 C++ zJavaVm::getCurrentContext 的思路一致,但放 Java 侧)
 * - 单测注入假实现:返回 null 或临时目录,不触 Android API
 */
public final class ContextProvider {

    /** Context 获取接口。 */
    public interface ContextGetter {
        /** @return Android Context(Application);非 Android 环境可返回 null。 */
        Object getContext();
    }

    private static volatile ContextGetter provider = ContextProvider::reflectCurrentApplication;

    /** 注入自定义 Context 获取器(单测用 null 表示无 Context)。传 null 恢复默认反射实现。 */
    public static void setProvider(ContextGetter p) {
        provider = (p == null) ? ContextProvider::reflectCurrentApplication : p;
    }

    /** 获取当前 Context(可能为 null:非 Android 环境/反射失败)。 */
    public static Object getContext() {
        ContextGetter p = provider;
        return (p != null) ? p.getContext() : null;
    }

    /**
     * 默认实现:反射 ActivityThread.currentApplication() 获取全局 Application。
     * 在非 Android(JVM 单测)下 ActivityThread 类不存在 → 捕获返回 null。
     */
    private static Object reflectCurrentApplication() {
        try {
            Class<?> activityThread = Class.forName("android.app.ActivityThread");
            java.lang.reflect.Method currentApp =
                    activityThread.getMethod("currentApplication");
            return currentApp.invoke(null);
        } catch (Throwable t) {
            return null; // 非 Android 环境/反射失败 → null
        }
    }

    private ContextProvider() {
    }
}