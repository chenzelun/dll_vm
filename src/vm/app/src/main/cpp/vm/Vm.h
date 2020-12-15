//
// Created by 陈泽伦 on 11/17/20.
//

#ifndef VM_VM_H
#define VM_VM_H

#include <jni.h>
#include <string>
#include "base/VmMethod.h"
#include "interpret/Interpret.h"
#include "base/VmStack.h"
#include "base/VmCache.h"
#include "base/VmMemory.h"

#define  PRIMITIVE_TYPE_SIZE 8

class Vm : public VmStack, public VmCache {
private:
    Interpret *interpret;

    VmMemory *vmMemory;
    VmStack *vmStack;
    VmCache* vmCache;

    VmMethodCaller *keyMethodCaller;
    VmMethodCaller *jniMethodCaller;

    // tmp data
    VmTempData methodTempData;

public:
    VmTempData *getTempDataBuf();

    inline VmMethodContext *getCurVMC() {
        return &this->vmStack->getTopFrame()->vmc;
    }

    inline bool isCallFromVm() {
        VmFrame *pre = this->vmStack->getTopFrame()->pre;
        return pre != nullptr && pre->vmc.isCallFromVm();
    }

    static void
    callMethod(jobject instance, jmethodID method, jvalue *pResult, ...);

    static bool isKeyFunction(uint32_t methodId);

    void init();

    ~Vm();

    jclass findPrimitiveClass(const char type) const;

    void setInterpret(Interpret *pInterpret);

    void run();

    void push(jobject caller, jmethodID method, jvalue *pResult, va_list param) override;

    void pushWithoutParams(jmethodID method, jvalue *pResult) override;

    void pop() override;

    uint8_t *mallocCache(uint32_t key, uint32_t count) override;

    void freeCache(uint32_t key, uint32_t count) override;

    uint32_t newCacheType(uint32_t bufSize) override;


private:
    const char primitiveType[PRIMITIVE_TYPE_SIZE] = {
//            'V',
            'B',
            'Z',
            'I',
            'S',
            'C',
            'F',
            'D',
            'J'
    };
    jclass primitiveClass[PRIMITIVE_TYPE_SIZE]{};
private:
    void initPrimitiveClass();

};


#endif //VM_VM_H
