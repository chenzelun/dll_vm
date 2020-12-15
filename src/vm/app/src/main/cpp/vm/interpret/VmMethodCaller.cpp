//
// Created by 陈泽伦 on 12/10/20.
//

#include "VmMethodCaller.h"
#include "../JavaException.h"
#include "../../VmContext.h"
#include "../interpret/StandardInterpret.h"

uint32_t VmJniMethodCaller::cacheKey = 0;

void VmJniMethodCaller::call(VmMethodContext *vmc) {
    if (!vmc->isMethodToCall()) {
        LOG_E("error vmc's state");
        throw VMException("error vmc's state");
    }

    const jvalue *params;
    uint32_t paramCount;
    if (vmc->isCallMethodRange()) {
        params = VmJniMethodCaller::pushMethodParamsRange(vmc, paramCount);
    } else {
        params = VmJniMethodCaller::pushMethodParams(vmc, paramCount);
    }
    if (vmc->isCallSuperMethod()) {
        VmJniMethodCaller::invokeSuperMethod(vmc, params);
    } else if (vmc->isCallStaticMethod()) {
        VmJniMethodCaller::invokeStaticMethod(vmc, params);
    } else {
        VmJniMethodCaller::invokeMethod(vmc, params);
    }
    assert(params != nullptr);
    assert(paramCount != 0);
    VM_CONTEXT::vm->freeCache(VmJniMethodCaller::cacheKey, paramCount);

    if ((*VM_CONTEXT::env).ExceptionCheck()) {
        JavaException::throwJavaException(vmc);
    }
    vmc->run();
}

const jvalue *VmJniMethodCaller::pushMethodParams(
        VmMethodContext *vmc, uint32_t &paramCount) {
    u2 count = vmc->tmp->src1 >> 4u;
    assert(count <= 5);
    paramCount = MAX(1, count);
    auto *vars = (jvalue *) VM_CONTEXT::vm->mallocCache(VmJniMethodCaller::cacheKey, paramCount);
    u2 varIdx = 0;
    u2 paramIdx = 0;
    if (!vmc->isCallStaticMethod()) {
        vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->dst & 0x0fu);
        paramIdx++;
        vmc->tmp->dst >>= 4u;
    }
    const char *shorty = vmc->method->dexFile->dexGetMethodShorty(vmc->tmp->val_1.u4);
    LOG_D_VM("method shorty: %s", shorty);
    for (; paramIdx < MIN(count, 4); paramIdx++, varIdx++, vmc->tmp->dst >>= 4u) {
        LOG_D_VM("param[%d]-type: %c", varIdx, shorty[varIdx + 1]);
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
        LOG_D_VM("param[%d]-type: %c", varIdx, shorty[varIdx + 1]);
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

const jvalue *VmJniMethodCaller::pushMethodParamsRange(
        VmMethodContext *vmc, uint32_t &paramCount) {
    u2 count = vmc->tmp->src1;
    paramCount = MAX(1, count);
    auto *vars = (jvalue *) VM_CONTEXT::vm->mallocCache(VmJniMethodCaller::cacheKey, paramCount);
    u2 varIdx = 0;
    u2 paramIdx = 0;
    if (!vmc->isCallStaticMethod()) {
        vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->dst + paramIdx);
        paramIdx++;
    }
    const char *shorty = vmc->method->dexFile->dexGetMethodShorty(vmc->tmp->val_1.u4);
    LOG_D_VM("method shorty: %s", shorty);
    for (; paramIdx < count; paramIdx++, varIdx++) {
        LOG_D_VM("param[%d]-type: %c", varIdx, shorty[varIdx + 1]);
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
#if defined(VM_DEBUG_FULL)
    VmJniMethodCaller::debugInvokeMethod(vmc, methodToCall, shorty, *vmc->retVal, params);
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
#if defined(VM_DEBUG_FULL)
    VmJniMethodCaller::debugInvokeMethod(vmc, methodToCall, shorty, *vmc->retVal, params);
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
#if defined(VM_DEBUG_FULL)
    VmJniMethodCaller::debugInvokeMethod(vmc, methodToCall, shorty, *vmc->retVal, params);
#endif
}

#if defined(VM_DEBUG_FULL)

void
VmJniMethodCaller::debugInvokeMethod(VmMethodContext *vmc, jmethodID methodCalled,
                                     const char *shorty, const jvalue retVal,
                                     const jvalue *params) {
    LOG_D("methodCalled info:");
    VmMethod vmMethod{};
    vmMethod.reset(methodCalled, false);
    LOG_D("methodCalled: %s#%s", vmMethod.clazzDescriptor, vmMethod.name);
    LOG_D("methodCalled sign: %s", vmMethod.resolveMethodSign(
            vmMethod.dexFile->dexGetMethodId(vmMethod.method_id)->protoIdx).data());
    LOG_D("");
    LOG_D("VmMethodContext info-caller:");
    vmc->printVmMethodContext();
    switch (shorty[0]) {
        case 'I':
            LOG_D("return value (int): %d", retVal.i);
            break;

        case 'Z':
            retVal.z ? LOG_D("return value (bool): true")
                     : LOG_D("return value (bool): false");
            break;

        case 'B':
            LOG_D("return value (byte): 0x%02x", retVal.b);
            break;

        case 'S':
            LOG_D("return value (short): %d", retVal.s);
            break;

        case 'C':
            LOG_D("return value (char): %c", retVal.c);
            break;

        case 'F':
            LOG_D("return value (float): %f", retVal.f);
            break;

        case 'L':
            LOG_D("return value (object): %p", retVal.l);
            break;

        case 'J':
            LOG_D("return value (long): %ld", retVal.j);
            break;

        case 'D':
            LOG_D("return value (double): %lf", retVal.d);
            break;

        case 'V':
            LOG_D("return value (void)");
            break;

        default:
            LOG_E("error method's return type(%s)...", shorty);
            assert(false);
            break;
    }

    for (int var_i = 0; shorty[var_i + 1] != '\0'; var_i++) {
        switch (shorty[var_i + 1]) {
            case 'I':
                LOG_D("var(%d) value (int): %d", var_i, params[var_i].i);
                break;

            case 'Z':
                params[var_i].z ? LOG_D("var(%d) value (bool): true", var_i)
                                : LOG_D("var(%d) value (bool): false", var_i);
                break;

            case 'B':
                LOG_D("var(%d) value (byte): 0x%02x", var_i, params[var_i].b);
                break;

            case 'S':
                LOG_D("var(%d) value (short): %d", var_i, params[var_i].s);
                break;

            case 'C':
                LOG_D("var(%d) value (char): %c", var_i, params[var_i].c);
                break;

            case 'F':
                LOG_D("var(%d) value (float): %f", var_i, params[var_i].f);
                break;

            case 'L':
                LOG_D("var(%d) value (object): %p", var_i, params[var_i].l);
                break;

            case 'J':
                LOG_D("var(%d) value (long): %ld", var_i, params[var_i].j);
                break;

            case 'D':
                LOG_D("var(%d) value (double): %lf", var_i, params[var_i].d);
                break;

            default:
                LOG_E("error method's param type(%s)...", shorty);
                assert(false);
                break;
        }
    }
}

#endif

VmJniMethodCaller::VmJniMethodCaller() {
    if (VmJniMethodCaller::cacheKey == 0) {
        VmJniMethodCaller::cacheKey = VM_CONTEXT::vm->newCacheType(sizeof(jvalue));
    }
    assert(VmJniMethodCaller::cacheKey != 0);
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
#if defined(VM_DEBUG_FULL)
    VmKeyMethodCaller::printMethodParam(vmc, curVMC);
#endif
    curVMC->run();
}

void VmKeyMethodCaller::pushMethodParams(const VmMethodContext *src, VmMethodContext *dst) {
    LOG_D_VM("pushMethodParams, start.");
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
    LOG_D_VM("method shorty: %s", shorty);
    u2 varIdx = 0;
    for (; regOff < MIN(count, 4); varIdx++, regOff++, src->tmp->dst >>= 4u) {
        switch (shorty[varIdx + 1]) {
            case 'D':
            case 'J':
                LOG_D_VM("param[%d]-type: %c, in-value: %lu, in-reg: %u",
                         varIdx, shorty[varIdx + 1],
                         src->getRegisterWide(src->tmp->dst & 0x0fu),
                         src->tmp->dst & 0x0fu);
                dst->setRegisterWide(
                        regStart + regOff,
                        src->getRegisterWide(src->tmp->dst & 0x0fu));
                LOG_D_VM("param[%d]-type: %c, value: %lu",
                         varIdx, shorty[varIdx + 1], dst->getRegisterWide(regStart + regOff));
                regOff++;
                src->tmp->dst >>= 4u;
                break;

            case 'L':
                LOG_D_VM("param[%d]-type: %c, in-value: %p, in-reg: %u",
                         varIdx, shorty[varIdx + 1],
                         src->getRegisterAsObject(src->tmp->dst & 0x0fu),
                         src->tmp->dst & 0x0fu);
                dst->setRegisterAsObject(
                        regStart + regOff,
                        src->getRegisterAsObject(src->tmp->dst & 0x0fu));
                LOG_D_VM("param[%d]-type: %c, value: %p",
                         varIdx, shorty[varIdx + 1], dst->getRegisterAsObject(regStart + regOff));
                break;

            default:
                LOG_D_VM("param[%d]-type: %c, in-value: 0x%08x, in-reg: %u",
                         varIdx, shorty[varIdx + 1],
                         src->getRegister(src->tmp->dst & 0x0fu),
                         src->tmp->dst & 0x0fu);
                dst->setRegister(
                        regStart + regOff,
                        src->getRegister(src->tmp->dst & 0x0fu));
                LOG_D_VM("param[%d]-type: %c, value: %u",
                         varIdx, shorty[varIdx + 1], dst->getRegister(regStart + regOff));
                break;
        }
    }
    if (regOff == 4 && count == 5) {
        LOG_D_VM("param[%d]-type: %c, in-value: 0x%016lx, in-reg: %u",
                 varIdx, shorty[varIdx + 1],
                 src->getRegisterWide(src->tmp->src1 & 0x0fu),
                 src->tmp->src1 & 0x0fu);
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
        LOG_D_VM("param[%d]-type: %c, value: 0x%016lx",
                 varIdx, shorty[varIdx + 1], dst->getRegisterWide(regStart + regOff));
    }
    LOG_D_VM("pushMethodParams, finish.");
}

void VmKeyMethodCaller::pushMethodParamsRange(const VmMethodContext *src, VmMethodContext *dst) {
    LOG_D_VM("pushMethodParamsRange, start.");
    u2 count = src->tmp->src1;
    assert(count == dst->method->code->insSize);
    u2 regStart = dst->method->code->registersSize - dst->method->code->insSize;
    for (u2 regOff = 0; regOff < count; regOff++) {
        dst->setRegisterWide(
                regStart + regOff,
                src->getRegisterWide(src->tmp->dst + regOff));
    }
    LOG_D_VM("pushMethodParamsRange, finish.");
}

#if defined(VM_DEBUG_FULL)

void VmKeyMethodCaller::printMethodParam(const VmMethodContext *src, const VmMethodContext *dst) {
    LOG_D_VM("print method(%s) param, start.",
             src->method->dexFile->dexStringById(
                     src->method->dexFile->dexGetMethodId(src->tmp->val_1.u4)->nameIdx));
    u2 regStart = dst->method->code->registersSize - dst->method->code->insSize;
    if (src->isCallStaticMethod()) {
        regStart++;
    }
    const char *shorty = src->method->dexFile->dexGetMethodShorty(src->tmp->val_1.u4);
    LOG_D_VM("method shorty: %s", shorty);
    for (u2 varIdx = 0; shorty[varIdx + 1] != '\0'; regStart++, varIdx++) {
        switch (*shorty) {
            case 'I':
                LOG_D_VM("var(%d) value (int): %d", varIdx, dst->reg[regStart].i);
                break;

            case 'Z':
                dst->reg[regStart].z ? LOG_D_VM("var(%d) value (bool): true", varIdx)
                                     : LOG_D_VM("var(%d) value (bool): false", varIdx);
                break;

            case 'B':
                LOG_D_VM("var(%d) value (byte): 0x%02x", varIdx, dst->reg[regStart].b);
                break;

            case 'S':
                LOG_D_VM("var(%d) value (short): %d", varIdx, dst->reg[regStart].s);
                break;

            case 'C':
                LOG_D_VM("var(%d) value (char): %c", varIdx, dst->reg[regStart].c);
                break;

            case 'F':
                LOG_D_VM("var(%d) value (float): %f", varIdx, dst->reg[regStart].f);
                break;

            case 'L':
                LOG_D_VM("var(%d) value (object): %p", varIdx, dst->reg[regStart].l);
                break;

            case 'J':
                LOG_D_VM("var(%d) value (long): %ld", varIdx, dst->reg[regStart].j);
                regStart++;
                break;

            case 'D':
                LOG_D_VM("var(%d) value (double): %lf", varIdx, dst->reg[regStart].d);
                regStart++;
                break;

            default:
                LOG_E("error method's param type(%s)...", shorty);
                assert(false);
                break;
        }
    }
    LOG_D_VM("print method's param, finish.");
}

#endif
