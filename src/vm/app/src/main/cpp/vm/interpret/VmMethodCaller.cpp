//
// Created by 陈泽伦 on 12/10/20.
//

#include "VmMethodCaller.h"
#include "../JavaException.h"
#include "../../VmContext.h"

void VmJniMethodCaller::call(VmMethodContext *vmc) {
    if (!vmc->isMethodToCall()) {
        LOG_E("error vmc's state");
        throw VMException("error vmc's state");
    }

    const jvalue *params;
    if (vmc->isCallMethodRange()) {
        params = VmJniMethodCaller::pushMethodParamsRange(vmc);
    } else {
        params = VmJniMethodCaller::pushMethodParams(vmc);
    }
    if (vmc->isCallSuperMethod()) {
        VmJniMethodCaller::invokeSuperMethod(vmc, params);
    } else if (vmc->isCallStaticMethod()) {
        VmJniMethodCaller::invokeStaticMethod(vmc, params);
    } else {
        VmJniMethodCaller::invokeMethod(vmc, params);
    }
    delete[] params;

    if ((*VM_CONTEXT::env).ExceptionCheck()) {
        JavaException::throwJavaException(vmc);
    }
    vmc->run();
}

const jvalue *VmJniMethodCaller::pushMethodParams(VmMethodContext *vmc) {
    u2 count = vmc->tmp->src1 >> 4u;
    assert(count <= 5);
    auto *vars = new jvalue[MAX(1, count)]();
    u2 varIdx = 0;
    u2 paramIdx = 0;
    if (!vmc->isCallStaticMethod()) {
        vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->dst & 0x0fu);
        paramIdx++;
        vmc->tmp->dst >>= 4u;
    }
    const char *shorty = vmc->method->dexFile->dexGetMethodShorty(vmc->tmp->val_1.u4);
    LOG_D("method shorty: %s", shorty);
    for (; paramIdx < MIN(count, 4); paramIdx++, varIdx++, vmc->tmp->dst >>= 4u) {
        LOG_D("param[%d]-type: %c", varIdx, shorty[varIdx + 1]);
        switch (shorty[varIdx + 1]) {
            case 'D':
            case 'J':
                vars[varIdx].j = vmc->getRegisterWide(vmc->tmp->dst & 0x0fu);
                vmc->tmp->dst >>= 4u;
                paramIdx++;
                break;

            case 'L':
                vars[varIdx].l = vmc->getRegisterAsObject(vmc->tmp->dst & 0x0fu);
                break;

            default:
                vars[varIdx].i = vmc->getRegister(vmc->tmp->dst & 0x0fu);
                break;
        }
    }
    if (paramIdx == 4 && count == 5) {
        LOG_D("param[%d]-type: %c", varIdx, shorty[varIdx + 1]);
        switch (shorty[varIdx + 1]) {
            case 'L':
                vars[varIdx].l = vmc->getRegisterAsObject(vmc->tmp->src1 & 0x0fu);
                break;

            default:
                vars[varIdx].i = vmc->getRegister(vmc->tmp->src1 & 0x0fu);
                break;
        }
    }
    return vars;
}

const jvalue *VmJniMethodCaller::pushMethodParamsRange(VmMethodContext *vmc) {
    u2 count = vmc->tmp->src1;
    auto *vars = new jvalue[MAX(1, count)]();
    u2 varIdx = 0;
    u2 paramIdx = 0;
    if (!vmc->isCallStaticMethod()) {
        vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->dst + paramIdx);
        paramIdx++;
    }
    const char *shorty = vmc->method->dexFile->dexGetMethodShorty(vmc->tmp->val_1.u4);
    LOG_D("method shorty: %s", shorty);
    for (; paramIdx < count; paramIdx++, varIdx++) {
        LOG_D("param[%d]-type: %c", varIdx, shorty[varIdx + 1]);
        switch (shorty[varIdx + 1]) {
            case 'D':
            case 'J':
                vars[varIdx].j = vmc->getRegisterLong(vmc->tmp->dst + paramIdx);
                paramIdx++;
                break;

            case 'L':
                vars[varIdx].l = vmc->getRegisterAsObject(vmc->tmp->dst + paramIdx);
                break;

            default:
                vars[varIdx].i = vmc->getRegisterInt(vmc->tmp->dst + paramIdx);
                break;
        }
    }
    return vars;
}

void VmJniMethodCaller::invokeStaticMethod(VmMethodContext *vmc, const jvalue *params) {
    JNIEnv *env = VM_CONTEXT::env;
    jmethodID methodToCall = vmc->method->resolveMethod(vmc->tmp->val_1.u4, true);
    if ((*env).ExceptionCheck()) {
        JavaException::throwJavaException(vmc);
        return;
    }

    u4 clazzTypeIdx = vmc->method->dexFile->dexGetMethodId(vmc->tmp->val_1.u4)->classIdx;
    jclass thisClazz = vmc->method->resolveClass(clazzTypeIdx);
    assert(thisClazz != nullptr);

    vmc->callMethodByJni();
    vmc->retVal->j = 0L;
    const char *shorty = vmc->method->dexFile->dexGetMethodShorty(vmc->tmp->val_1.u4);
    switch (shorty[0]) {
        case 'I':
            vmc->retVal->i = (*env).CallStaticIntMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'Z':
            vmc->retVal->z = (*env).CallStaticBooleanMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'B':
            vmc->retVal->b = (*env).CallStaticBooleanMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'S':
            vmc->retVal->s = (*env).CallStaticShortMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'C':
            vmc->retVal->c = (*env).CallStaticCharMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'F':
            vmc->retVal->f = (*env).CallStaticFloatMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'L':
            vmc->retVal->l = (*env).CallStaticObjectMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'D':
            vmc->retVal->d = (*env).CallStaticDoubleMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'J':
            vmc->retVal->j = (*env).CallStaticLongMethodA(
                    thisClazz, methodToCall, params);
            break;

        case 'V':
            (*env).CallStaticVoidMethodA(
                    thisClazz, methodToCall, params);
            break;

        default:
            LOG_E("error method's return type(%s)...", shorty);
            throw VMException("error type of field... cc");
    }

#if defined(VM_DEBUG)
    StandardInterpret::debugInvokeMethod(vmc, methodToCall, shorty, *vmc->retVal, params);
#endif
}

void VmJniMethodCaller::invokeSuperMethod(VmMethodContext *vmc, const jvalue *params) {
    JNIEnv *env = VM_CONTEXT::env;
    jmethodID methodToCall = vmc->method->resolveMethod(vmc->tmp->val_1.u4, false);
    if ((*env).ExceptionCheck()) {
        JavaException::throwJavaException(vmc);
        return;
    }

    jobject thisObj = vmc->tmp->val_2.l;
    if (!JavaException::checkForNull(vmc, thisObj)) {
        return;
    }

    u4 clazzTypeIdx = vmc->method->dexFile->dexGetMethodId(vmc->tmp->val_1.u4)->classIdx;
    jclass thisClazz = vmc->method->resolveClass(clazzTypeIdx);
    assert(thisClazz != nullptr);

    vmc->callMethodByJni();
    vmc->retVal->j = 0L;
    const char *shorty = vmc->method->dexFile->dexGetMethodShorty(vmc->tmp->val_1.u4);
    switch (shorty[0]) {
        case 'I':
            vmc->retVal->i = (*env).CallNonvirtualIntMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'Z':
            vmc->retVal->z = (*env).CallNonvirtualBooleanMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'B':
            vmc->retVal->b = (*env).CallNonvirtualBooleanMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'S':
            vmc->retVal->s = (*env).CallNonvirtualShortMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'C':
            vmc->retVal->c = (*env).CallNonvirtualCharMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'F':
            vmc->retVal->f = (*env).CallNonvirtualFloatMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'L':
            vmc->retVal->l = (*env).CallNonvirtualObjectMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'D':
            vmc->retVal->d = (*env).CallNonvirtualDoubleMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'V':
            (*env).CallNonvirtualVoidMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        case 'J':
            vmc->retVal->j = (*env).CallNonvirtualLongMethodA(
                    thisObj, thisClazz, methodToCall, params);
            break;

        default:
            LOG_E("error method's return type(%s)...", shorty);
            throw VMException("error type of field... cc");
    }

#if defined(VM_DEBUG)
    StandardInterpret::debugInvokeMethod(vmc, methodToCall, shorty, *vmc->retVal, params);
#endif
}

void VmJniMethodCaller::invokeMethod(VmMethodContext *vmc, const jvalue *params) {
    JNIEnv *env = VM_CONTEXT::env;
    jmethodID methodToCall = vmc->method->resolveMethod(vmc->tmp->val_1.u4, false);
    if ((*env).ExceptionCheck()) {
        JavaException::throwJavaException(vmc);
        return;
    }

    jobject thisObj = vmc->tmp->val_2.l;
    if (!JavaException::checkForNull(vmc, thisObj)) {
        return;
    }
    vmc->callMethodByJni();
    vmc->retVal->j = 0L;
    const char *shorty = vmc->method->dexFile->dexGetMethodShorty(vmc->tmp->val_1.u4);
    switch (shorty[0]) {
        case 'I':
            vmc->retVal->i = (*env).CallIntMethodA(thisObj, methodToCall, params);
            break;

        case 'Z':
            vmc->retVal->z = (*env).CallBooleanMethodA(thisObj, methodToCall, params);
            break;

        case 'B':
            vmc->retVal->b = (*env).CallBooleanMethodA(thisObj, methodToCall, params);
            break;

        case 'S':
            vmc->retVal->s = (*env).CallShortMethodA(thisObj, methodToCall, params);
            break;

        case 'C':
            vmc->retVal->c = (*env).CallCharMethodA(thisObj, methodToCall, params);
            break;

        case 'F':
            vmc->retVal->f = (*env).CallFloatMethodA(thisObj, methodToCall, params);
            break;

        case 'L':
            vmc->retVal->l = (*env).CallObjectMethodA(thisObj, methodToCall, params);
            break;

        case 'D':
            vmc->retVal->d = (*env).CallDoubleMethodA(thisObj, methodToCall, params);
            break;

        case 'J':
            vmc->retVal->j = (*env).CallLongMethodA(thisObj, methodToCall, params);
            break;

        case 'V':
            (*env).CallVoidMethodA(thisObj, methodToCall, params);
            break;

        default:
            LOG_E("error method's return type(%s)...", shorty);
            throw VMException("error type of field... cc");
    }
#if defined(VM_DEBUG)
    StandardInterpret::debugInvokeMethod(vmc, methodToCall, shorty, *vmc->retVal, params);
#endif
}


void VmKeyMethodCaller::call(VmMethodContext *vmc) {
    jmethodID methodToCall = vmc->method->resolveMethod(
            vmc->tmp->val_1.u4, vmc->isCallStaticMethod());
    if ((*VM_CONTEXT::env).ExceptionCheck()) {
        JavaException::throwJavaException(vmc);
        return;
    }
    VM_CONTEXT::vm->pushWithoutParams(methodToCall, vmc->retVal);
    VmMethodContext *curVMC = VM_CONTEXT::vm->getCurVMC();
    assert(vmc->isCallStaticMethod() ==
           DexFile::isStaticMethod(curVMC->method->accessFlags));

    if (vmc->isCallMethodRange()) {
        VmKeyMethodCaller::pushMethodParamsRange(vmc, curVMC);
    } else {
        VmKeyMethodCaller::pushMethodParams(vmc, curVMC);
    }
    curVMC->run();
}

void VmKeyMethodCaller::pushMethodParams(const VmMethodContext *src, VmMethodContext *dst) {
    u2 count = src->tmp->src1 >> 4u;
    assert(count <= 5);
    assert(count == dst->method->code->insSize);
    u2 regStart = dst->method->code->registersSize - dst->method->code->insSize;
    u2 regOff = 0;
    if (!src->isCallStaticMethod()) {
        dst->setRegisterAsObject(
                regStart + regOff,
                src->getRegisterAsObject(src->tmp->dst & 0x0fu));
        regOff++;
        src->tmp->dst >>= 4u;
    }
    const char *shorty = src->method->dexFile->dexGetMethodShorty(src->tmp->val_1.u4);
    LOG_D("method shorty: %s", shorty);
    u2 varIdx = 0;
    for (; regOff < MIN(count, 4); regOff++, src->tmp->dst >>= 4u) {
        LOG_D("param[%d]-type: %c", varIdx, shorty[varIdx + 1]);
        switch (shorty[varIdx + 1]) {
            case 'D':
            case 'J':
                dst->setRegisterWide(
                        regStart + regOff,
                        src->getRegisterWide(src->tmp->dst & 0x0fu));
                src->tmp->dst >>= 4u;
                regOff++;
                break;

            case 'L':
                dst->setRegisterAsObject(
                        regStart + regOff,
                        src->getRegisterAsObject(src->tmp->dst & 0x0fu));
                break;

            default:
                dst->setRegister(
                        regStart + regOff,
                        src->getRegister(src->tmp->dst & 0x0fu));
                break;
        }
    }
    if (regOff == 4 && count == 5) {
        LOG_D("param[%d]-type: %c", varIdx, shorty[varIdx + 1]);
        switch (shorty[varIdx + 1]) {
            case 'L':
                dst->setRegisterAsObject(
                        regStart + regOff,
                        src->getRegisterAsObject(src->tmp->src1 & 0x0fu));
                break;

            default:
                dst->setRegister(
                        regStart + regOff,
                        src->getRegister(src->tmp->src2 & 0x0fu));
                break;
        }
    }
}

void VmKeyMethodCaller::pushMethodParamsRange(const VmMethodContext *src, VmMethodContext *dst) {
    u2 count = src->tmp->src1;
    assert(count == dst->method->code->insSize);
    u2 regStart = dst->method->code->registersSize - dst->method->code->insSize;
    for (u2 regOff = 0; regOff < count; regOff++) {
        dst->setRegisterWide(
                regStart + regOff,
                src->getRegisterWide(src->tmp->dst + regOff));
    }
}
