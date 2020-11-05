//
// Created by 陈泽伦 on 2020/11/2.
//

#ifndef VM_UTIL_H
#define VM_UTIL_H

#include "VmContext.h"

#include <jni.h>
#include <android/log.h>
#include <string>

#define SHELL_LOG

#if defined(SHELL_LOG)
#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG,__FUNCTION__,__VA_ARGS__) // 定义LOGD类型
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR,__FUNCTION__,__VA_ARGS__) // 定义LOGE类型
#else
#define LOG_D(...)
#define LOG_E(...)
#endif

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
};


#endif //VM_UTIL_H
