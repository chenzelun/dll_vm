//
// Created by 陈泽伦 on 2020/11/2.
//

#ifndef VM_UTIL_H
#define VM_UTIL_H

#include <jni.h>
#include <android/log.h>
#include <string>

//#define VM_NEW_CODE
#define VM_DEBUG
//#define VM_INFO

#if defined(VM_NEW_CODE)

#define LOG_D_NEW(...) __android_log_print(ANDROID_LOG_DEBUG,__FUNCTION__,__VA_ARGS__)
#define LOG_I_NEW(...) __android_log_print(ANDROID_LOG_INFO ,__FUNCTION__,__VA_ARGS__)
#define LOG_W_NEW(...) __android_log_print(ANDROID_LOG_WARN ,__FUNCTION__,__VA_ARGS__)
#define LOG_E_NEW(...) __android_log_print(ANDROID_LOG_ERROR,__FUNCTION__,__VA_ARGS__)

#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG,__FUNCTION__,__VA_ARGS__)
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO ,__FUNCTION__,__VA_ARGS__)
#define LOG_W(...) __android_log_print(ANDROID_LOG_WARN ,__FUNCTION__,__VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR,__FUNCTION__,__VA_ARGS__)

#elif defined(VM_DEBUG)

#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG,__FUNCTION__,__VA_ARGS__)
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO ,__FUNCTION__,__VA_ARGS__)
#define LOG_W(...) __android_log_print(ANDROID_LOG_WARN ,__FUNCTION__,__VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR,__FUNCTION__,__VA_ARGS__)

#elif defined(VM_INFO)

#define LOG_D(...)
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO ,__FUNCTION__,__VA_ARGS__)
#define LOG_W(...) __android_log_print(ANDROID_LOG_WARN ,__FUNCTION__,__VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR,__FUNCTION__,__VA_ARGS__)

#else

#define LOG_D(...)
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)

#endif

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))


class Util {
public:
    static std::string &getBaseFilesDir();

    static std::string &getDataDir();

    static std::string &getLibDir();

    static std::string &getOdexDir();

    static std::string &getDexDir();

    static void buildFileSystem();

    static const uint8_t *getFileBufFromAssets(const std::string &fileName, uint32_t &buf_size);

    static jobject getAppContext();

    constexpr static const char *getCUP_ABI();

    static jobject getClassLoader();

    static bool
    HookNativeInline(const char *soPath, const char *signature, void *my_func, void **ori_func);

    static bool
    HookNativeInlineAnonymous(const char *soPath, uint64_t addr, void *my_func, void **ori_func);

    static bool
    HookJava(JNIEnv *env, const char *clazzPath, const char *methodName,
             const char *methodSignature, const void *my_func, jmethodID *ori_func);
};


#endif //VM_UTIL_H
