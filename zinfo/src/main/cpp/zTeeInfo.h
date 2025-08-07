//
// Created by lxz on 2025/7/17.
//

#ifndef OVERT_TEE_INFO_H
#define OVERT_TEE_INFO_H

#include <jni.h>
#include "zStd.h"

/**
 * 获取TEE（可信执行环境）信息
 * 通过Android KeyStore获取认证证书，分析TEE扩展信息
 * 检测设备锁定状态和验证启动状态
 * @return 包含TEE检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_tee_info();

/**
 * 获取TEE信息的重载函数
 * 允许外部传入JNI环境和上下文对象
 * @param env JNI环境指针
 * @param context Android上下文对象
 * @return 包含TEE检测结果的Map，格式：{检测项目 -> {风险等级, 说明}}
 */
map<string, map<string, string>> get_tee_info(JNIEnv* env, jobject context);

#endif //OVERT_TEE_INFO_H
