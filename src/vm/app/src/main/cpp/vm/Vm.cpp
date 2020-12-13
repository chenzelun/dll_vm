//
// Created by 陈泽伦 on 11/17/20.
//

#include "Vm.h"
#include "../common/Util.h"
#include "../common/VmConstant.h"
#include "../VmContext.h"
#include "interpret/StandardInterpret.h"
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
#if defined(VM_DEBUG)
    vmc->printMethodInsns();
#endif

    if (this->interpret == nullptr) {
        LOG_D("vm::interpret == nullptr");
        throw VMException("vm::interpret == nullptr");
    }

    RUN_VM_METHOD:
    while (!this->getCurVMC()->isFinish()) {
        if (this->getCurVMC()->curException != nullptr &&
            !JavaException::handleJavaException(this->getCurVMC())) {
            LOG_D("threw exception.");
            break;
        } else if (this->getCurVMC()->isMethodToCall()) {
            LOG_D("invoke a new function.");
            if (!Vm::isKeyFunction(this->getCurVMC()->tmp->val_1.u4)) {
                // call method by jni and check exceptions.
                this->jniMethodCaller->call(this->getCurVMC());
                this->getCurVMC()->pc_off(3);
                continue;       // must be continue
            } else {
                // push the VmMethodContext and
                // build new method's context which is to called.
                this->keyMethodCaller->call(this->getCurVMC());
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
            LOG_D("found: %c", type);
            return this->primitiveClass[i];
        }
    }
    LOG_E("Unknown primitive type '%c'", type);
    throw VMException(std::string("Unknown primitive type: ") + type);
}

void Vm::initPrimitiveClass() {
    LOG_D("init primitiveClass start");
    char type[3] = {'[', ' ', '\0'};
    JNIEnv *env = VM_CONTEXT::env;
    jclass cArray;
    jclass cClass = (*env).FindClass(VM_REFLECT::C_NAME_Class);
    for (int i = 0; i < PRIMITIVE_TYPE_SIZE; i++) {
        type[1] = this->primitiveType[i];
        LOG_D("get jclass: %s", type);
        cArray = (*env).FindClass(type);
        jmethodID mGetComponentType = (*env).GetMethodID(
                cClass,
                VM_REFLECT::NAME_Class_getComponentType,
                VM_REFLECT::SIGN_Class_getComponentType);
        this->primitiveClass[i] = (jclass) (*env).CallObjectMethod(cArray, mGetComponentType);
        assert(this->primitiveClass[i] != nullptr);
        LOG_D("get jclass: %s, finish.", type);
//        (*env).DeleteLocalRef(cArray);
    }
//    (*env).DeleteLocalRef(cClass);
    LOG_D("init primitiveClass end");
}

bool Vm::isKeyFunction(uint32_t methodId) {
    return VM_CONTEXT::vmKFCFile->getCode(methodId) != nullptr;
}

void Vm::push(jobject caller, jmethodID method, jvalue *pResult, va_list param) {
    this->stackManager->push(caller, method, pResult, param);
}

void Vm::pushWithoutParams(jmethodID method, jvalue *pResult) {
    this->stackManager->pushWithoutParams(method, pResult);
}

void Vm::pop() {
    this->stackManager->pop();
}

VmTempData *Vm::getTempDataBuf() {
    memset(&this->methodTempData, 0, sizeof(this->methodTempData));
    return &this->methodTempData;
}

uint8_t *Vm::malloc() {
    return this->vmMemory->malloc();
}

void Vm::free(void *p) {
    this->vmMemory->free(p);
}

void Vm::init() {
    LOG_D("VmFrame size of: %lu", sizeof(VmFrame));
    static_assert(sizeof(VmFrame) <= 64u, "too big of VmFrame.");
    this->vmMemory = new VmRandomMemory(VM_CONFIG::VM_MEMORY_SIZE);
    // TODO: change vm.Interpret.
    this->interpret = nullptr;
    this->stackManager = new VmRandomStack(VM_CONFIG::VM_STACK_FREE_PAGE_SIZE);
    this->initPrimitiveClass();
    this->keyMethodCaller = new VmKeyMethodCaller();
    this->jniMethodCaller = new VmJniMethodCaller();
}

Vm::~Vm() {
    delete this->interpret;
    delete this->stackManager;
    delete this->vmMemory;
    delete this->keyMethodCaller;
    delete this->jniMethodCaller;
}
