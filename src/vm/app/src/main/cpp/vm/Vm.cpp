//
// Created by 陈泽伦 on 11/17/20.
//

#include "Vm.h"
#include "../common/Util.h"
#include "../common/VmConstant.h"
#include "../VmContext.h"
#include "JavaException.h"
#include <cmath>

void Vm::callMethod(jobject instance, jmethodID method, jvalue *pResult, ...) {
    va_list args;
    va_start(args, pResult);
    // init vm method context
    VM_CONTEXT::vm->push(instance, method, pResult, args);
    va_end(args);
    // do it
    VM_CONTEXT::vm->run();
    VM_CONTEXT::vm->pop();
}

void Vm::run() {
    if (this->interpret == nullptr) {
        LOG_E("vm::interpret == nullptr");
        throw VMException("vm::interpret == nullptr");
    }
#if defined(VM_DEBUG_FULL)
    this->getCurVMC()->printMethodInsns();
#endif

    RUN_VM_METHOD:
    while (!this->getCurVMC()->isFinish()) {
        if (this->getCurVMC()->curException != nullptr &&
            !JavaException::handleJavaException(this->getCurVMC())) {
            LOG_E("threw exception.");
            break;
        } else if (this->getCurVMC()->isMethodToCall()) {
            if (!Vm::isKeyFunction(this->getCurVMC()->tmp->val_1.u4)) {
                LOG_D_VM("invoke a new function by VmJniMethodCaller.");
                // call method by jni and check exceptions.
                this->jniMethodCaller->call(this->getCurVMC());
                this->getCurVMC()->pc_off(3);
                continue;       // must be continue
            } else {
                LOG_D_VM("invoke a new function by VmKeyMethodCaller.");
                // push the VmMethodContext and
                // build new method's context which is to called.
                this->keyMethodCaller->call(this->getCurVMC());
#if defined(VM_DEBUG_FULL)
                this->getCurVMC()->printMethodInsns();
#endif
            }
        }
        // run opcode.
        this->interpret->run(this->getCurVMC());
    }

    if (this->isCallFromVm()) {
        // pop the current VmMethodContext and
        // resume the old called by this->keyMethodCaller.
        // throw exceptions if has.
        this->pop();
        this->getCurVMC()->pc_off(3);
        this->getCurVMC()->run();
        goto RUN_VM_METHOD;
    }
    // throw uncaught exceptions called by this->jniMethodCaller.
    (*VM_CONTEXT::env).Throw(this->getCurVMC()->curException);
}

void Vm::setInterpret(Interpret *pInterpret) {
    LOG_I("start,  setInterpret.");
    this->interpret = pInterpret;
    LOG_I("finish, setInterpret.");
}

jclass Vm::findPrimitiveClass(const char type) const {
    assert(type != 'V');
    for (int i = 0; i < PRIMITIVE_TYPE_SIZE; i++) {
        if (this->primitiveType[i] == type) {
            LOG_D_VM("found: %c", type);
            return this->primitiveClass[i];
        }
    }
    LOG_E("Unknown primitive type '%c'", type);
    throw VMException(std::string("Unknown primitive type: ") + type);
}

void Vm::initPrimitiveClass() {
    LOG_D_VM("init primitiveClass start");
    char type[3] = {'[', ' ', '\0'};
    JNIEnv *env = VM_CONTEXT::env;
    jclass cArray;
    jclass cClass = (*env).FindClass(VM_REFLECT::C_NAME_Class);
    for (int i = 0; i < PRIMITIVE_TYPE_SIZE; i++) {
        type[1] = this->primitiveType[i];
        LOG_D_VM("get jclass: %s", type);
        cArray = (*env).FindClass(type);
        jmethodID mGetComponentType = (*env).GetMethodID(
                cClass,
                VM_REFLECT::NAME_Class_getComponentType,
                VM_REFLECT::SIGN_Class_getComponentType);
        this->primitiveClass[i] = (jclass) (*env).CallObjectMethod(cArray, mGetComponentType);
        assert(this->primitiveClass[i] != nullptr);
        LOG_D_VM("get jclass: %s, finish.", type);
//        (*env).DeleteLocalRef(cArray);
    }
//    (*env).DeleteLocalRef(cClass);
    LOG_D_VM("init primitiveClass end");
}

bool Vm::isKeyFunction(uint32_t methodId) {
    return VM_CONTEXT::vmKFCFile->getCode(methodId) != nullptr;
}

void Vm::push(jobject caller, jmethodID method, jvalue *pResult, va_list param) {
    this->vmStack->push(caller, method, pResult, param);
}

void Vm::pushWithoutParams(jmethodID method, jvalue *pResult) {
    this->vmStack->pushWithoutParams(method, pResult);
}

void Vm::pop() {
    this->vmStack->pop();
}

VmTempData *Vm::getTempDataBuf() {
    return &this->methodTempData;
}

void Vm::init() {
    LOG_I("VmFrame size of: %lu", sizeof(VmFrame));
    static_assert(sizeof(VmFrame) <= 64u, "too big of VmFrame.");
    // TODO: change vm.Interpret.
    this->interpret = nullptr;
    LOG_I("init this->vmMemory, start.");
    this->vmMemory = new VmRandomMemory(VM_CONFIG::VM_MEMORY_SIZE);
    LOG_I("init this->vmMemory: %p, finish.", this->vmMemory);
    LOG_I("init this->vmStack, start.");
    this->vmStack = new VmRandomStack(VM_CONFIG::VM_STACK_FREE_PAGE_SIZE, this->vmMemory);
    LOG_I("init this->vmStack: %p, finish.", this->vmStack);
    LOG_I("init this->vmCache, start.");
    this->vmCache = new VmLinearCache(this->vmMemory);
    LOG_I("init this->vmCache: %p, finish.", this->vmCache);

    // method's caller
    LOG_I("init method's caller, start.");
    this->keyMethodCaller = new VmKeyMethodCaller();
    this->jniMethodCaller = new VmJniMethodCaller();
    LOG_I("init method's caller, finish.");

    // init
    this->initPrimitiveClass();
}

Vm::~Vm() {
    delete this->interpret;
    delete this->vmStack;
    delete this->vmCache;
    delete this->vmMemory;
    delete this->keyMethodCaller;
    delete this->jniMethodCaller;
}

uint8_t *Vm::mallocCache(uint32_t key, uint32_t count) {
    return this->vmCache->mallocCache(key, count);
}

void Vm::freeCache(uint32_t key, uint32_t count) {
    this->vmCache->freeCache(key, count);
}

uint32_t Vm::newCacheType(uint32_t bufSize) {
    return this->vmCache->newCacheType(bufSize);
}
