//
// Created by 陈泽伦 on 2020/11/2.
//

#ifndef VM_UTIL_H
#define VM_UTIL_H

#include <android/log.h>

#if defined(SHELL_LOG)
#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG,__FUNCTION__,__VA_ARGS__) // 定义LOGD类型
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR,__FUNCTION__,__VA_ARGS__) // 定义LOGE类型
#else
#define LOG_D(...)
#define LOG_E(...)
#endif

class Util {

};


#endif //VM_UTIL_H
