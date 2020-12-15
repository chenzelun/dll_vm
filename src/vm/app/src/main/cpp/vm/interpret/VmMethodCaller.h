//
// Created by 陈泽伦 on 12/10/20.
//

#ifndef VM_VMMETHODCALLER_H
#define VM_VMMETHODCALLER_H

#include "../base/VmMethod.h"
#include "../base/VmCache.h"

class VmMethodCaller {
public:
    virtual void call(VmMethodContext *vmc) = 0;

    virtual ~VmMethodCaller() {};
};

class VmKeyMethodCaller : public VmMethodCaller {
public:
    void call(VmMethodContext *vmc) override;

private:
    static void pushMethodParams(const VmMethodContext *src, VmMethodContext *dst);

    static void pushMethodParamsRange(const VmMethodContext *src, VmMethodContext *dst);

#if defined(VM_DEBUG_FULL)
    static void printMethodParam(const VmMethodContext *src, const VmMethodContext *dst);
#endif
};

class VmJniMethodCaller : public VmMethodCaller {
public:
    void call(VmMethodContext *vmc) override;

    VmJniMethodCaller();

private:
    static uint32_t cacheKey;

    static const jvalue *pushMethodParams(VmMethodContext *vmc, uint32_t &paramCount);

    static const jvalue *pushMethodParamsRange(VmMethodContext *vmc, uint32_t &paramCount);

    static void invokeMethod(VmMethodContext *vmc, const jvalue *params);

    static void invokeSuperMethod(VmMethodContext *vmc, const jvalue *params);

    static void invokeStaticMethod(VmMethodContext *vmc, const jvalue *params);

#if defined(VM_DEBUG_FULL)
    static void debugInvokeMethod(VmMethodContext *vmc, jmethodID methodCalled,
                                      const char *shorty, const jvalue retVal,
                                      const jvalue *params);
#endif
};


#endif //VM_VMMETHODCALLER_H
