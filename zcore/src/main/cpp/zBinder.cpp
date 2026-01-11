#include "zBinder.h"
#include "zLog.h"
#include <android/sharedmem.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <linux/futex.h>
#include <mutex>
#include <thread>
#include <chrono>
#include <asm-generic/unistd.h>
#include <zSyscall.h>

// 单例实例指针
zBinder* zBinder::instance = nullptr;

// Futex 辅助函数
static int futex_wait(volatile int* addr, int val) {
    return syscall(__NR_futex, addr, FUTEX_WAIT, val, NULL, NULL, 0);
}

static int futex_wake(volatile int* addr) {
    return syscall(__NR_futex, addr, FUTEX_WAKE, 1, NULL, NULL, 0);
}

/**
 * 获取单例实例
 * 采用线程安全的懒加载模式，首次调用时创建实例
 * @return zBinder单例指针
 */
zBinder* zBinder::getInstance() {
    // 使用 std::call_once 确保线程安全的单例初始化
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        try {
            instance = new zBinder();
            LOGI("zBinder: Created singleton instance");
        } catch (const std::exception& e) {
            LOGE("zBinder: Failed to create singleton instance: %s", e.what());
        } catch (...) {
            LOGE("zBinder: Failed to create singleton instance with unknown error");
        }
    });

    return instance;
}

zBinder::zBinder() : m_ptr(nullptr), m_fd(-1), 
                     m_main_loop_running(false), m_server_loop_running(false),
                     m_main_loop_thread(nullptr), m_server_loop_thread(nullptr),
                     m_message_callback(nullptr) {
}

zBinder::~zBinder() {
    stopMessageLoop();
    // 注意：fd 生命周期由创建者管理，这里不关闭
    // 如果需要清理，可以在外部调用 close(m_fd)
}

int zBinder::createSharedMemory() {
    if (m_fd >= 0 && m_ptr != nullptr) {
        LOGI("Shared memory already created, fd=%d", m_fd);
        return m_fd;
    }
    
    LOGI("Creating shared memory...");
    
    // 创建共享内存
    int fd = ASharedMemory_create("ipc_buf", SHM_SIZE);
    if (fd < 0) {
        LOGE("ASharedMemory_create failed");
        return -1;
    }
    
    // 设置权限
    int ret = ASharedMemory_setProt(fd, PROT_READ | PROT_WRITE);
    if (ret != 0) {
        LOGE("ASharedMemory_setProt failed: %d", ret);
        close(fd);
        return -1;
    }
    
    // 映射到内存
    void* ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        LOGE("mmap failed");
        close(fd);
        return -1;
    }
    
    m_ptr = ptr;
    m_fd = fd;
    
    // 初始化共享内存结构
    ShmLayout* layout = static_cast<ShmLayout*>(ptr);
    layout->ready.store(0, std::memory_order_release);
    
    LOGI("Shared memory created, fd=%d, ptr=%p", fd, ptr);
    return fd;
}

int zBinder::mapSharedMemory(int fd) {
    if (fd < 0) {
        LOGE("Invalid fd: %d", fd);
        return -1;
    }
    
    if (m_ptr != nullptr) {
        LOGI("Shared memory already mapped");
        return 0;
    }
    
    LOGI("Mapping shared memory, fd=%d", fd);
    
    // 映射共享内存
    void* ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        LOGE("mmap failed for fd %d", fd);
        return -1;
    }
    
    m_ptr = ptr;
    m_fd = fd;
    
    LOGI("Shared memory mapped successfully");
    return 0;
}

void zBinder::waitForReady(int target_value) {
    if (m_ptr == nullptr) {
        return;
    }
    
    ShmLayout* layout = static_cast<ShmLayout*>(m_ptr);
    int ready = layout->ready.load(std::memory_order_acquire);
    
    // 快速路径：如果已经就绪，直接返回
    if (ready == target_value) {
        return;
    }
    
    // 第一阶段：短时间自旋等待
    int spin_count = 0;
    while (spin_count < 1000 && ready != target_value) {
        ready = layout->ready.load(std::memory_order_acquire);
        if (ready == target_value) {
            return;
        }
        __asm__ __volatile__("yield" ::: "memory");
        spin_count++;
    }
    
    // 第二阶段：使用 futex 阻塞等待
    volatile int* ready_ptr = reinterpret_cast<volatile int*>(&layout->ready);
    while (ready != target_value) {
        int current = layout->ready.load(std::memory_order_acquire);
        if (current == target_value) {
            break;
        }
        futex_wait(ready_ptr, current);
        ready = layout->ready.load(std::memory_order_acquire);
    }
}

void zBinder::wakeWaiter() {
    if (m_ptr == nullptr) {
        return;
    }
    
    ShmLayout* layout = static_cast<ShmLayout*>(m_ptr);
    volatile int* ready_ptr = reinterpret_cast<volatile int*>(&layout->ready);
    futex_wake(ready_ptr);
}

int zBinder::sendMsg(const char* message) {
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return -1;
    }
    
    if (message == nullptr) {
        LOGE("Message is null");
        return -1;
    }
    
    ShmLayout* layout = static_cast<ShmLayout*>(m_ptr);
    
    // 如果上次的回复还没读取，先等待
    int ready = layout->ready.load(std::memory_order_acquire);
    if (ready == 1) {
        // 消息已发送但还没收到回复，等待回复
        waitForReady(2);
    }
    
    // 重置状态并写入消息
    layout->ready.store(0, std::memory_order_release);
    strncpy(layout->data, message, sizeof(layout->data) - 1);
    layout->data[sizeof(layout->data) - 1] = '\0';
    
    // 标记为已写入，唤醒 isolated 进程
    layout->ready.store(1, std::memory_order_release);
    wakeWaiter();
    
    LOGI("Message sent: %s", message);
    return 0;
}

int zBinder::waitForResponse(char* response, size_t maxLen) {
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return -1;
    }
    
    if (response == nullptr || maxLen == 0) {
        LOGE("Invalid response buffer");
        return -1;
    }
    
    ShmLayout* layout = static_cast<ShmLayout*>(m_ptr);
    
    // 等待响应
    waitForReady(2);
    
    int ready = layout->ready.load(std::memory_order_acquire);
    if (ready == 2) {
        strncpy(response, layout->data, maxLen - 1);
        response[maxLen - 1] = '\0';
        LOGI("Received response1: %s", response);
        return 0;
    } else {
        LOGE("Failed to get response");
        return -1;
    }
}

int zBinder::waitForMessage() {
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return -1;
    }
    
    ShmLayout* layout = static_cast<ShmLayout*>(m_ptr);
    waitForReady(1);
    
    int ready = layout->ready.load(std::memory_order_acquire);
    return (ready == 1) ? 0 : -1;
}

int zBinder::readMessage(char* message, size_t maxLen) {
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return -1;
    }
    
    if (message == nullptr || maxLen == 0) {
        LOGE("Invalid message buffer");
        return -1;
    }
    
    ShmLayout* layout = static_cast<ShmLayout*>(m_ptr);
    strncpy(message, layout->data, maxLen - 1);
    message[maxLen - 1] = '\0';
    
    return 0;
}

int zBinder::sendResponse(const char* response) {
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return -1;
    }
    
    if (response == nullptr) {
        LOGE("Response is null");
        return -1;
    }
    
    ShmLayout* layout = static_cast<ShmLayout*>(m_ptr);
    
    // 写入回复
    strncpy(layout->data, response, sizeof(layout->data) - 1);
    layout->data[sizeof(layout->data) - 1] = '\0';
    
    // 标记为已回复，并唤醒等待的主进程
    layout->ready.store(2, std::memory_order_release);
    wakeWaiter();
    
    LOGI("Response sent: %s", response);
    return 0;
}

// ========== 高级封装 API 实现 ==========

std::string zBinder::sendMessage(const std::string& message) {
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return "";
    }
    
    if (message.empty()) {
        LOGE("Message is empty");
        return "";
    }
    
    // 发送消息
    if (sendMsg(message.c_str()) != 0) {
        LOGE("Failed to send message");
        return "";
    }
    
    // 等待响应
    char response[512];
    if (waitForResponse(response, sizeof(response)) != 0) {
        LOGE("Failed to wait for response");
        return "";
    }
    
    return std::string(response);
}

std::string zBinder::waitAndReadMessage() {
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return "";
    }
    
    // 等待消息
    if (waitForMessage() != 0) {
        LOGE("Failed to wait for message");
        return "";
    }
    
    // 读取消息
    char message[512];
    if (readMessage(message, sizeof(message)) != 0) {
        LOGE("Failed to read message");
        return "";
    }
    
    return std::string(message);
}

int zBinder::sendResponse(const std::string& response) {
    if (response.empty()) {
        LOGE("Response is empty");
        return -1;
    }
    
    return sendResponse(response.c_str());
}

// ========== 消息循环线程管理 ==========

int zBinder::startMainMessageLoop() {
    if (m_main_loop_running) {
        LOGI("Main message loop already running");
        return 0;
    }
    
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return -1;
    }
    
    m_main_loop_running = true;
    m_main_loop_thread = new std::thread(&zBinder::mainMessageLoopThread, this);
    
    LOGI("Main message loop thread started");
    return 0;
}

int zBinder::startServerMessageLoop(int fd, std::function<std::string(std::string)> callback) {

    int ret = mapSharedMemory(fd);
    if (ret != 0) {
        LOGE("Failed to map shared memory");
        return -1;
    }

    if (m_server_loop_running) {
        LOGI("Server message loop already running");
        return 0;
    }
    
    if (m_ptr == nullptr) {
        LOGE("Shared memory not initialized");
        return -1;
    }
    
    // 保存回调函数
    m_message_callback = callback;
    
    m_server_loop_running = true;
    m_server_loop_thread = new std::thread(&zBinder::serverMessageLoopThread, this);
    
    LOGI("Server message loop thread started");
    return 0;
}

void zBinder::stopMessageLoop() {
    // 停止主进程消息循环
    if (m_main_loop_running) {
        m_main_loop_running = false;
        if (m_main_loop_thread != nullptr && m_main_loop_thread->joinable()) {
            m_main_loop_thread->join();
            delete m_main_loop_thread;
            m_main_loop_thread = nullptr;
        }
        LOGI("Main message loop stopped");
    }
    
    // 停止 isolated 进程消息循环
    if (m_server_loop_running) {
        m_server_loop_running = false;
        if (m_server_loop_thread != nullptr && m_server_loop_thread->joinable()) {
            m_server_loop_thread->join();
            delete m_server_loop_thread;
            m_server_loop_thread = nullptr;
        }
        LOGI("Server message loop stopped");
    }
}

void zBinder::mainMessageLoopThread() {
    LOGI("Main message loop thread started");
    int messageCount = 0;
    
    while (m_main_loop_running) {
        if (m_ptr == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        try {
            messageCount++;
            
            // 构造消息
            std::string msg = "hello";
            
            LOGI("Sending: %s", msg.c_str());
            
            // 使用高级封装 API：发送消息并等待响应
            std::string response = sendMessage(msg);
            if (!response.empty()) {
                LOGI("Received response2: %s", response.c_str());
            } else {
                LOGE("Failed to get response for message #%d", messageCount);
            }
            
            // 等待 1 秒
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            LOGE("Error in main message loop: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    LOGI("Main message loop thread stopped");
}

void zBinder::serverMessageLoopThread() {
    LOGI("Server message loop thread started");
    int responseCount = 0;
    
    while (m_server_loop_running) {
        if (m_ptr == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        try {
            // 使用高级封装 API：等待并读取消息
            std::string message = waitAndReadMessage();
            if (!message.empty()) {
                responseCount++;
                LOGI("Received message: %s", message.c_str());
                
                // 生成回复：如果有回调函数则使用回调函数，否则使用默认回复
                std::string response;
                if (m_message_callback) {
                    response = m_message_callback(message);
                } else {
                    response = "";
                }
                
                // 使用高级封装 API：发送响应
                if (sendResponse(response) == 0) {
                    LOGI("Sent response: %s", response.c_str());
                } else {
                    LOGE("Failed to send response");
                }
            } else {
                LOGE("Failed to wait for message");
            }
        } catch (const std::exception& e) {
            LOGE("Error in server message loop: %s", e.what());
        }
    }
    
    LOGI("Server message loop thread stopped");
}

