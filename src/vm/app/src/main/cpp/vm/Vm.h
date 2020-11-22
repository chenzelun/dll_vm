//
// Created by 陈泽伦 on 11/17/20.
//

#ifndef VM_VM_H
#define VM_VM_H

#include <jni.h>
#include "VmMethod.h"

class VMException : public std::runtime_error {
public:
    VMException(const char *exp) : runtime_error(std::string("VMException: ") + exp) {}
};

class Vm {
public:
    static void
    callMethod(jobject instance, jmethodID method, jvalue *pResult, ...);

    void run(const VmMethodContext *vmc);

};


#endif //VM_VM_H
