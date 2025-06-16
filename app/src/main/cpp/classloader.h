//
// Created by lxz on 2025/4/24.
//

#ifndef DEMO_CLASSLOADER_H
#define DEMO_CLASSLOADER_H

#include <jni.h>
#include <vector>

extern std::vector<std::string> classNameList;
extern std::vector<std::string> classLoaderStringList;

void debug(C_JNIEnv *env, const char *format, jobject object);
void traverseClassLoader(JNIEnv *env);
std::string get_class_loader_info(JNIEnv* env, jobject object);

#endif //DEMO_CLASSLOADER_H
