//
// Created by lxz on 2025/7/17.
//

#ifndef OVERT_TEE_INFO_H
#define OVERT_TEE_INFO_H

#include <jni.h>
#include "config.h"

map<string, map<string, string>> get_tee_info();
map<string, map<string, string>> get_tee_info(JNIEnv* env, jobject context);


#endif //OVERT_TEE_INFO_H
