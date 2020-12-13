//
// Created by 陈泽伦 on 12/10/20.
//

#ifndef VM_VMMETHODCALLER_H
#define VM_VMMETHODCALLER_H

#include "../base/VmMethod.h"

class VmMethodCaller {
public:
    virtual void call(VmMethodContext *vmc) = 0;
};

class VmKeyMethodCaller : public VmMethodCaller {
public:
    void call(VmMethodContext *vmc) override;

private:
    static void pushMethodParams(const VmMethodContext *src, VmMethodContext *dst);

    static void pushMethodParamsRange(const VmMethodContext *src, VmMethodContext *dst);
};

class VmJniMethodCaller : public VmMethodCaller {
public:
    void call(VmMethodContext *vmc) override;

private:
    static const jvalue *pushMethodParams(VmMethodContext *vmc);

    static const jvalue *pushMethodParamsRange(VmMethodContext *vmc);

    static void invokeMethod(VmMethodContext *vmc, const jvalue *params);

    static void invokeSuperMethod(VmMethodContext *vmc, const jvalue *params);

    static void invokeStaticMethod(VmMethodContext *vmc, const jvalue *params);
};


#endif //VM_VMMETHODCALLER_H
