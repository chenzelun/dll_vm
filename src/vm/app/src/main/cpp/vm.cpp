#include "common/Util.h"
#include "common/VmContext.h"

#include <jni.h>
#include <cassert>

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
    JNIEnv *env = nullptr;
    if ((*vm).GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }
    assert(env != nullptr);
    VM_CONTEXT.env = env;

    Util::buildFileSystem();

    VmContext::initVmDataFileOfVmContext();

    VmContext::updateNativeLibraryDirectories();
    VmContext::loadDexFromMemory();
    VmContext::changeTopApplication();

    return JNI_VERSION_1_4;
}