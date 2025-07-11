//
// Created by lxz on 2025/4/24.
//

#ifndef DEMO_CLASSLOADER_H
#define DEMO_CLASSLOADER_H

#include <jni.h>
#include <vector>



class zClassLoader {
private:
    zClassLoader();

    zClassLoader(const zClassLoader&) = delete;

    zClassLoader& operator=(const zClassLoader&) = delete;

    static zClassLoader* instance;



public:

    static zClassLoader* getInstance() {
        if (instance == nullptr) {
            instance = new zClassLoader();
        }
        return instance;
    }

    ~zClassLoader();

    std::vector<std::string> classNameList;
    std::vector<std::string> classLoaderStringList;
    bool initialized = false;

    // void debug(JNIEnv *env, const char *format, jobject object);
    // void traverseClassLoader(JNIEnv *env);
    // std::string get_class_loader_info(JNIEnv* env, jobject object);

    void traverseClassLoader(JNIEnv* env);
    void checkGlobalRef(JNIEnv *env, jclass clazz);
    void checkWeakGlobalRef(JNIEnv *env, jclass clazz);


};

#endif //DEMO_CLASSLOADER_H
