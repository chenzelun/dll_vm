//
// Created by 陈泽伦 on 11/17/20.
//

#ifndef VM_VM_H
#define VM_VM_H

#include <jni.h>
#include <map>
#include <string>
#include "VmMethod.h"


class CodeHandler {
public:
    virtual void run(VmMethodContext *vmc) = 0;
};

class Interpret {
protected:
    std::map<uint32_t, CodeHandler *> codeMap;

public:
    virtual bool run(VmMethodContext *vmc) = 0;
};

class StandardInterpret : public Interpret {

public:
    StandardInterpret();

    bool run(VmMethodContext *vmc);
};

#define GET_REGISTER(off)       (vmc->reg[off].u32)
#define SET_REGISTER(off, val)  (vmc->reg[off].u32 = *(uint32_t*)(&(val)))

// Standard Interpret's code handler

class CH_NOP : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Move : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Move_From_16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};


class VMException : public std::runtime_error {
public:
    VMException(const char *exp) : runtime_error(std::string("VMException: ") + exp) {}
};

class Vm {
public:
    Interpret *interpret;

public:
    static void
    callMethod(jobject instance, jmethodID method, jvalue *pResult, ...);

    Vm();

    void run(VmMethodContext *vmc);

};


#endif //VM_VM_H
