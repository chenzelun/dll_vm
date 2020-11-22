//
// Created by 陈泽伦 on 11/17/20.
//

#include "Vm.h"
#include "../VmContext.h"


void Vm::run(const VmMethodContext *vmc) {

}

void Vm::callMethod(jobject instance, jmethodID method, jvalue *pResult, ...) {
    // init vm method from dex file.
    VmMethod vmMethod(method);
    va_list args;
    va_start(args, pResult);
    // init vm method context
    VmMethodContext vmc(instance, &vmMethod, pResult, args);
    va_end(args);
    // do it
    VM_CONTEXT.vm->run(&vmc);
}
