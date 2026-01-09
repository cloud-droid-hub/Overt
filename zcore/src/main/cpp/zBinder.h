#ifndef ZBINDER_H
#define ZBINDER_H

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <functional>

// 共享内存布局结构
#define SHM_SIZE 4096
struct ShmLayout {
    std::atomic<int> ready;  // 0=未就绪, 1=主进程已写入, 2=isolated进程已写入
    char data[SHM_SIZE - sizeof(std::atomic<int>)];
};

/**
 * zBinder 类：封装共享内存 IPC 通信（单例模式）
 */
class zBinder {
public:
    /**
     * 获取单例实例
     * 采用线程安全的懒加载模式，首次调用时创建实例
     * @return zBinder单例指针
     */
    static zBinder* getInstance();
    
    ~zBinder();
    
    // 禁止拷贝和赋值
    zBinder(const zBinder&) = delete;
    zBinder& operator=(const zBinder&) = delete;
    
    /**
     * 主进程：创建共享内存
     * @return fd 文件描述符，失败返回 -1
     */
    int createSharedMemory();
    
    /**
     * isolated 进程：映射共享内存
     * @param fd 文件描述符
     * @return 0 成功，-1 失败
     */
    int mapSharedMemory(int fd);
    
    /**
     * 获取共享内存 fd
     * @return fd 文件描述符
     */
    int getFd() const { return m_fd; }
    
    /**
     * 获取共享内存指针
     * @return 共享内存指针
     */
    void* getPtr() const { return m_ptr; }
    
    /**
     * 发送消息（主进程使用）
     * @param message 消息内容
     * @return 0 成功，-1 失败
     */
    int sendMsg(const char* message);
    
    /**
     * 等待响应（主进程使用）
     * @param response 输出参数，响应内容
     * @param maxLen 最大长度
     * @return 0 成功，-1 失败
     */
    int waitForResponse(char* response, size_t maxLen);
    
    /**
     * 等待消息（isolated 进程使用）
     * @return 0 成功，-1 失败
     */
    int waitForMessage();
    
    /**
     * 读取消息（isolated 进程使用）
     * @param message 输出参数，消息内容
     * @param maxLen 最大长度
     * @return 0 成功，-1 失败
     */
    int readMessage(char* message, size_t maxLen);
    
    /**
     * 发送响应（isolated 进程使用）
     * @param response 响应内容
     * @return 0 成功，-1 失败
     */
    int sendResponse(const char* response);
    
    // ========== 高级封装 API ==========
    
    /**
     * 发送消息并等待响应（主进程使用，高级封装）
     * @param message 消息内容
     * @return 响应字符串，失败返回空字符串
     */
    std::string sendMessage(const std::string& message);
    
    /**
     * 等待并读取消息（isolated 进程使用，高级封装）
     * @return 消息字符串，失败返回空字符串
     */
    std::string waitAndReadMessage();
    
    /**
     * 发送响应（isolated 进程使用，高级封装）
     * @param response 响应内容
     * @return 0 成功，-1 失败
     */
    int sendResponse(const std::string& response);
    
    /**
     * 检查是否已初始化
     */
    bool isInitialized() const { return m_ptr != nullptr; }
    
    /**
     * 启动主进程消息循环线程（自动发送消息并等待响应）
     * @return 0 成功，-1 失败
     */
    int startMainMessageLoop();
    
    /**
     * 启动 isolated 进程消息循环线程（自动等待消息并发送响应）
     * @param fd 文件描述符
     * @param callback 消息处理回调函数，接收消息字符串，返回响应字符串
     * @return 0 成功，-1 失败
     */
    int startServerMessageLoop(int fd, std::function<std::string(std::string)> callback = nullptr);
    
    /**
     * 停止消息循环线程
     */
    void stopMessageLoop();

private:
    // 私有构造函数，禁止外部创建实例
    zBinder();
    
    // 单例实例指针
    static zBinder* instance;
    
    void* m_ptr;
    int m_fd;
    
    // 消息循环线程管理
    bool m_main_loop_running;
    bool m_server_loop_running;
    std::thread* m_main_loop_thread;
    std::thread* m_server_loop_thread;
    
    // 消息处理回调函数
    std::function<std::string(std::string)> m_message_callback;
    
    // 内部辅助函数
    void waitForReady(int target_value);
    void wakeWaiter();
    
    // 消息循环线程函数
    void mainMessageLoopThread();
    void serverMessageLoopThread();
};

#endif // ZBINDER_H

