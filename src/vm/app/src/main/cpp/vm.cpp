#include <jni.h>
#include <cassert>
#include "Util.h"

// global variable
JNIEnv *env = nullptr;

/**
 * get JNIEnv, the version usually is 1.4.
 * init global variable
 * @param vm
 * @param unused
 * @return JNI_VERSION_1_4
 */
jint JNI_OnLoad(JavaVM *vm, void *unused) {
    LOG_D("start, JNI_OnLoad(JavaVM *vm, void* unused)");

    // get JNIEnv
    if ((*vm).GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }

    assert(env != nullptr);

    return JNI_VERSION_1_4;
}