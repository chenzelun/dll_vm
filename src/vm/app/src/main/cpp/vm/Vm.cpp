//
// Created by 陈泽伦 on 11/17/20.
//

#include "Vm.h"
#include "VmMethod.h"
#include "../common/Util.h"

static const char kSpacing[] = "            ";

bool StandardInterpret::run(VmMethodContext *vmc) {
    uint16_t code = vmc->fetch_op();
    StandardInterpret::codeMap[code]->run(vmc);
    return vmc->curException == nullptr;
}

// TODO
StandardInterpret::StandardInterpret() {
    this->codeMap = {
            // code map start
            // code map end
    };
}


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
    this->interpret = new StandardInterpret();
}

void Vm::run(VmMethodContext *vmc) {

}

void CH_NOP::run(VmMethodContext *vmc) {
    LOG_D("NOP, pc: 0x%08x", vmc->pc);
    vmc->pc_off(1);
}

void CH_Move::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|move%s v%d,v%d %s(v%d=0x%08x)",
          "", vmc->dst, vmc->src1, kSpacing, vmc->dst,
          GET_REGISTER(vmc->src1));
    SET_REGISTER(vmc->dst, GET_REGISTER(vmc->src1));
    vmc->pc_off(1);
}

void CH_Move_From_16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    LOG_D("|move%s/from16 v%d,v%d %s(v%d=0x%08x)",
          "", vmc->dst, vmc->src1, kSpacing, vmc->dst,
          GET_REGISTER(vmc->src1));
    SET_REGISTER(vmc->dst, GET_REGISTER(vmc->src1));
    vmc->pc_off(2);
}
