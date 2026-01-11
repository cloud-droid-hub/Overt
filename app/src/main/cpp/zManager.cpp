//
// Created by lxz on 2025/7/10.
//

#include <sys/resource.h>
#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zFile.h"
#include "zManager.h"
#include "zSyscall.h"
#include "zSslInfo.h"
#include "zJson.h"
#include "zRootStateInfo.h"
#include "zProcInfo.h"
#include "zSystemPropInfo.h"
#include "zLinkerInfo.h"
#include "zTimeInfo.h"
#include "zPackageInfo.h"
#include "zClassLoaderInfo.h"
#include "zSystemSettingInfo.h"
#include "zPortInfo.h"
#include "zTeeInfo.h"
#include "zSslInfo.h"
#include "zLocalNetworkInfo.h"
#include "zThreadPool.h"
#include "zLogcatInfo.h"
#include "zJavaVm.h"
#include "zSignatureInfo.h"
#include "zSideChannelInfo.h"
#include "zIsoloatedProcess.h"
#include "zSensorInfo.h"
#include "zFingerInfo.h"
#include <mutex>
#include <cmath>


/**
 * 静态成员变量初始化
 * 单例实例指针，初始化为nullptr
 * 采用懒加载模式，首次调用getInstance()时创建
 */
zManager* zManager::instance = nullptr;

/**
 * 设备信息存储Map初始化
 * 三层嵌套的Map结构，用于存储所有检测结果
 * 结构：检测类别 -> 检测项目 -> 属性名 -> 属性值
 * 线程安全：通过shared_mutex保护
 */
map<string, map<string, map<string, string>>> zManager::device_info;

/**
 * 获取单例实例
 * 采用线程安全的懒加载模式，首次调用时创建实例
 * @return zManager单例指针
 */
zManager* zManager::getInstance() {
    // 使用 std::call_once 确保线程安全的单例初始化
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        try {
            instance = new zManager();
            LOGI("zManager: Created singleton instance");
        } catch (const std::exception& e) {
            LOGE("zManager: Failed to create singleton instance: %s", e.what());
        } catch (...) {
            LOGE("zManager: Failed to create singleton instance with unknown error");
        }
    });
    
    return instance;
}

/**
 * 清理单例实例（主要用于测试或程序退出时）
 */
void zManager::cleanup() {
    if (instance != nullptr) {
        try {
            delete instance;
            instance = nullptr;
            LOGI("zManager: Singleton instance cleaned up");
        } catch (const std::exception& e) {
            LOGE("zManager: Exception during cleanup: %s", e.what());
        } catch (...) {
            LOGE("zManager: Unknown exception during cleanup");
        }
    }
}

/**
 * 构造函数实现
 * 初始化设备信息管理器
 */
zManager::zManager() {
    LOGD("Constructor called");
    // 初始化代码可以在这里添加
}

/**
 * 析构函数实现
 * 清理资源
 */
zManager::~zManager() {
    LOGD("Destructor called");
    // 清理代码可以在这里添加
}

/**
 * 获取所有设备信息
 * 使用读锁保证线程安全，允许多个线程同时读取
 * @return 三层嵌套的Map，包含所有收集到的设备信息
 */
const map<string, map<string, map<string, string>>>& zManager::get_device_info() const{
    LOGD("get_device_info called");
    // 使用共享读锁，允许多个线程同时读取
    std::shared_lock<std::shared_mutex> lock(device_info_mtx_);
    LOGI("get_device_info: device_info size=%zu", device_info.size());
    return device_info;
};

/**
 * 更新设备信息
 * 使用写锁保证线程安全，确保数据一致性
 * @param key 信息类别（如"task_info", "maps_info"等）
 * @param value 该类别下的具体信息
 */
void zManager::update_device_info(const string& key, const map<string, map<string, string>>& value){
    LOGD("update_device_info called, key=%s", key.c_str());
    // 使用独占写锁，确保更新操作的原子性
    std::unique_lock<std::shared_mutex> lock(device_info_mtx_);
    device_info[key] = value;
    LOGI("update_device_info: updated key=%s", key.c_str());
};

const map<string, map<string, string>> zManager::get_info(const string& key){
    LOGD("get_info called, key=%s", key.c_str());
    // 使用独占写锁，确保更新操作的原子性
    std::unique_lock<std::shared_mutex> lock(device_info_mtx_);
    map<string, map<string, string>> info = device_info[key];
    LOGI("update_device_info: updated key=%s", key.c_str());
    return info;
};


/**
 * 周期性任务管理循环
 * 
 * 功能说明：
 * 1. 管理所有安全检测任务的周期性执行
 * 2. 通过线程池调度任务执行
 * 3. 提供异常处理和错误恢复机制
 * 4. 实现任务的自动重试和监控
 * 
 * 任务类型：
 * - root_state_info: Root状态检测
 * - proc_info: 进程信息检测
 * - tee_info: TEE环境检测
 * - class_loader_info: 类加载器检测
 * - linker_info: 链接器信息检测
 * - package_info: 包信息检测
 * - system_setting_info: 系统设置检测
 * - system_prop_info: 系统属性检测
 * - signature_info: 签名信息检测
 * - port_info: 端口信息检测
 * - time_info: 时间信息检测
 * - ssl_info: SSL证书检测
 * - local_network_info: 本地网络检测
 * - logcat_info: 系统日志检测
 * - side_channel_info: 侧信道检测
 * 
 * 执行机制：
 * - 无限循环，持续监控任务状态
 * - 检查任务是否已在线程池中运行
 * - 未运行的任务会被添加到线程池
 * - 每个周期后等待指定时间间隔
 * 
 * 异常处理：
 * - 捕获标准异常和未知异常
 * - 异常时短暂等待后继续执行
 * - 确保任务管理循环不会因异常而停止
 */
void zManager::round_tasks(){
    LOGI("add_tasks: starting periodic task management loop");
    
    // 定义所有需要周期性执行的任务
    // 使用函数指针数组存储任务名称和对应的更新函数
    vector<pair<string, void(zManager::*)()>> periodic_tasks = {

            {"finger_info", &zManager::update_finger_info},
            {"linker_info", &zManager::update_linker_info},
            {"proc_info", &zManager::update_proc_info},
            {"root_state_info", &zManager::update_root_state_info},
            {"tee_info", &zManager::update_tee_info},
            {"class_loader_info", &zManager::update_class_loader_info},
            {"class_info", &zManager::update_class_info},
            {"package_info", &zManager::update_package_info},
            {"system_setting_info", &zManager::update_system_setting_info},
            {"system_prop_info", &zManager::update_system_prop_info},
            {"signature_info", &zManager::update_signature_info},
            {"port_info", &zManager::update_port_info},
            {"time_info", &zManager::update_time_info},
            {"ssl_info", &zManager::update_ssl_info},
            {"local_network_info", &zManager::update_local_network_info},
            {"logcat_info", &zManager::update_logcat_info},
            {"side_channel_info", &zManager::side_channel_info},
            {"isoloated_process_info", &zManager::update_isoloated_process_info},
            {"sensor_info", &zManager::update_sensor_info},

    };
    LOGI("add_tasks: initialized %zu periodic tasks", periodic_tasks.size());
    
    // 主循环：持续监控和管理任务
    while (true) {
        try {
            // 检查每个任务是否需要执行
            for(const auto& task : periodic_tasks) {
                const string& task_name = task.first;
                // 如果任务未在线程池中运行，则添加任务
                if(!zThreadPool::getInstance()->hasTaskName(task_name)){
                     zThreadPool::getInstance()->addTask(task_name, zManager::getInstance(), task.second);
                }
            }
            // 等待一段时间后再次检查
            LOGD("add_tasks: waiting %d seconds before next cycle", time_interval);
            sleep(time_interval*10);
        } catch (const std::exception& e) {
            LOGE("add_tasks: exception occurred: %s", e.what());
            sleep(5); // 发生异常时短暂等待
        } catch (...) {
            LOGE("add_tasks: unknown exception occurred");
            sleep(5); // 发生异常时短暂等待
        }
    }
    LOGI("add_tasks: periodic task management loop stopped");
};

/**
 * 更新SSL信息检测结果
 * 
 * 功能说明：
 * 1. 收集SSL证书和连接信息
 * 2. 检测SSL证书异常和配置问题
 * 3. 更新设备信息存储
 * 4. 通知Java层更新UI
 * 
 * 检测内容：
 * - SSL证书有效性
 * - 证书链完整性
 * - 加密算法强度
 * - 连接安全性
 */
void zManager::update_ssl_info(){
    // 收集SSL信息 - 检测SSL证书异常
    update_device_info("ssl_info", get_ssl_info());
    notice_java("ssl_info");
};

/**
 * 更新本地网络信息检测结果
 * 
 * 功能说明：
 * 1. 扫描本地网络中的设备
 * 2. 检测同一网络中的其他Overt设备
 * 3. 分析网络拓扑和连接状态
 * 4. 更新设备信息存储
 * 5. 通知Java层更新UI
 * 
 * 检测内容：
 * - 网络设备列表
 * - 设备类型和操作系统
 * - 网络连接状态
 * - 安全风险评估
 */
void zManager::update_local_network_info(){
    // 收集本地网络信息 - 检测同一网络中的其他Overt设备
    zManager::getInstance()->update_device_info("local_network_info", get_local_network_info());
    notice_java("local_network_info");
};

void zManager::update_isoloated_process_info(){
    zManager::getInstance()->update_device_info("isoloated_process_info", get_isoloated_process_info());
    notice_java("isoloated_process_info");
};

void zManager::update_sensor_info(){
    zManager::getInstance()->update_device_info("sensor_info", get_sensor_info());
    notice_java("sensor_info");
};

void zManager::update_finger_info(){
    zManager::getInstance()->update_device_info("finger_info", get_finger_info());
    notice_java("finger_info");
};

/**
 * 更新进程信息检测结果
 * 
 * 功能说明：
 * 1. 收集当前运行的进程信息
 * 2. 检测Frida等调试工具注入的进程
 * 3. 分析进程权限和状态
 * 4. 更新设备信息存储
 * 5. 通知Java层更新UI
 * 
 * 检测内容：
 * - 进程列表和状态
 * - 调试工具特征检测
 * - 进程权限分析
 * - 异常进程识别
 */
void zManager::update_proc_info(){
    // 收集任务信息 - 检测Frida等调试工具注入的进程
    zManager::getInstance()->update_device_info("proc_info", get_proc_info());
    notice_java("proc_info");
};

/**
 * 更新Root状态检测结果
 * 
 * 功能说明：
 * 1. 检测设备是否被Root
 * 2. 检查Root相关文件和目录
 * 3. 分析系统权限状态
 * 4. 更新设备信息存储
 * 5. 通知Java层更新UI
 * 
 * 检测内容：
 * - su文件检测
 * - 系统属性检查
 * - 挂载点分析
 * - 权限状态验证
 */
void zManager::update_root_state_info(){
    // 收集Root文件信息 - 检测Root相关文件
    zManager::getInstance()->update_device_info("root_state_info", get_root_state_info());
    notice_java("root_state_info");
};

void zManager::update_system_prop_info(){
// 收集系统属性信息 - 检测系统配置异常
    zManager::getInstance()->update_device_info("system_prop_info", get_system_prop_info());
    notice_java("system_prop_info");
};

void zManager::update_linker_info(){
// 收集链接器信息 - 检测动态链接库加载异常
    zManager::getInstance()->update_device_info("linker_info", get_linker_info());
    notice_java("linker_info");
};

void zManager::update_port_info(){
    LOGI("update_port_info: starting port info collection");
    zManager::getInstance()->update_device_info("port_info", get_port_info());
    notice_java("port_info");
};

void zManager::update_class_loader_info(){
// 收集类加载器信息 - 检测Java层异常
    zManager::getInstance()->update_device_info("class_loader_info", get_class_loader_info());
    notice_java("class_loader_info");
};

void zManager::update_class_info(){
// 收集类信息 - 检测Java层异常
    zManager::getInstance()->update_device_info("class_info", get_class_info());
    notice_java("class_info");
};

void zManager::update_package_info(){
// 收集包信息 - 检测已安装应用异常
    zManager::getInstance()->update_device_info("package_info", get_package_info());
    notice_java("package_info");
};


void zManager::update_system_setting_info(){
// 收集系统设置信息 - 检测系统设置异常
    zManager::getInstance()->update_device_info("system_setting_info", get_system_setting_info());
    notice_java("system_setting_info");
};


void zManager::update_tee_info(){
// 收集TEE信息 - 检测可信执行环境异常
    update_device_info("tee_info", get_tee_info());
    notice_java("tee_info");
};


void zManager::update_time_info(){
// 收集时间信息 - 检测系统时间异常
    zManager::getInstance()->update_device_info("time_info", get_time_info());
    notice_java("time_info");
};

void zManager::update_logcat_info(){
// 检测系统日志
    zManager::getInstance()->update_device_info("logcat_info", get_logcat_info());
    notice_java("logcat_info");
};

// 侧信道检测
void zManager::side_channel_info(){
    zManager::getInstance()->update_device_info("side_channel_info", get_side_channel_info());
    notice_java("side_channel_info");
};


void zManager::update_signature_info(){
    zManager::getInstance()->update_device_info("signature_info", get_signature_info());
    notice_java("signature_info");
};

/**
 * 通知Java层更新UI
 * 
 * 功能说明：
 * 1. 将Native层的检测结果传递给Java层
 * 2. 通过JNI调用Java层的静态方法
 * 3. 实现Native层到Java层的数据通信
 * 4. 触发UI更新显示最新的检测结果
 * 
 * 执行流程：
 * 1. 获取JNI环境
 * 2. 查找MainActivity类
 * 3. 获取回调方法ID
 * 4. 获取检测结果数据
 * 5. 转换为JSON格式
 * 6. 创建Java字符串对象
 * 7. 调用Java层方法
 * 8. 处理异常和清理资源
 * 
 * 线程安全：
 * - 使用静态互斥锁保护整个函数
 * - 确保多线程环境下的安全调用
 * - 避免JNI调用的竞态条件
 * 
 * 错误处理：
 * - 检查JNI环境有效性
 * - 验证类和方法查找结果
 * - 处理Java层异常
 * - 清理JNI局部引用
 * 
 * @param title 检测类型标题，用于标识检测结果
 */
void zManager::notice_java(string title){
    LOGI("notice_java: %s", title.c_str());

    // 添加线程安全保护
    // 使用静态互斥锁确保多线程环境下的安全调用
    static std::mutex notice_java_mutex;
    std::lock_guard<std::mutex> lock(notice_java_mutex);

    LOGI("notice_java: lock by %s ", title.c_str());

    // 获取JNI环境
    JNIEnv *env = zJavaVm::getInstance()->getEnv();
    if(env == nullptr){
        LOGE("notice_java: env is null");
        return;
    }

    // 查找MainActivity类
    jclass activity_class = zJavaVm::getInstance()->findClass("com/example/overt/MainActivity");
    if (activity_class == nullptr) {
        LOGE("notice_java: activity_class is null");
        return;
    }

    // 获取Java层的回调方法ID
    // 方法签名：(Ljava/lang/String;Ljava/lang/String;)V
    // 参数：标题字符串，数据字符串
    // 返回：void
    jmethodID method_id = env->GetStaticMethodID(activity_class, "onCardInfoUpdated", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (method_id == nullptr){
        LOGE("notice_java: method_id onCardInfoUpdated is null");
        return;
    }

    // 使用读锁安全地获取设备信息
    map<string, map<string, string>> card_data = get_info(title);

    // 将C++数据转换为JSON格式
    zJson card_data_json = card_data;
    string card_data_str = card_data_json.dump();
    LOGE("card_data_str:%s", card_data_str.c_str());

    // 创建Java字符串对象 - 标题
    jstring title_jstr = env->NewStringUTF(title.c_str());
    if (title_jstr == nullptr) {
        LOGE("notice_java: Failed to create title string");
        return;
    }

    // 创建Java字符串对象 - 数据
    jstring card_data_jstr = env->NewStringUTF(card_data_str.c_str());
    if (card_data_jstr == nullptr) {
        LOGE("notice_java: Failed to create data string");
        return;
    }

    // 调用Java层方法通知信息更新完成
    LOGI("notice_java: calling onCardInfoUpdated for title: %s", title.c_str());
    env->CallStaticVoidMethod(activity_class, method_id, title_jstr, card_data_jstr);

    // 检查是否抛出异常
    if (env->ExceptionCheck()) {
        LOGE("notice_java: Exception occurred during Java method call");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    // 清理JNI局部引用，防止内存泄漏
    env->DeleteLocalRef(title_jstr);
    env->DeleteLocalRef(card_data_jstr);

    LOGI("notice_java: completed successfully for title: %s", title.c_str());
}

/**
 * 清空所有设备信息
 * 使用写锁保证线程安全
 * 通常在信息返回给Java层后调用，避免重复返回
 */
void zManager::clear_device_info(){
    LOGD("clear_device_info called");
    // 使用独占写锁，确保清空操作的原子性
    std::unique_lock<std::shared_mutex> lock(device_info_mtx_);
    device_info.clear();
    LOGI("clear_device_info");
};

/**
 * 获取大核心CPU列表
 * 
 * 功能说明：
 * 1. 扫描所有CPU核心的频率信息
 * 2. 识别具有最高频率的大核心
 * 3. 返回大核心的ID列表
 * 4. 用于线程性能优化和CPU绑定
 * 
 * 实现原理：
 * - 读取/sys/devices/system/cpu/cpuX/cpufreq/cpuinfo_max_freq文件
 * - 比较所有核心的最大频率
 * - 将具有最高频率的核心标记为大核心
 * - 支持big.LITTLE架构的CPU
 * 
 * 使用场景：
 * - 线程性能优化
 * - CPU亲和性设置
 * - 高优先级任务调度
 * - 性能敏感操作
 * 
 * @return 大核心CPU ID列表
 */
vector<int> zManager::get_big_core_list() {
    LOGI("get_big_core_list called");
    const int cpuCount = std::thread::hardware_concurrency();

    vector<int> big_cores;      // 大核心列表
    int max_freq = 0;           // 最大频率
    vector<int> freqs;          // 各核心频率数组

    LOGD("Scanning CPU frequencies for %d CPUs", cpuCount);
    
    // 扫描所有CPU核心的频率
    for (int cpu = 0; cpu < cpuCount; ++cpu) {
        // 构建频率文件路径
        string path = string_format("/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu);

        LOGV("Checking CPU %d frequency file: %s", cpu, path.c_str());

        // 读取频率文件
        zFile cpuinfo_max_freq_file(path);
        string cpu_freq_str = cpuinfo_max_freq_file.readAllText();
        LOGE("freq_str %s", cpu_freq_str.c_str());

        // 转换为整数频率值
        int cpu_freq = atoi(cpu_freq_str.c_str());
        LOGE("cpu_freq %d", cpu_freq);

        // 存储频率并更新最大值
        freqs[cpu] = cpu_freq;
        if (cpu_freq > max_freq) {
            max_freq = cpu_freq;
            LOGI("New max frequency found: %d kHz (CPU %d)", max_freq, cpu);
        }
    }

    LOGI("Max frequency found: %d kHz", max_freq);
    LOGD("Identifying big cores with max frequency...");

    // 识别具有最大频率的大核心
    for (int i = 0; i < cpuCount; ++i) {
        if (freqs[i] == max_freq) {
            big_cores.push_back(i);
            LOGI("Added CPU %d as big core (freq: %d kHz)", i, freqs[i]);
        }
    }

    LOGI("Found %zu big cores out of %d CPUs", big_cores.size(), cpuCount);
    return big_cores;
}


/**
 * 获取当前线程ID
 * 
 * 功能说明：
 * 1. 使用系统调用获取当前线程的ID
 * 2. 用于线程管理和CPU绑定操作
 * 3. 提供线程标识符
 * 
 * @return 当前线程的ID
 */
pid_t zManager::gettid(){
    return syscall(SYS_gettid);
}

/**
 * 将当前线程绑定到大核心
 * 
 * 功能说明：
 * 1. 识别设备的大核心CPU
 * 2. 将当前线程绑定到大核心上运行
 * 3. 提供性能优化和CPU亲和性设置
 * 4. 支持fallback机制，确保绑定成功
 * 
 * 实现流程：
 * 1. 获取大核心列表
 * 2. 检查大核心是否可用
 * 3. 设置CPU亲和性
 * 4. 验证绑定结果
 * 5. 提供fallback机制
 * 
 * 性能优化：
 * - 优先使用大核心，提高执行效率
 * - 避免在小核心上运行性能敏感任务
 * - 支持big.LITTLE架构优化
 * 
 * 错误处理：
 * - 检查大核心可用性
 * - 提供CPU 0作为fallback
 * - 记录详细的错误信息
 */
void zManager::bind_self_to_least_used_big_core() {
    LOGI("bind_self_to_least_used_big_core called");
    const int cpuCount = std::thread::hardware_concurrency();

    LOGD("Getting list of big cores...");
    vector<int> big_cores = get_big_core_list();

    // 检查是否找到大核心
    if (big_cores.empty()) {
        LOGW("No big cores found, falling back to CPU 0");
        // fallback绑定CPU0
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        pid_t tid = gettid();
        LOGD("Attempting to bind thread %d to fallback CPU 0", tid);

        int ret = sched_setaffinity(tid, sizeof(cpuset), &cpuset);
        if (ret == 0) {
            LOGI("Successfully bound thread %d to fallback CPU 0", tid);
        } else {
            LOGE("Failed to bind thread %d to fallback CPU 0 (errno: %d: %s)",
                 tid, errno, strerror(errno));
        }
        return;
    }

    LOGI("Found %zu big cores: [", big_cores.size());
    for (size_t i = 0; i < big_cores.size(); ++i) {
        if (i > 0) LOGI(", ");
        LOGI("%d", big_cores[i]);
    }
    LOGI("]");

    LOGD("Initializing CPU affinity set for big cores...");
    cpu_set_t set;
    // 初始化清空 CPU 集合，否则可能残留旧数据，可能引发未预期的绑定行为。
    CPU_ZERO(&set);

    // 加入目标 CPU 核心 id
    LOGD("Adding big cores to CPU affinity set:");
    for (size_t i = 0; i < big_cores.size(); ++i) {
        CPU_SET(big_cores[i], &set);
        LOGD("  Added CPU %d to affinity set", big_cores[i]);
    }

    // 将一个进程或线程的调度限制在指定的 CPU 核心集合中运行。
    pid_t tid = gettid();
    LOGD("Attempting to bind thread %d to big core set (size: %zu)", tid, big_cores.size());

    int ret = sched_setaffinity(tid, sizeof(set), &set);
    if (ret == 0) {
        LOGI("Successfully bound thread %d to big core set", tid);

        // 验证绑定结果
        cpu_set_t verify_set;
        CPU_ZERO(&verify_set);
        if (sched_getaffinity(tid, sizeof(verify_set), &verify_set) == 0) {
            LOGD("Verification - current CPU affinity:");
            for (int cpu = 0; cpu < cpuCount; ++cpu) {
                if (CPU_ISSET(cpu, &verify_set)) {
                    LOGD("  CPU %d is in affinity set", cpu);
                }
            }
        } else {
            LOGW("Failed to verify CPU affinity (errno: %d: %s)", errno, strerror(errno));
        }
    } else {
        LOGE("Failed to bind thread %d to big core set (errno: %d: %s)",
             tid, errno, strerror(errno));
        if (errno == EINVAL) {
            LOGE("Invalid CPU set or size");
        } else if (errno == EPERM) {
            LOGE("Permission denied - insufficient privileges");
        }
    }
}

/**
 * 提升线程优先级
 * 
 * 功能说明：
 * 1. 设置当前线程的nice值（优先级）
 * 2. 提供性能优化和任务调度控制
 * 3. 支持实时任务和性能敏感操作
 * 4. 验证设置结果并提供详细反馈
 * 
 * nice值说明：
 * - 范围：-20（最高优先级）到19（最低优先级）
 * - 默认值：0（普通优先级）
 * - 负值：提高优先级，需要root权限
 * - 正值：降低优先级，普通用户可用
 * 
 * 权限要求：
 * - 负nice值需要root权限或CAP_SYS_NICE能力
 * - 正值nice值普通用户可以使用
 * - 权限不足时会记录错误信息
 * 
 * 使用场景：
 * - 性能敏感的安全检测任务
 * - 实时数据处理
 * - 关键系统监控
 * - 高优先级后台任务
 * 
 * @param nice_priority nice值，范围-20到19
 */
void zManager::raise_thread_priority(int nice_priority){
    LOGI("raise_thread_priority called - nice_priority: %d", nice_priority);

    // 验证nice值范围
    if (nice_priority > 19 || nice_priority < -20) {
        LOGE("Invalid nice value: %d (range: -20 to 19)", nice_priority);
        return;
    }

    pid_t tid = gettid();
    LOGD("Current thread ID: %d", tid);

    // 设置线程优先级
    int ret = setpriority(PRIO_PROCESS, tid, nice_priority);
    if (ret != 0) {
        LOGE("setpriority failed for thread %d (errno: %d: %s)", tid, errno, strerror(errno));
        if (errno == EPERM) {
            LOGE("Permission denied: You need root or CAP_SYS_NICE to increase priority (negative nice value)");
        }
    } else {
        // 验证设置结果
        int actual = getpriority(PRIO_PROCESS, tid);
        LOGI("Successfully set thread %d priority to %d (actual nice: %d)", tid, nice_priority, actual);
    }
}

