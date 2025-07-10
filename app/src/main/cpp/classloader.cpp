//
// Created by lxz on 2025/4/24.
//

#include "classloader.h"

#include <jni.h>
#include <fcntl.h>
#include <unistd.h>
#include "art.h"
#include <vector>
#include <string>

#include <android/log.h>
#include "zLinker.h"
#include "zLog.h"

std::vector<std::string> classNameList = std::vector<std::string>();
std::vector<std::string> classLoaderStringList = std::vector<std::string>();

void debug(C_JNIEnv *env, const char *format, jobject object) {
    if (object == nullptr) {
        LOGE(format, nullptr);
    } else {
        jclass objectClass = (*env)->FindClass((JNIEnv *) env, "java/lang/Object");
        jmethodID toString = (*env)->GetMethodID((JNIEnv *) env, objectClass, "toString", "()Ljava/lang/String;");
        auto string = (jstring) (*env)->CallObjectMethod((JNIEnv *) env, object, toString);
        const char *value = (*env)->GetStringUTFChars((JNIEnv *) env, string, nullptr);
        LOGE(format, value);
        (*env)->ReleaseStringUTFChars((JNIEnv *) env, string, value);
        (*env)->DeleteLocalRef((JNIEnv *) env, string);
        (*env)->DeleteLocalRef((JNIEnv *) env, objectClass);
    }
}

std::string get_class_loader_info(JNIEnv* env, jobject object){
    std::string info = "";
    if (object == nullptr) {
        return info;
    } else {
        jclass objectClass = env->FindClass("java/lang/Object");
        jmethodID toString = env->GetMethodID(objectClass, "toString", "()Ljava/lang/String;");
        auto string = (jstring)env->CallObjectMethod(object, toString);
        const char *value = env->GetStringUTFChars(string, nullptr);
        info = value;
        env->ReleaseStringUTFChars(string, value);
        env->DeleteLocalRef(string);
        env->DeleteLocalRef(objectClass);
    }
    return info;
}

jclass getClass(JNIEnv *env, jobject jobject_) {
    return env->GetObjectClass(jobject_);
}

std::string getClassName(JNIEnv *env, jclass jclass_) {
    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID getNameMethod = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    jstring className = (jstring) env->CallObjectMethod(jclass_, getNameMethod);
    const char *classNameStr = env->GetStringUTFChars(className, nullptr);
    std::string name(classNameStr);
    env->ReleaseStringUTFChars(className, classNameStr);
    env->DeleteLocalRef(className);
    env->DeleteLocalRef(classClass);
    return name;
}

std::string getClassName(JNIEnv *env, jobject jobject_) {
    jclass objectClass = getClass(env, jobject_);
    return getClassName(env, objectClass);
}

std::vector<std::string> getClassNameList(JNIEnv *env, jobject classloader) {
    std::vector<std::string> classNameList = {};

    // 获取 classloader 的类
    jclass classloaderClass = env->GetObjectClass(classloader);

    // 获取 pathList 字段
    jfieldID pathListFieldID = env->GetFieldID(classloaderClass, "pathList","Ldalvik/system/DexPathList;");
    if (pathListFieldID == nullptr) {
        LOGE("Failed to find field 'pathList' in classloader");
        return classNameList;
    }

    jobject pathList = env->GetObjectField(classloader, pathListFieldID);
    if (pathList == nullptr) {
        LOGE("pathList is null");
        return classNameList;
    }

    // 获取 dexElements 字段
    jclass dexPathListClass = env->GetObjectClass(pathList);
    jfieldID dexElementsFieldID = env->GetFieldID(dexPathListClass, "dexElements",
                                                  "[Ldalvik/system/DexPathList$Element;");
    if (dexElementsFieldID == nullptr) {
        LOGE("Failed to find field 'dexElements' in DexPathList");
        return classNameList;
    }

    jobjectArray dexElements = (jobjectArray) env->GetObjectField(pathList, dexElementsFieldID);
    if (dexElements == nullptr) {
        LOGE("dexElements is null");
        return classNameList;
    }

    // 遍历 dexElements 数组
    jint dexElementsLength = env->GetArrayLength(dexElements);
    for (jint i = 0; i < dexElementsLength; i++) {
        jobject dexElement = env->GetObjectArrayElement(dexElements, i);

        // 获取 dexFile 字段
        jclass dexElementClass = env->GetObjectClass(dexElement);
        jfieldID dexFileFieldID = env->GetFieldID(dexElementClass, "dexFile",
                                                  "Ldalvik/system/DexFile;");
        if (dexFileFieldID == nullptr) {
            LOGE("Failed to find field 'dexFile' in DexPathList$Element");
            continue;
        }

        jobject dexFile = env->GetObjectField(dexElement, dexFileFieldID);
        if (dexFile == nullptr) {
            LOGE("dexFile is null");
            continue;
        }

        // 获取 DexFile 的类名列表
        jclass dexFileClass = env->GetObjectClass(dexFile);
        jmethodID entriesMethodID = env->GetMethodID(dexFileClass, "entries", "()Ljava/util/Enumeration;");
        if (entriesMethodID == nullptr) {
            LOGE("Failed to find method 'entries' in DexFile");
            continue;
        }

        jobject entries = env->CallObjectMethod(dexFile, entriesMethodID);
        if (entries == nullptr) {
            LOGE("entries is null");
            continue;
        }

        // 遍历类名
        jclass enumerationClass = env->FindClass("java/util/Enumeration");
        jmethodID hasMoreElementsMethodID = env->GetMethodID(enumerationClass, "hasMoreElements", "()Z");
        jmethodID nextElementMethodID = env->GetMethodID(enumerationClass, "nextElement","()Ljava/lang/Object;");
        while (env->CallBooleanMethod(entries, hasMoreElementsMethodID)) {
            jstring className = (jstring) env->CallObjectMethod(entries, nextElementMethodID);
            const char *classNameStr = env->GetStringUTFChars(className, nullptr);
//            LOGE("Found class: %s", classNameStr);
//            sleep(0);
            classNameList.push_back(classNameStr);
            env->ReleaseStringUTFChars(className, classNameStr);
            env->DeleteLocalRef(className);
        }

        // 清理局部引用
        env->DeleteLocalRef(entries);
        env->DeleteLocalRef(dexFile);
        env->DeleteLocalRef(dexElement);
        env->DeleteLocalRef(dexElementClass);
        env->DeleteLocalRef(dexFileClass);
        env->DeleteLocalRef(enumerationClass);
    }

    // 清理局部引用
    env->DeleteLocalRef(dexElements);
    env->DeleteLocalRef(pathList);
    env->DeleteLocalRef(classloaderClass);

    return classNameList;
}



static jobject newLocalRef(JNIEnv *env, void *object) {
    static jobject (*NewLocalRef)(JNIEnv *, void *) = nullptr;
    if (object == nullptr) {
        return nullptr;
    }
    if (NewLocalRef == nullptr) {
        NewLocalRef = (jobject (*)(JNIEnv *, void *)) (zLinker::getInstance()->find_lib("libart.so").find_symbol("_ZN3art9JNIEnvExt11NewLocalRefEPNS_6mirror6ObjectE"));
    }
    if (NewLocalRef != nullptr) {
        return NewLocalRef(env, object);
    } else {
        return nullptr;
    }
}

static void deleteLocalRef(JNIEnv *env, jobject object) {
    static void (*DeleteLocalRef)(JNIEnv *, jobject) = nullptr;
    if (DeleteLocalRef == nullptr) {
        DeleteLocalRef = (void (*)(JNIEnv *, jobject)) zLinker::getInstance()->find_lib("libart.so").find_symbol("_ZN3art9JNIEnvExt14DeleteLocalRefEP8_jobject");
    }
    if (DeleteLocalRef != nullptr) {
        DeleteLocalRef(env, object);
    }
}


class ClassLoaderVisitor : public art::SingleRootVisitor {
public:
    ClassLoaderVisitor(JNIEnv *env, jclass classLoader) : env_(env), classLoader_(classLoader) {
    }

    void VisitRoot(art::mirror::Object *root, const art::RootInfo &info ATTRIBUTE_UNUSED) final {
        jobject object = newLocalRef(env_, (jobject) root);
        if (object != nullptr) {
            if (env_->IsInstanceOf(object, classLoader_)) {
                std::string classLoaderName = getClassName((JNIEnv *) env_, object);
                LOGE("ClassLoaderVisitor classLoaderName %s", classLoaderName.c_str());
                std::string classLoaderInfo = get_class_loader_info(env_, object);
                LOGE("ClassLoaderVisitor %s", classLoaderInfo.c_str());
                std::vector<std::string> classLoaderClassNameList = getClassNameList((JNIEnv *) env_, object);
                classLoaderStringList.push_back(classLoaderInfo);
                classNameList.insert(classNameList.end(), classLoaderClassNameList.begin(), classLoaderClassNameList.end());
            }else{
                deleteLocalRef(env_, object);
            }
        }
    }

private:
    JNIEnv *env_;
    jclass classLoader_;
};

static void checkGlobalRef(JNIEnv *env, jclass clazz) {
    auto VisitRoots = (void (*)(void *, void *)) zLinker::getInstance()->find_lib("libart.so").find_symbol("_ZN3art9JavaVMExt10VisitRootsEPNS_11RootVisitorE");

    if (VisitRoots == nullptr) {
        return;
    }
    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    ClassLoaderVisitor visitor(env, clazz);
    VisitRoots(jvm, &visitor);
    __android_log_print(6, "lxz", "checkGlobalRef end");
}


class WeakClassLoaderVisitor : public art::IsMarkedVisitor {
public :
    WeakClassLoaderVisitor(JNIEnv *env, jclass classLoader) : env_(env), classLoader_(classLoader) {
    }

    art::mirror::Object *IsMarked(art::mirror::Object *obj) override {
        jobject object = newLocalRef(env_, (jobject) obj);
        if (object != nullptr) {
            if (env_->IsInstanceOf(object, classLoader_)) {
                std::string classLoaderName = getClassName((JNIEnv *) env_, object);
                LOGE("WeakClassLoaderVisitor classLoaderName %s", classLoaderName.c_str());
                std::string classLoaderInfo = get_class_loader_info(env_, object);
                classLoaderStringList.push_back(get_class_loader_info(env_, object));
                LOGE("WeakClassLoaderVisitor %s", classLoaderInfo.c_str());
                std::vector<std::string> classLoaderClassNameList = getClassNameList((JNIEnv *) env_, object);
                classNameList.insert(classNameList.end(), classLoaderClassNameList.begin(), classLoaderClassNameList.end());
            }
            deleteLocalRef(env_, object);
        }
        return obj;
    }

private:
    JNIEnv *env_;
    jclass classLoader_;
};

static void checkWeakGlobalRef(JNIEnv *env, jclass clazz) {
    // auto SweepJniWeakGlobals = (void (*)(void *, void *)) plt_dlsym("_ZN3art9JavaVMExt19SweepJniWeakGlobalsEPNS_15IsMarkedVisitorE", nullptr);
    auto SweepJniWeakGlobals = (void (*)(void *, void *)) zLinker::getInstance()->find_lib("libart.so").find_symbol("_ZN3art9JavaVMExt19SweepJniWeakGlobalsEPNS_15IsMarkedVisitorE");

    if (SweepJniWeakGlobals == nullptr) {
        return;
    }
    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    WeakClassLoaderVisitor visitor(env, clazz);
    SweepJniWeakGlobals(jvm, &visitor);
}

void traverseClassLoader(JNIEnv* env) {
    __android_log_print(6, "lxz", "checkClassloader11");
    char buffer[100];
    __system_property_get("ro.build.version.sdk", buffer);
    int sdk_version = atoi(buffer);

    if (sdk_version < 21) {
        return;
    }

    __android_log_print(6, "lxz", "checkClassloader1");

    jclass clazz = env->FindClass("dalvik/system/BaseDexClassLoader");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }

    __android_log_print(6, "lxz", "checkClassloader2");
    if (clazz == nullptr) {
        __android_log_print(6, "lxz", "checkClassloader3");
        return;
    }

    __android_log_print(6, "lxz", "checkClassloader5");
    checkGlobalRef(env, clazz);

    __android_log_print(6, "lxz", "checkClassloader6");
    checkWeakGlobalRef(env, clazz);

    __android_log_print(6, "lxz", "checkClassloader7");
    env->DeleteLocalRef(clazz);

}
