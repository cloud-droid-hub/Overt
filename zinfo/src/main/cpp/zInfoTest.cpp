//
// Created by lxz on 2025/8/7.
//
#include "zLog.h"
#include "zLibc.h"
#include "zLibcUtil.h"
#include "zStd.h"
#include "zFile.h"
#include "zLinker.h"
#include "zJavaVm.h"
#include "zJson.h"
#include "zBroadCast.h"

#include "zLocalNetworkInfo.h"
#include "zRootFileInfo.h"
#include "zSystemPropInfo.h"
#include "zLinkerInfo.h"
#include "zPortInfo.h"
#include "zClassLoaderInfo.h"
#include "zPackageInfo.h"
#include "zSystemSettingInfo.h"
#include "zTeeInfo.h"
#include "zTimeInfo.h"
#include "zSslInfo.h"
#include "zProcInfo.h"

void __attribute__((constructor)) init_(void){
    LOGI("zCore init - Starting comprehensive tests");

//    // 收集SSL信息 - 检测SSL证书异常
//    get_ssl_info();
//
//    // 收集本地网络信息 - 检测同一网络中的其他Overt设备
//    get_local_network_info();
//
//    // 收集任务信息 - 检测Frida等调试工具注入的进程
//    get_task_info();
//
//    // 收集内存映射信息 - 检测关键系统库是否被篡改
//    get_maps_info();
//
//    // 收集Root文件信息 - 检测Root相关文件
//    get_root_file_info();
//
//    // 收集挂载点信息 - 检测异常的文件系统挂载
    get_proc_info();
//
//    // 收集系统属性信息 - 检测系统配置异常
//    get_system_prop_info();

    // 收集链接器信息 - 检测动态链接库加载异常
//    get_linker_info();
//
//    // 收集端口信息 - 检测网络端口异常
//    get_port_info();
//
//    // 收集类加载器信息 - 检测Java层异常
//    get_class_loader_info();
//
//    // 收集包信息 - 检测已安装应用异常
//    get_package_info();
//
//    // 收集系统设置信息 - 检测系统设置异常
//    get_system_setting_info();
//
//    // 收集TEE信息 - 检测可信执行环境异常
//    get_tee_info();
//
//    // 收集时间信息 - 检测系统时间异常
//    get_time_info();


    LOGI("zCore init - All tests completed successfully");
}