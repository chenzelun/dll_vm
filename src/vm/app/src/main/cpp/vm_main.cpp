#include "common/Util.h"
#include "VmContext.h"
#include "vm/interpret/StandardInterpret.h"

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
    VM_CONTEXT::env = env;

    Util::buildFileSystem();

    // init VM_CONTEXT start
    VM_CONTEXT::initVmDataFileOfVC();
    VM_CONTEXT::initVmKeyFuncCodeFileOfVC(); // may be sub process.

    VM_CONTEXT::initVm();
    VM_CONTEXT::vm->setInterpret(new StandardInterpret());
    // init VM_CONTEXT end


    VM_CONTEXT::loadDexFromMemory();
    VM_CONTEXT::changeTopApplication();
    LOG_D("VM init success.");
    return JNI_VERSION_1_4;
}