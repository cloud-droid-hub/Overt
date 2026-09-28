//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_SYSTEM_SETTING_INFO_H
#define OVERT_SYSTEM_SETTING_INFO_H

#include "zStd.h"


map<string, map<string, string>> get_system_setting_info();

// 查杀分离重构: 采集端按数据源分组上报原始键值对,判定全在 zengine SystemSettingAnalyzer。
// info 结构(外层键=数据源组名, 值=组内扁平键值对 <string,string>):
//   settings_info["global:adb_enabled"]="1"       // 三表合并,键带表前缀
//   settings_info["secure:development_settings_enabled"]="1"
//   battery_info["status"]="2"  battery_info["level"]="50"
//   telephony_info["getSimState"]="5"  telephony_info["getSimOperator"]="46000"
//   network_info["0"]="0,1,4"        // 逐网络 transport int 逗号串(不做 VPN 判定)
//   biometric_info["fingerprint_feature"]="1"  biometric_info["biometric_status"]="0"
//   installer_info["installer"]="com.xxx"       // ADB 安装则为空组,不写 key
//   keyguard_info["password"]="1"
map<string, map<string, string>> get_system_setting_info(JNIEnv* env, jobject context);

#endif //OVERT_SYSTEM_SETTING_INFO_H