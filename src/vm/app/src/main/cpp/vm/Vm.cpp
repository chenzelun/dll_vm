//
// Created by 陈泽伦 on 11/17/20.
//

#include "Vm.h"
#include "../common/Util.h"
#include "../common/VmConstant.h"
#include "../VmContext.h"
#include "../Interpret/StandardInterpret.h"
#include "JAVAException.h"
#include <cmath>

void Vm::callMethod(jobject instance, jmethodID method, jvalue *pResult, ...) {
    // init vm method from dex file.
    VmMethod vmMethod(method);
    vmMethod.updateCode();
    va_list args;
    va_start(args, pResult);
    // init vm method context
    VmMethodContext vmc(instance, &vmMethod, pResult, args);
    va_end(args);
    // do it
    VM_CONTEXT::vm->run(&vmc);
}

Vm::Vm() {
    // TODO: change Interpret.
    this->interpret = nullptr;
    this->initPrimitiveClass();
}

void Vm::setInterpret(Interpret *pInterpret) {
    this->interpret = pInterpret;
}

void Vm::run(VmMethodContext *vmc) const {
#ifdef VM_DEBUG
    vmc->printMethodInsns();
#endif

    while (!vmc->isFinish()) {
        if (vmc->curException != nullptr && !JAVAException::handleJavaException(vmc)) {
            LOG_D("threw exception.");
            (*VM_CONTEXT::env).Throw(vmc->curException);
            return;
        }
        this->interpret->run(vmc);
    }
    LOG_D("exit vm success.");
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
