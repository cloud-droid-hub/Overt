//
// Created by lxz on 2025/4/24.
//

#include <jni.h>
#include <fcntl.h>
#include <unistd.h>

#include "art.h"
#include "zJavaVm.h"
#include "zLinker.h"


#include "zLog.h"
#include "zLibc.h"
#include "zStd.h"
#include "zStdUtil.h"
#include "zClassLoader.h"
#include <mutex>

// 静态单例实例指针
zClassLoader* zClassLoader::instance = nullptr;

/**
 * 获取单例实例
 * 采用线程安全的懒加载模式，首次调用时创建实例
 * @return zClassLoader单例指针
 */
zClassLoader* zClassLoader::getInstance() {
    // 使用 std::call_once 确保线程安全的单例初始化
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        try {
            instance = new zClassLoader();
            LOGI("zClassLoader: Created singleton instance");
        } catch (const std::exception& e) {
            LOGE("zClassLoader: Failed to create singleton instance: %s", e.what());
        } catch (...) {
            LOGE("zClassLoader: Failed to create singleton instance with unknown error");
        }
    });
    
    return instance;
}

/**
 * 调试函数：打印Java对象的字符串表示
 * 用于调试时查看Java对象的内容
 * @param env JNI环境指针
 * @param format 日志格式字符串
 * @param object 要调试的Java对象
 */
void debug(JNIEnv *env, const char *format, jobject object) {
    if (object == nullptr) {
        LOGE("%s", format);
    } else {
        // 获取Object类
        jclass objectClass = env->FindClass("java/lang/Object");
        // 获取toString方法
        jmethodID toString = env->GetMethodID(objectClass, "toString", "()Ljava/lang/String;");
        // 调用toString方法获取字符串表示
        auto string = (jstring) env->CallObjectMethod(object, toString);
        const char *value = env->GetStringUTFChars(string, nullptr);
        LOGE("%s", format);
        // 释放字符串资源
        env->ReleaseStringUTFChars(string, value);
        env->DeleteLocalRef(string);
        env->DeleteLocalRef(objectClass);
    }
}

/**
 * 获取类加载器的字符串表示
 * 通过调用toString方法获取类加载器的描述信息
 * @param env JNI环境指针
 * @param object 类加载器对象
 * @return 类加载器的字符串表示
 */
string get_class_loader_string(JNIEnv* env, jobject object){
    LOGD("get_class_loader_string called");
    string info = "";
    if (object == nullptr) {
        return info;
    } else {
        // 获取Object类
        jclass objectClass = env->FindClass("java/lang/Object");
        // 获取toString方法
        jmethodID toString = env->GetMethodID(objectClass, "toString", "()Ljava/lang/String;");
        // 调用toString方法获取字符串表示
        auto string = (jstring)env->CallObjectMethod(object, toString);
        const char *value = env->GetStringUTFChars(string, nullptr);
        info = value;
        // 释放字符串资源
        env->ReleaseStringUTFChars(string, value);
        env->DeleteLocalRef(string);
        env->DeleteLocalRef(objectClass);
    }
    return info;
}

/**
 * 获取Java对象的类对象
 * @param env JNI环境指针
 * @param jobject_ Java对象
 * @return 类对象
 */
jclass getClass(JNIEnv *env, jobject jobject_) {
    LOGD("getClass called");
    return env->GetObjectClass(jobject_);
}

/**
 * 获取类名
 * 通过Class.getName()方法获取类的全限定名
 * @param env JNI环境指针
 * @param jclass_ 类对象
 * @return 类名字符串
 */
string getClassName(JNIEnv *env, jclass jclass_) {
    LOGD("getClassName called");
    // 获取Class类
    jclass classClass = env->FindClass("java/lang/Class");
    // 获取getName方法
    jmethodID getNameMethod = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    // 调用getName方法获取类名
    jstring className = (jstring) env->CallObjectMethod(jclass_, getNameMethod);
    const char *classNameStr = env->GetStringUTFChars(className, nullptr);
    string name(classNameStr);
    // 释放字符串资源
    env->ReleaseStringUTFChars(className, classNameStr);
    env->DeleteLocalRef(className);
    env->DeleteLocalRef(classClass);
    return name;
}

/**
 * 获取Java对象的类名
 * 先获取对象的类，再获取类名
 * @param env JNI环境指针
 * @param jobject_ Java对象
 * @return 类名字符串
 */
string getClassName(JNIEnv *env, jobject jobject_) {
    LOGD("getClassName (object) called");
    jclass objectClass = getClass(env, jobject_);
    return getClassName(env, objectClass);
}

/**
 * 获取类加载器加载的所有类名列表
 * 通过反射访问DexPathList和DexFile获取所有已加载的类
 * @param env JNI环境指针
 * @param classloader 类加载器对象
 * @return 类名列表
 */
vector<string> getClassNameList(JNIEnv *env, jobject classloader) {
    LOGD("getClassNameList called");
    vector<string> classNameList = {};

    // 获取类加载器的类
    jclass classloaderClass = env->GetObjectClass(classloader);

    // 获取pathList字段（DexPathList对象）
    jfieldID pathListFieldID = env->GetFieldID(classloaderClass, "pathList","Ldalvik/system/DexPathList;");
    if (pathListFieldID == nullptr) {
        LOGE("Failed to find field 'pathList' in classloader");
        return classNameList;
    }

    // 获取pathList对象
    jobject pathList = env->GetObjectField(classloader, pathListFieldID);
    if (pathList == nullptr) {
        LOGE("pathList is null");
        return classNameList;
    }

    // 获取dexElements字段（Element数组）
    jclass dexPathListClass = env->GetObjectClass(pathList);
    jfieldID dexElementsFieldID = env->GetFieldID(dexPathListClass, "dexElements",
                                                  "[Ldalvik/system/DexPathList$Element;");
    if (dexElementsFieldID == nullptr) {
        LOGE("Failed to find field 'dexElements' in DexPathList");
        return classNameList;
    }

    // 获取dexElements数组
    jobjectArray dexElements = (jobjectArray) env->GetObjectField(pathList, dexElementsFieldID);
    if (dexElements == nullptr) {
        LOGE("dexElements is null");
        return classNameList;
    }

    // 遍历dexElements数组中的每个Element
    jint dexElementsLength = env->GetArrayLength(dexElements);
    for (jint i = 0; i < dexElementsLength; i++) {
        jobject dexElement = env->GetObjectArrayElement(dexElements, i);

        // 获取dexFile字段（DexFile对象）
        jclass dexElementClass = env->GetObjectClass(dexElement);
        jfieldID dexFileFieldID = env->GetFieldID(dexElementClass, "dexFile",
                                                  "Ldalvik/system/DexFile;");
        if (dexFileFieldID == nullptr) {
            LOGE("Failed to find field 'dexFile' in DexPathList$Element");
            continue;
        }

        // 获取DexFile对象
        jobject dexFile = env->GetObjectField(dexElement, dexFileFieldID);
        if (dexFile == nullptr) {
            LOGE("dexFile is null");
            continue;
        }

        // 获取DexFile的类名列表
        jclass dexFileClass = env->GetObjectClass(dexFile);
        jmethodID entriesMethodID = env->GetMethodID(dexFileClass, "entries", "()Ljava/util/Enumeration;");
        if (entriesMethodID == nullptr) {
            LOGE("Failed to find method 'entries' in DexFile");
            continue;
        }

        // 调用entries方法获取类名枚举
        jobject entries = env->CallObjectMethod(dexFile, entriesMethodID);
        if (entries == nullptr) {
            LOGE("entries is null");
            continue;
        }

        // 遍历枚举中的每个类名
        jclass enumerationClass = env->FindClass("java/util/Enumeration");
        jmethodID hasMoreElementsMethodID = env->GetMethodID(enumerationClass, "hasMoreElements", "()Z");
        jmethodID nextElementMethodID = env->GetMethodID(enumerationClass, "nextElement","()Ljava/lang/Object;");
        
        // 遍历所有类名
        while (env->CallBooleanMethod(entries, hasMoreElementsMethodID)) {
            jstring className = (jstring) env->CallObjectMethod(entries, nextElementMethodID);
            const char *classNameStr = env->GetStringUTFChars(className, nullptr);
//            LOGE("Found class: %s", classNameStr);
//            sleep(0);
            classNameList.push_back(classNameStr);
            env->ReleaseStringUTFChars(className, classNameStr);
            env->DeleteLocalRef(className);
        }

        // 清理局部引用，避免内存泄漏
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

/**
 * 创建局部引用的底层实现
 * 通过动态链接获取ART内部的NewLocalRef函数
 * @param env JNI环境指针
 * @param object 原始对象指针
 * @return 局部引用对象
 */
static jobject newLocalRef(JNIEnv *env, void *object) {
    static jobject (*NewLocalRef)(JNIEnv *, void *) = nullptr;
    if (object == nullptr) {
        return nullptr;
    }
    if (NewLocalRef == nullptr) {
        // 从libart.so中获取NewLocalRef函数指针
        NewLocalRef = (jobject (*)(JNIEnv *, void *)) (zLinker::getInstance()->find_lib("libart.so").find_symbol("_ZN3art9JNIEnvExt11NewLocalRefEPNS_6mirror6ObjectE"));
    }
    if (NewLocalRef != nullptr) {
        return NewLocalRef(env, object);
    } else {
        return nullptr;
    }
}

/**
 * 删除局部引用的底层实现
 * 通过动态链接获取ART内部的DeleteLocalRef函数
 * @param env JNI环境指针
 * @param object 局部引用对象
 */

static void deleteLocalRef(JNIEnv *env, jobject object) {
    static void (*DeleteLocalRef)(JNIEnv *, jobject) = nullptr;
    if (DeleteLocalRef == nullptr) {
        // 从libart.so中获取DeleteLocalRef函数指针
        DeleteLocalRef = (void (*)(JNIEnv *, jobject)) zLinker::getInstance()->find_lib("libart.so").find_symbol("_ZN3art9JNIEnvExt14DeleteLocalRefEP8_jobject");
    }
    if (DeleteLocalRef != nullptr) {
        DeleteLocalRef(env, object);
    }
}

/**
 * 类加载器访问者类
 * 继承自ART的SingleRootVisitor，用于遍历全局引用中的类加载器
 */
class ClassLoaderVisitor : public art::SingleRootVisitor {
public:
    vector<string> classLoaderStringList;  // 类加载器字符串列表
    vector<string> classNameList;          // 类名列表

    /**
     * 构造函数
     * @param env JNI环境指针
     * @param classLoader 目标类加载器类型
     */
    ClassLoaderVisitor(JNIEnv *env, jclass classLoader) : env_(env), classLoader_(classLoader) {
        classLoaderStringList = vector<string>();
        classNameList = vector<string>();
    }

    /**
     * 访问根对象的回调函数
     * 检查每个根对象是否为类加载器，如果是则收集信息
     * @param root 根对象指针
     * @param info 根信息（未使用）
     */
    void VisitRoot(art::mirror::Object *root, const art::RootInfo &info ATTRIBUTE_UNUSED) final {
        // 创建局部引用
        jobject object = newLocalRef(env_, (jobject) root);
        if (object != nullptr) {
            // 检查对象是否为指定类型的类加载器
            if (env_->IsInstanceOf(object, classLoader_)) {
                // 获取类加载器名称
                string classLoaderName = getClassName((JNIEnv *) env_, object);
                LOGD("ClassLoaderVisitor classLoaderName %s", classLoaderName.c_str());
                
                // 获取类加载器字符串表示
                string classLoaderString = get_class_loader_string(env_, object);
                LOGD("ClassLoaderVisitor %s", classLoaderString.c_str());
                
                // 获取该类加载器加载的所有类名
                vector<string> classLoaderClassNameList = getClassNameList((JNIEnv *) env_, object);

                // 添加到结果列表
                classLoaderStringList.push_back(classLoaderString);
                classNameList.insert(classNameList.end(), classLoaderClassNameList.begin(), classLoaderClassNameList.end());

            }else{
                // 不是目标类型，删除局部引用
                deleteLocalRef(env_, object);
            }
        }
    }

private:
    JNIEnv *env_;           // JNI环境指针
    jclass classLoader_;     // 目标类加载器类型
};

/**
 * 检查全局引用中的类加载器
 * 通过ART的VisitRoots机制遍历所有全局引用
 * @param env JNI环境指针
 * @param clazz 目标类加载器类型
 */
void zClassLoader::checkGlobalRef(JNIEnv *env, jclass clazz) {
    LOGD("checkGlobalRef called");
    // 从libart.so中获取VisitRoots函数指针
    auto VisitRoots = (void (*)(void *, void *)) zLinker::getInstance()->find_lib("libart.so").find_symbol("_ZN3art9JavaVMExt10VisitRootsEPNS_11RootVisitorE");

    if (VisitRoots == nullptr) {
        LOGE("Failed to find method 'VisitRoots' in JavaVMExt");
        return;
    }
    
    // 获取JavaVM指针
    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    
    // 创建访问者并开始遍历
    ClassLoaderVisitor visitor(env, clazz);
    VisitRoots(jvm, &visitor);

    // 将访问者收集的结果合并到全局列表
    classLoaderStringList.insert(classLoaderStringList.end(), visitor.classLoaderStringList.begin(), visitor.classLoaderStringList.end());

    auto a = classNameList.end();

    classNameList.insert(classNameList.end(), visitor.classNameList.begin(), visitor.classNameList.end());

    LOGI("ClassLoaderVisitor classLoaderStringList size %zu", visitor.classLoaderStringList.size());
}

/**
 * 弱引用类加载器访问者类
 * 继承自ART的IsMarkedVisitor，用于遍历弱全局引用中的类加载器
 */
class WeakClassLoaderVisitor : public art::IsMarkedVisitor {
public :
    vector<string> classLoaderStringList;  // 类加载器字符串列表
    vector<string> classNameList;          // 类名列表

    /**
     * 构造函数
     * @param env JNI环境指针
     * @param classLoader 目标类加载器类型
     */
    WeakClassLoaderVisitor(JNIEnv *env, jclass classLoader) : env_(env), classLoader_(classLoader) {
        classLoaderStringList = vector<string>();
        classNameList = vector<string>();
    }

    /**
     * 检查对象是否被标记的回调函数
     * 检查每个弱引用对象是否为类加载器，如果是则收集信息
     * @param obj 对象指针
     * @return 原对象指针
     */
    art::mirror::Object *IsMarked(art::mirror::Object *obj) override {
        // 创建局部引用
        jobject object = newLocalRef(env_, (jobject) obj);
        if (object != nullptr) {
            // 检查对象是否为指定类型的类加载器
            if (env_->IsInstanceOf(object, classLoader_)) {
                // 获取类加载器名称
                string classLoaderName = getClassName(env_, object);
                LOGD("WeakClassLoaderVisitor classLoaderName %s", classLoaderName.c_str());
                
                // 获取类加载器字符串表示
                string classLoaderSting = get_class_loader_string(env_, object);
                classLoaderStringList.push_back(get_class_loader_string(env_, object));
                LOGD("WeakClassLoaderVisitor %s", classLoaderSting.c_str());
                
                // 获取该类加载器加载的所有类名
                vector<string> classLoaderClassNameList = getClassNameList(env_, object);
                classNameList.insert(classNameList.end(), classLoaderClassNameList.begin(), classLoaderClassNameList.end());
            }
            // 删除局部引用
            deleteLocalRef(env_, object);
        }
        return obj;
    }

private:
    JNIEnv *env_;           // JNI环境指针
    jclass classLoader_;     // 目标类加载器类型
};

/**
 * 检查弱全局引用中的类加载器
 * 通过ART的SweepJniWeakGlobals机制遍历所有弱全局引用
 * @param env JNI环境指针
 * @param clazz 目标类加载器类型
 */
void zClassLoader::checkWeakGlobalRef(JNIEnv *env, jclass clazz) {
    LOGD("checkWeakGlobalRef called");
    // 从libart.so中获取SweepJniWeakGlobals函数指针
    auto SweepJniWeakGlobals = (void (*)(void *, void *)) zLinker::getInstance()->find_lib("libart.so").find_symbol("_ZN3art9JavaVMExt19SweepJniWeakGlobalsEPNS_15IsMarkedVisitorE");

    if (SweepJniWeakGlobals == nullptr) {
        return;
    }
    
    // 获取JavaVM指针
    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    
    // 创建访问者并开始遍历
    WeakClassLoaderVisitor visitor(env, clazz);
    SweepJniWeakGlobals(jvm, &visitor);
    
    // 将访问者收集的结果合并到全局列表
    classLoaderStringList.insert(classLoaderStringList.end(), visitor.classLoaderStringList.begin(), visitor.classLoaderStringList.end());
    classNameList.insert(classNameList.end(), visitor.classNameList.begin(), visitor.classNameList.end());
    LOGI("WeakClassLoaderVisitor classLoaderStringList size %zu", visitor.classLoaderStringList.size());
}

/**
 * 遍历系统中的类加载器
 * 检查全局引用和弱全局引用中的所有类加载器
 * @param env JNI环境指针
 */
void zClassLoader::traverseClassLoader(JNIEnv* env) {
    LOGD("traverseClassLoader called");

    if (env == nullptr){
        LOGE("traverseClassLoader env is null");
        return;
    }

    // 检查SDK版本，只支持API 21及以上
    char buffer[100];
    __system_property_get("ro.build.version.sdk", buffer);
    int sdk_version = atoi(buffer);

    if (sdk_version < 21) {
        LOGE("traverseClassLoader sdk_version < 21");
        return;
    }

    LOGI("traverseClassLoader start");

    // 获取BaseDexClassLoader类
    jclass clazz = env->FindClass("dalvik/system/BaseDexClassLoader");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }

    if (clazz == nullptr) {
        LOGE("traverseClassLoader clazz is null");
        return;
    }

    // 检查全局引用中的类加载器
    LOGI("traverseClassLoader checkGlobalRef");
    checkGlobalRef(env, clazz);

    // 检查弱全局引用中的类加载器
    LOGI("traverseClassLoader checkWeakGlobalRef");
    checkWeakGlobalRef(env, clazz);

    LOGI("traverseClassLoader end");
    env->DeleteLocalRef(clazz);
}

/**
 * zClassLoader构造函数
 * 初始化类加载器检测器，开始遍历系统中的类加载器
 */
zClassLoader::zClassLoader(){
    LOGD("zClassLoader constructor called");
    classNameList = vector<string>();
    classLoaderStringList = vector<string>();
    // 开始遍历类加载器
    traverseClassLoader(zJavaVm::getInstance()->getEnv());
}

/**
 * zClassLoader析构函数
 * 清理资源
 */
zClassLoader::~zClassLoader() {
    LOGD("zClassLoader destructor called");
}

