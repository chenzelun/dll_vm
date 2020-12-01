//
// Created by 陈泽伦 on 11/17/20.
//

#include "Vm.h"
#include "../common/Util.h"
#include "../common/VmConstant.h"
#include "../VmContext.h"
#include <cmath>

static const char kSpacing[] = "            ";

void StandardInterpret::run(VmMethodContext *vmc) {
    uint16_t code = vmc->fetch_op();
    StandardInterpret::codeMap[code]->run(vmc);
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
    this->initPrimitiveClass();
}

void Vm::run(VmMethodContext *vmc) const {
   while (!vmc->isFinish()){
       if (vmc->curException!= nullptr && !JAVAException::handleJavaException(vmc)){
           return;
       }
       this->interpret->run(vmc);
   }
}

jclass Vm::findPrimitiveClass(const char type) const {
    assert(type != 'V');
    for (int i = 0; i < PRIMITIVE_TYPE_SIZE; i++) {
        if (this->primitiveType[i] == type) {
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

void CH_NOP::run(VmMethodContext *vmc) {
    LOG_D("NOP");
    vmc->pc_off(1);
}

void CH_Move::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|move%s v%d,v%d %s(v%d=%d)",
          "", vmc->dst, vmc->src1, kSpacing, vmc->dst,
          vmc->getRegisterInt(vmc->src1));
    vmc->setRegisterInt(vmc->dst, vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(1);
}

void CH_Move_From16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    LOG_D("|move%s/from16 v%d,v%d %s(v%d=%d)",
          "", vmc->dst, vmc->src1, kSpacing, vmc->dst,
          vmc->getRegisterInt(vmc->src1));
    vmc->setRegisterInt(vmc->dst, vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(2);
}

void CH_Move_16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->fetch(1);
    vmc->src1 = vmc->fetch(2);
    LOG_D("|move%s/16 v%d,v%d %s(v%d=%d)",
          "", vmc->dst, vmc->src1, kSpacing, vmc->dst,
          vmc->getRegisterInt(vmc->src1));
    vmc->setRegisterInt(vmc->dst, vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(3);
}

void CH_Move_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|move-wide v%d,v%d %s(v%d=%ld)",
          vmc->dst, vmc->src1, kSpacing + 5, vmc->dst,
          vmc->getRegisterLong(vmc->src1));
    vmc->setRegisterLong(vmc->dst, vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(1);
}

void CH_Move_Wide_From16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    LOG_D("|move-wide/from16 v%d,v%d  (v%d=%ld)",
          vmc->dst, vmc->src1, vmc->dst,
          vmc->getRegisterLong(vmc->src1));
    vmc->setRegisterLong(vmc->dst, vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(2);
}

void CH_Move_Wide16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->fetch(1);
    vmc->src1 = vmc->fetch(2);
    LOG_D("|move-wide/16 v%d,v%d %s(v%d=%ld)",
          vmc->dst, vmc->src1, kSpacing + 8, vmc->dst,
          vmc->getRegisterWide(vmc->src1));
    vmc->setRegisterLong(vmc->dst, vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(3);
}

void CH_Move_Object::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|move%s v%d,v%d %s(v%d=%p)", "-object",
          vmc->dst, vmc->src1, kSpacing, vmc->dst,
          vmc->getRegisterAsObject(vmc->src1));
    vmc->setRegisterAsObject(vmc->dst, vmc->getRegisterAsObject(vmc->src1));
    vmc->pc_off(1);
}

void CH_Move_Object_From16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    LOG_D("|move%s/from16 v%d,v%d %s(v%d=%p)",
          "-object", vmc->dst, vmc->src1, kSpacing, vmc->dst,
          vmc->getRegisterAsObject(vmc->src1));
    vmc->setRegisterAsObject(vmc->dst, vmc->getRegisterAsObject(vmc->src1));
    vmc->pc_off(2);
}

void CH_Move_Object16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->fetch(1);
    vmc->src1 = vmc->fetch(2);
    LOG_D("|move%s/16 v%d,v%d %s(v%d=%p)",
          "-object", vmc->dst, vmc->src1, kSpacing, vmc->dst,
          vmc->getRegisterAsObject(vmc->src1));
    vmc->setRegisterAsObject(vmc->dst, vmc->getRegisterAsObject(vmc->src1));
    vmc->pc_off(3);
}

void CH_Move_Result::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    LOG_D("|move-result%s v%d %s(v%d=%d)",
          "", vmc->dst, kSpacing + 4, vmc->dst, vmc->retVal->i);
    vmc->setRegisterInt(vmc->dst, vmc->retVal->i);
    vmc->pc_off(1);
}

void CH_Move_Result_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    LOG_D("|move-result-wide v%d %s(v%d=%ld)",
          vmc->dst, kSpacing, vmc->dst, vmc->retVal->j);
    vmc->setRegisterLong(vmc->dst, vmc->retVal->j);
    vmc->pc_off(1);
}

void CH_Move_Result_Object::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    LOG_D("|move-result%s v%d %s(v%d=0x%p)",
          "-object", vmc->dst, kSpacing + 4, vmc->dst, vmc->retVal->l);
    vmc->setRegisterAsObject(vmc->dst, vmc->retVal->l);
    vmc->pc_off(1);
}

void CH_Move_Exception::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    LOG_D("|move-exception v%d", vmc->dst);
    assert(vmc->curException != nullptr);
    vmc->setRegisterAsObject(vmc->dst, vmc->curException);
    vmc->curException = nullptr;
    vmc->pc_off(1);
}

void CH_Return_Void::run(VmMethodContext *vmc) {
    LOG_D("|return-void");
    vmc->finish();
}

void CH_Return::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    LOG_D("|return%s v%d", "", vmc->src1);
    vmc->retVal->j = 0L;    // set 0
    vmc->retVal->i = vmc->getRegisterInt(vmc->src1);
    vmc->finish();
}

void CH_Return_Wide::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    LOG_D("return-wide v%d", vmc->src1);
    vmc->retVal->j = vmc->getRegisterLong(vmc->src1);
    vmc->finish();
}

void CH_Return_Object::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    LOG_D("|return%s v%d", "-object", vmc->src1);
    vmc->retVal->l = vmc->getRegisterAsObject(vmc->src1);
    vmc->finish();
}

void CH_Const4::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->tmp1.s4 = (s4) (vmc->inst_B() << 28) >> 28;  // sign extend 4-bit value
    LOG_D("|const/4 v%d,#%d", vmc->dst, vmc->tmp1.s4);
    vmc->setRegisterInt(vmc->dst, vmc->tmp1.s4);
    vmc->pc_off(1);
}

void CH_Const16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|const/16 v%d,#%d", vmc->dst, vmc->tmp1.s2);
    vmc->setRegister(vmc->dst, vmc->tmp1.s2);
    vmc->pc_off(2);
}

void CH_Const::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);
    vmc->tmp1.u4 |= (u4) vmc->fetch(2) << 16u;
    LOG_D("|const v%d,#%d", vmc->dst, vmc->tmp1.s4);
    vmc->setRegisterInt(vmc->dst, vmc->tmp1.s4);
    vmc->pc_off(3);
}

void CH_Const_High16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    LOG_D("|const/high16 v%d,#0x%04x0000", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst, (u4) vmc->src1 << 16u);
    vmc->pc_off(2);
}

void CH_Const_Wide16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|const-wide/16 v%d,#%d", vmc->dst, vmc->tmp1.s2);
    vmc->setRegisterLong(vmc->dst, vmc->tmp1.s2);
    vmc->pc_off(2);
}

void CH_Const_Wide32::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);
    vmc->tmp1.u4 |= (u4) vmc->fetch(2) << 16u;
    LOG_D("|const-wide/32 v%d,#%d", vmc->dst, vmc->tmp1.s4);
    vmc->setRegisterLong(vmc->dst, vmc->tmp1.s4);
    vmc->pc_off(3);
}

void CH_Const_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u8 = vmc->fetch(1);
    vmc->tmp1.u8 |= (u8) vmc->fetch(2) << 16u;
    vmc->tmp1.u8 |= (u8) vmc->fetch(3) << 32u;
    vmc->tmp1.u8 |= (u8) vmc->fetch(4) << 48u;
    LOG_D("|const-wide v%d,#%ld", vmc->dst, vmc->tmp1.s8);
    vmc->setRegisterLong(vmc->dst, vmc->tmp1.s8);
    vmc->pc_off(5);
}

void CH_Const_Wide_High16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|const-wide/high16 v%d,#0x%04x000000000000",
          vmc->dst, vmc->src1);
    vmc->tmp2.u8 = ((u8) vmc->src1) << 48u;
    vmc->setRegisterLong(vmc->dst, vmc->tmp2.s8);
    vmc->pc_off(2);
}

void CH_Const_String::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);
    LOG_D("|const-string v%d string@%u", vmc->dst, vmc->tmp1.u4);
    vmc->tmp1.l = vmc->method->resolveString(vmc->tmp1.u4);
    assert(vmc->tmp1.l != nullptr);
    vmc->setRegisterAsObject(vmc->dst, vmc->tmp1.l);
    vmc->pc_off(2);
}

void CH_Const_String_Jumbo::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);
    vmc->tmp1.u4 |= (u4) vmc->fetch(2) << 16u;
    LOG_D("|const-string/jumbo v%d string@%u", vmc->dst, vmc->tmp1.u4);
    vmc->tmp1.l = vmc->method->resolveString(vmc->tmp1.u4);
    assert(vmc->tmp1.l != nullptr);
    vmc->setRegisterAsObject(vmc->dst, vmc->tmp1.l);
    vmc->pc_off(3);
}

void CH_Const_Class::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);
    LOG_D("|const-class v%d class@%u", vmc->dst, vmc->tmp1.u4);
    vmc->tmp1.l = vmc->method->resolveClass(vmc->tmp1.u4);
    if (vmc->tmp1.l == nullptr) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->dst, vmc->tmp1.l);
    vmc->pc_off(2);
}

void CH_Monitor_Enter::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    LOG_D("|monitor-enter v%d %s(%p)",
          vmc->src1, kSpacing + 6, vmc->getRegisterAsObject(vmc->src1));
    vmc->tmp1.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    (*VM_CONTEXT::env).MonitorEnter(vmc->tmp1.l);
    vmc->pc_off(1);
}

void CH_Monitor_Exit::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    LOG_D("|monitor-exit v%d %s(%p)",
          vmc->src1, kSpacing + 5, vmc->getRegisterAsObject(vmc->src1));
    vmc->tmp1.l = vmc->getRegisterAsObject(vmc->src1);
    /**
     * 注意：如果该指令需要抛出异常，则必须像 PC 已超出该指令那样抛出。
     * 不妨将其想象成，该指令（在某种意义上）已成功执行，并在该指令执行后
     * 但下一条指令找到机会执行前抛出异常。这种定义使得某个方法有可能将
     * 监视锁清理 catch-all（例如 finally）分块用作该分块自身的
     * 监视锁清理，以便处理可能由于 Thread.stop() 的既往实现而
     * 抛出的任意异常，同时仍尽力维持适当的监视锁安全机制。
     */
    vmc->pc_off(1);
    if (!JAVAException::checkForNull(vmc->tmp1.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (!(*VM_CONTEXT::env).MonitorExit(vmc->tmp1.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
}

void CH_Check_Cast::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);
    LOG_D("|check-cast v%d,class@%u", vmc->src1, vmc->tmp1.u4);
    vmc->tmp1.l = vmc->getRegisterAsObject(vmc->src1);
    if (vmc->tmp1.l) {
        vmc->tmp2.lc = vmc->method->resolveClass(vmc->src1);
        if (vmc->tmp2.lc == nullptr) {
            JAVAException::throwJavaException(vmc);
            return;
        }
        if (!(*VM_CONTEXT::env).IsInstanceOf(vmc->tmp1.l, vmc->tmp2.lc)) {
            JAVAException::throwClassCastException(
                    (*VM_CONTEXT::env).GetObjectClass(vmc->tmp1.l), vmc->tmp2.lc);
            JAVAException::throwJavaException(vmc);
            return;
        }
//        (*VM_CONTEXT::env).DeleteLocalRef(clazz);
    }
    vmc->pc_off(2);
}

void CH_Instance_Of::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.u4 = vmc->fetch(1);
    LOG_D("|instance-of v%d,v%d,class@%u",
          vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp1.l = vmc->getRegisterAsObject(vmc->src1);
    vmc->tmp2.lc = nullptr;
    if (vmc->tmp1.l == nullptr) {
        vmc->setRegister(vmc->dst, 0);
    } else {
        vmc->tmp2.lc = vmc->method->resolveClass(vmc->fetch(1));
        if (vmc->tmp2.lc == nullptr) {
            JAVAException::throwJavaException(vmc);
            return;
        }
    }
    vmc->tmp1.z = (*VM_CONTEXT::env).IsInstanceOf(vmc->tmp1.l, vmc->tmp2.lc);
    vmc->setRegister(vmc->dst, vmc->tmp1.z);
//    (*VM_CONTEXT::env).DeleteLocalRef(clazz);
    vmc->pc_off(2);
}

void CH_Array_Length::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.l = vmc->getRegisterAsObject(vmc->src1);
    LOG_D("|array-length v%d,v%d  (%p)",
          vmc->dst, vmc->src1, vmc->tmp1.l);
    if (!JAVAException::checkForNull(vmc->tmp1.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp1.u4 = (u4) (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.la);
    vmc->setRegister(vmc->dst, vmc->tmp1.u4);
    vmc->pc_off(1);
}

void CH_New_Instance::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);
    LOG_D("|new-instance v%d,class@%u", vmc->dst, vmc->tmp1.u4);
    vmc->tmp1.l = vmc->method->resolveClass(vmc->tmp1.u4);
    if (vmc->tmp1.l == nullptr) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.l = (*VM_CONTEXT::env).AllocObject(vmc->tmp1.lc);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->dst, vmc->tmp2.l);
//    (*VM_CONTEXT::env).DeleteLocalRef(vmc->tmp1.l);
    vmc->pc_off(2);
}

void CH_New_Array::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.u4 = vmc->fetch(1);
    LOG_D("|new-array v%d,v%d,class@%u  (%d elements)",
          vmc->dst, vmc->src1, vmc->tmp1.u4, vmc->getRegister(vmc->src1));
    vmc->tmp2.s4 = vmc->getRegister(vmc->src1);
    if (vmc->tmp2.s4 < 0) {
        JAVAException::throwNegativeArraySizeException(vmc->tmp2.s4);
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp1.la = vmc->method->allocArray(vmc->tmp2.s4, vmc->tmp1.u4);
    if (vmc->tmp1.la == nullptr) {
        JAVAException::throwRuntimeException("error type of field... cc");
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->dst, vmc->tmp1.l);
    vmc->pc_off(2);
}

void CH_Filled_New_Array::run(VmMethodContext *vmc) {
    LOG_D("|filled_new_array");
    CodeHandler::filledNewArray(vmc, false);
}

void CH_Filled_New_Array_Range::run(VmMethodContext *vmc) {
    LOG_D("|filled_new_array_range");
    CodeHandler::filledNewArray(vmc, true);
}

void CH_Fill_Array_Data::run(VmMethodContext *vmc) {
    JNIEnv *env = VM_CONTEXT::env;
    vmc->src1 = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1) | (((u4) vmc->fetch(2)) << 16u);
    LOG_D("|fill-array-data v%d +%d", vmc->src1, vmc->tmp1.s4);

    const u2 *data = vmc->arrayData(vmc->tmp1.s4);
    vmc->tmp1.l = vmc->getRegisterAsObject(vmc->src1);
    /*
     * Array data table format:
     *  ushort ident = 0x0300   magic value
     *  ushort width            width of each element in the table
     *  uint   size             number of elements in the table
     *  ubyte  data[size*width] table of data values (may contain a single-byte
     *                          padding at the end)
     *
     * Total size is 4+(width * size + 1)/2 16-bit code units.
     */
    if (data[0] != kArrayDataSignature) {
        JAVAException::throwInternalError("bad array data magic");
        JAVAException::throwJavaException(vmc);
        return;
    }
    u4 size = data[2] | ((u4) data[3] << 16u);
    if (size > (*env).GetArrayLength(vmc->tmp1.la)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                (*env).GetArrayLength(vmc->tmp1.la), size);
        JAVAException::throwJavaException(vmc);
        return;
    }
    const std::string desc = VmMethod::getClassDescriptorByJClass(
            (*env).GetObjectClass(vmc->tmp1.l));
    switch (desc[1]) {
        case 'I':
            (*env).SetIntArrayRegion(vmc->tmp1.lia, 0, size, (jint *) (data + 4));
            break;

        case 'C':
            (*env).SetCharArrayRegion(vmc->tmp1.lca, 0, size, (jchar *) (data + 4));
            break;

        case 'Z':
            (*env).SetBooleanArrayRegion(vmc->tmp1.lza, 0, size, (jboolean *) (data + 4));
            break;

        case 'B':
            (*env).SetByteArrayRegion(vmc->tmp1.lba, 0, size, (jbyte *) (data + 4));
            break;

        case 'F':
            (*env).SetFloatArrayRegion(vmc->tmp1.lfa, 0, size, (jfloat *) (data + 4));
            break;

        case 'D':
            (*env).SetDoubleArrayRegion(vmc->tmp1.lda, 0, size, (jdouble *) (data + 4));
            break;

        case 'S':
            (*env).SetShortArrayRegion(vmc->tmp1.lsa, 0, size, (jshort *) (data + 4));
            break;

        case 'J':
            (*env).SetLongArrayRegion(vmc->tmp1.lja, 0, size, (jlong *) (data + 4));
            break;

        default:
            LOG_E("Unknown primitive type '%c'", desc[1]);
            JAVAException::throwRuntimeException("error type of field... cc");
            JAVAException::throwJavaException(vmc);
            return;
    }
    vmc->pc_off(3);
}

void CH_Throw::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    LOG_D("throw v%d  (%p)",
          vmc->src1, vmc->getRegisterAsObject(vmc->src1));
    vmc->tmp1.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.l)) {
        /* will throw a null pointer exception */
        LOG_E("Bad exception");
    } else {
        /* use the requested exception */
        (*VM_CONTEXT::env).Throw(vmc->tmp1.lt);
    }
    JAVAException::throwJavaException(vmc);
    // no pc_off.
}

void CH_Goto::run(VmMethodContext *vmc) {
    vmc->tmp1.s1 = vmc->inst_AA();
    if (vmc->tmp1.s1 < 0) {
        LOG_D("|goto -%d", -(vmc->tmp1.s1));
    } else {
        LOG_D("|goto +%d", (vmc->tmp1.s1));
        LOG_D("> branch taken");
    }
    vmc->pc_goto(vmc->tmp1.s1);
    // no pc_off
}

void CH_Goto16::run(VmMethodContext *vmc) {
    vmc->tmp1.s2 = vmc->fetch(1);   /* sign-extend next code unit */
    if (vmc->tmp1.s2 < 0) {
        LOG_D("|goto -%d", -(vmc->tmp1.s2));
    } else {
        LOG_D("|goto +%d", (vmc->tmp1.s2));
        LOG_D("> branch taken");
    }
    vmc->pc_goto(vmc->tmp1.s2);
    // no pc_off
}

void CH_Goto32::run(VmMethodContext *vmc) {
    vmc->tmp1.u4 = vmc->fetch(1);   /* low-order 16 bits */
    vmc->tmp1.u4 |= ((u4) vmc->fetch(2)) << 16u;    /* high-order 16 bits */
    if (vmc->tmp1.s4 < 0) {
        LOG_D("|goto -%d", -(vmc->tmp1.s4));
    } else {
        LOG_D("|goto +%d", (vmc->tmp1.s4));
        LOG_D("> branch taken");
    }
    vmc->pc_goto(vmc->tmp1.s4);
    // no pc_off
}

void CH_Packed_Switch::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1) | ((u4) vmc->fetch(2) << 16u);
    LOG_D("|packed-switch v%d +%d", vmc->src1, vmc->tmp1.s4);
    const u2 *data = vmc->arrayData(vmc->tmp1.s4);   // offset in 16-bit units
    vmc->tmp1.u4 = vmc->getRegister(vmc->src1);
    vmc->tmp1.s4 = CodeHandler::handlePackedSwitch(data, vmc->tmp1.u4);
    LOG_D("> branch taken (%d)", vmc->tmp1.s4);
    vmc->pc_goto(vmc->tmp1.s4);
    // no pc_off
}

void CH_Sparse_Switch::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1) | ((u4) vmc->fetch(2) << 16u);
    LOG_D("|packed-switch v%d +%d", vmc->src1, vmc->tmp1.s4);
    const u2 *data = vmc->arrayData(vmc->tmp1.s4);   // offset in 16-bit units
    vmc->tmp1.u4 = vmc->getRegister(vmc->src1);
    vmc->tmp1.s4 = CodeHandler::handleSparseSwitch(data, vmc->tmp1.u4);
    LOG_D("> branch taken (%d)", vmc->tmp1.s4);
    vmc->pc_goto(vmc->tmp1.s4);
    // no pc_off
}

void CH_CMPL_Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "l-float", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.f = vmc->getRegisterFloat(vmc->src1);
    vmc->tmp2.f = vmc->getRegisterFloat(vmc->src2);
    if (vmc->tmp1.f == vmc->tmp2.f) {
        vmc->retVal->i = 0;
    } else if (vmc->tmp1.f > vmc->tmp2.f) {
        vmc->retVal->i = 1;
    } else {
        vmc->retVal->i = -1;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void CH_CMPG_Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "g-float", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.f = vmc->getRegisterFloat(vmc->src1);
    vmc->tmp2.f = vmc->getRegisterFloat(vmc->src2);
    if (vmc->tmp1.f == vmc->tmp2.f) {
        vmc->retVal->i = 0;
    } else if (vmc->tmp1.f < vmc->tmp2.f) {
        vmc->retVal->i = -1;
    } else {
        vmc->retVal->i = 1;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void CH_CMPL_Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "l-double", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.d = vmc->getRegisterDouble(vmc->src1);
    vmc->tmp2.d = vmc->getRegisterDouble(vmc->src2);
    if (vmc->tmp1.d == vmc->tmp2.d) {
        vmc->retVal->i = 0;
    } else if (vmc->tmp1.d > vmc->tmp2.d) {
        vmc->retVal->i = 1;
    } else {
        vmc->retVal->i = -1;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void CH_CMPG_Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "g-double", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.d = vmc->getRegisterDouble(vmc->src1);
    vmc->tmp2.d = vmc->getRegisterDouble(vmc->src2);
    if (vmc->tmp1.d == vmc->tmp2.d) {
        vmc->retVal->i = 0;
    } else if (vmc->tmp1.d < vmc->tmp2.d) {
        vmc->retVal->i = -1;
    } else {
        vmc->retVal->i = 1;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void CH_CMP_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "-long", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.s8 = vmc->getRegisterWide(vmc->src1);
    vmc->tmp2.s8 = vmc->getRegisterWide(vmc->src2);
    if (vmc->tmp1.s8 > vmc->tmp2.s8) {
        vmc->retVal->i = 1;
    } else if (vmc->tmp1.s8 < vmc->tmp2.s8) {
        vmc->retVal->i = -1;
    } else {
        vmc->retVal->i = 0;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void CH_IF_EQ::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    vmc->src2 = vmc->inst_B();
    vmc->tmp1.s4 = (s4) vmc->getRegister(vmc->src1);
    vmc->tmp2.s4 = (s4) vmc->getRegister(vmc->src2);
    if (vmc->tmp1.s4 == vmc->tmp2.s4) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "eq", vmc->src1, vmc->src2, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u,-",
              "eq", vmc->src1, vmc->src2);
        vmc->pc_off(2);
    }
}

void CH_IF_NE::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    vmc->src2 = vmc->inst_B();
    vmc->tmp1.s4 = (s4) vmc->getRegister(vmc->src1);
    vmc->tmp2.s4 = (s4) vmc->getRegister(vmc->src2);
    if (vmc->tmp1.s4 != vmc->tmp2.s4) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "ne", vmc->src1, vmc->src2, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u,-",
              "ne", vmc->src1, vmc->src2);
        vmc->pc_off(2);
    }
}

void CH_IF_LT::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    vmc->src2 = vmc->inst_B();
    vmc->tmp1.s4 = (s4) vmc->getRegister(vmc->src1);
    vmc->tmp2.s4 = (s4) vmc->getRegister(vmc->src2);
    if (vmc->tmp1.s4 < vmc->tmp2.s4) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "lt", vmc->src1, vmc->src2, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u,-",
              "lt", vmc->src1, vmc->src2);
        vmc->pc_off(2);
    }
}

void CH_IF_LE::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    vmc->src2 = vmc->inst_B();
    vmc->tmp1.s4 = (s4) vmc->getRegister(vmc->src1);
    vmc->tmp2.s4 = (s4) vmc->getRegister(vmc->src2);
    if (vmc->tmp1.s4 <= vmc->tmp2.s4) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "le", vmc->src1, vmc->src2, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u,-",
              "le", vmc->src1, vmc->src2);
        vmc->pc_off(2);
    }
}

void CH_IF_GT::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    vmc->src2 = vmc->inst_B();
    vmc->tmp1.s4 = (s4) vmc->getRegister(vmc->src1);
    vmc->tmp2.s4 = (s4) vmc->getRegister(vmc->src2);
    if (vmc->tmp1.s4 > vmc->tmp2.s4) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "gt", vmc->src1, vmc->src2, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u,-",
              "gt", vmc->src1, vmc->src2);
        vmc->pc_off(2);
    }
}

void CH_IF_GE::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    vmc->src2 = vmc->inst_B();
    vmc->tmp1.s4 = (s4) vmc->getRegister(vmc->src1);
    vmc->tmp2.s4 = (s4) vmc->getRegister(vmc->src2);
    if (vmc->tmp1.s4 >= vmc->tmp2.s4) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "ge", vmc->src1, vmc->src2, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u,-",
              "ge", vmc->src1, vmc->src2);
        vmc->pc_off(2);
    }
}

void CH_IF_EQZ::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->src1) == 0) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "eqz", vmc->src1, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,-", "eqz", vmc->src1);
        vmc->pc_off(2);
    }
}

void CH_IF_NEZ::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->src1) != 0) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "nez", vmc->src1, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,-", "nez", vmc->src1);
        vmc->pc_off(2);
    }
}

void CH_IF_LTZ::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->src1) < 0) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "ltz", vmc->src1, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,-", "ltz", vmc->src1);
        vmc->pc_off(2);
    }
}

void CH_IF_GEZ::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->src1) >= 0) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "gez", vmc->src1, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,-", "gez", vmc->src1);
        vmc->pc_off(2);
    }
}

void CH_IF_GTZ::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->src1) > 0) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "gtz", vmc->src1, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,-", "gtz", vmc->src1);
        vmc->pc_off(2);
    }
}

void CH_IF_LEZ::run(VmMethodContext *vmc) {
    vmc->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->src1) <= 0) {
        vmc->tmp1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "lez", vmc->src1, vmc->tmp1.s4);
        LOG_D("> branch taken");
        vmc->pc_goto(vmc->tmp1.s4);
    } else {
        LOG_D("|if-%s v%u,-", "lez", vmc->src1);
        vmc->pc_off(2);
    }
}

void CH_Aget::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-normal", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lia = (jintArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lia)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lia);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetIntArrayRegion(
            vmc->tmp1.lia, vmc->getRegister(vmc->src2), 1, (jint *) buf);
    vmc->setRegisterInt(vmc->dst, *(jint *) buf);
    LOG_D("+ AGET[%u]=%d",
          vmc->getRegister(vmc->src2),
          vmc->getRegisterInt(vmc->dst));
    vmc->pc_off(2);
}

void CH_Aget_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-wide", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lja = (jlongArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lja)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lja);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetLongArrayRegion(
            vmc->tmp1.lja, vmc->getRegister(vmc->src2), 1, (jlong *) buf);
    vmc->setRegisterLong(vmc->dst, *(jlong *) buf);
    LOG_D("+ AGET[%d]=%ld",
          vmc->getRegister(vmc->src2),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Aget_Object::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("|aget%s v%u,v%u,v%u",
          "-object", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lla = (jobjectArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lla)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lla);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.l = (*VM_CONTEXT::env).GetObjectArrayElement(
            vmc->tmp1.lla, vmc->getRegister(vmc->src2));
    vmc->setRegisterAsObject(vmc->dst, vmc->tmp2.l);
    LOG_D("+ AGET[%d]=%p",
          vmc->getRegister(vmc->src2),
          vmc->getRegisterAsObject(vmc->dst));
    vmc->pc_off(2);
}

void CH_Aget_Boolean::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-boolean", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lza = (jbooleanArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lza)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lza);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetBooleanArrayRegion(
            vmc->tmp1.lza, vmc->getRegister(vmc->src2), 1, (jboolean *) buf);
    vmc->setRegister(vmc->dst, *(jboolean *) buf);
    LOG_D("+ AGET[%d]=%u",
          vmc->getRegister(vmc->src2),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Aget_Byte::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-byte", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lba = (jbyteArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lba)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lba);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetByteArrayRegion(
            vmc->tmp1.lba, vmc->getRegister(vmc->src2), 1, (jbyte *) buf);
    vmc->setRegister(vmc->dst, *(jbyte *) buf);
    LOG_D("+ AGET[%d]=%u",
          vmc->getRegister(vmc->src2),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Aget_Char::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-char", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lca = (jcharArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lca)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lca);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetCharArrayRegion(
            vmc->tmp1.lca, vmc->getRegister(vmc->src2), 1, (jchar *) buf);
    vmc->setRegister(vmc->dst, *(jchar *) buf);
    LOG_D("+ AGET[%d]=%u",
          vmc->getRegister(vmc->src2),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Aget_Short::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-short", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lsa = (jshortArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lsa)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lsa);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetShortArrayRegion(
            vmc->tmp1.lsa, vmc->getRegister(vmc->src2), 1, (jshort *) buf);
    vmc->setRegister(vmc->dst, *(jshort *) buf);
    LOG_D("+ AGET[%d]=%u",
          vmc->getRegister(vmc->src2),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Aput::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();      /* AA: source value */
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-normal", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lia = (jintArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lia)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lia);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ APUT[%d]=%d",
          vmc->getRegister(vmc->src2),
          vmc->getRegisterInt(vmc->dst));
    u8 buf[1];
    *(jint *) buf = vmc->getRegister(vmc->dst);
    (*VM_CONTEXT::env).SetIntArrayRegion(
            vmc->tmp1.lia, vmc->getRegister(vmc->src2), 1, (jint *) buf);
    vmc->pc_off(2);
}

void CH_Aput_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();      /* AA: source value */
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-wide", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lja = (jlongArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lja)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lja);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ APUT[%d]=%ld",
          vmc->getRegister(vmc->src2),
          vmc->getRegisterLong(vmc->dst));
    u8 buf[1];
    *(jlong *) buf = vmc->getRegisterLong(vmc->dst);
    (*VM_CONTEXT::env).SetLongArrayRegion(
            vmc->tmp1.lja, vmc->getRegister(vmc->src2), 1, (jlong *) buf);
    vmc->pc_off(2);
}


void CH_Aput_Object::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();      /* AA: source value */
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u", "-object", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lla = (jobjectArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lla)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lla);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ APUT[%d]=%p", vmc->getRegister(vmc->src2),
          vmc->getRegisterAsObject(vmc->dst));
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->dst);
    (*VM_CONTEXT::env).SetObjectArrayElement(
            vmc->tmp1.lla, vmc->getRegister(vmc->src2), vmc->tmp2.l);
    vmc->pc_off(2);
}

void CH_Aput_Boolean::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();      /* AA: source value */
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-boolean", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lza = (jbooleanArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lza)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lza);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ APUT[%d]=%u",
          vmc->getRegister(vmc->src2),
          vmc->getRegister(vmc->dst));
    u8 buf[1];
    *(jboolean *) buf = vmc->getRegister(vmc->dst);
    (*VM_CONTEXT::env).SetBooleanArrayRegion(
            vmc->tmp1.lza, vmc->getRegister(vmc->src2), 1, (jboolean *) buf);
    vmc->pc_off(2);
}

void CH_Aput_Byte::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();      /* AA: source value */
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-byte", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lba = (jbyteArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lba)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lba);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ APUT[%d]=%u",
          vmc->getRegister(vmc->src2),
          vmc->getRegister(vmc->dst));
    u8 buf[1];
    *(jbyte *) buf = vmc->getRegister(vmc->dst);
    (*VM_CONTEXT::env).SetByteArrayRegion(
            vmc->tmp1.lba, vmc->getRegister(vmc->src2), 1, (jbyte *) buf);
    vmc->pc_off(2);
}

void CH_Aput_Char::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();      /* AA: source value */
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-char", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lca = (jcharArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lca)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lca);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ APUT[%d]=%u",
          vmc->getRegister(vmc->src2),
          vmc->getRegister(vmc->dst));
    u8 buf[1];
    *(jchar *) buf = vmc->getRegister(vmc->dst);
    (*VM_CONTEXT::env).SetCharArrayRegion(
            vmc->tmp1.lca, vmc->getRegister(vmc->src2), 1, (jchar *) buf);
    vmc->pc_off(2);
}

void CH_Aput_Short::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();      /* AA: source value */
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;    /* index */
    vmc->src1 = vmc->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-short", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.lsa = (jshortArray) vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp1.lsa)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->tmp2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp1.lsa);
    if (vmc->tmp2.u4 <= vmc->getRegister(vmc->src2)) {
        JAVAException::throwArrayIndexOutOfBoundsException(
                vmc->tmp2.u4, vmc->getRegister(vmc->src2));
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ APUT[%d]=%d",
          vmc->getRegister(vmc->src2),
          vmc->getRegister(vmc->dst));
    u8 buf[1];
    *(jshort *) buf = vmc->getRegister(vmc->dst);
    (*VM_CONTEXT::env).SetShortArrayRegion(
            vmc->tmp1.lsa, vmc->getRegister(vmc->src2), 1, (jshort *) buf);
    vmc->pc_off(2);
}

void OP_Iget::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-normal", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterInt(vmc->dst, val.i);
    LOG_D("+ IGET '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterInt(vmc->dst));
    vmc->pc_off(2);
}

void OP_Iget_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-wide", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterLong(vmc->dst, val.j);
    LOG_D("+ IGET '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void OP_Iget_Object::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-object", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->dst, val.l);
    LOG_D("+ IGET '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterAsObject(vmc->dst));
    vmc->pc_off(2);
}

void OP_Iget_Boolean::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-bool", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.z);
    LOG_D("+ IGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void OP_Iget_Byte::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-byte", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.b);
    LOG_D("+ IGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void OP_Iget_Char::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-char", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.c);
    LOG_D("+ IGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void OP_Iget_Short::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-short", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.s);
    LOG_D("+ IGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-normal", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.i = vmc->getRegisterInt(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterInt(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-wide", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.j = vmc->getRegisterLong(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput_Object::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-object", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.l = vmc->getRegisterAsObject(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterAsObject(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput_Boolean::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-bool", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.z = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput_Byte::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-byte", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.b = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput_Char::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-char", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.c = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_IF_Short::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-short", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.s = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-normal", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterInt(vmc->dst, val.i);
    LOG_D("+ SGET '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterInt(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-wide", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterLong(vmc->dst, val.j);
    LOG_D("+ SGET '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Object::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-object", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->dst, val.l);
    LOG_D("+ SGET '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterAsObject(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Boolean::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-boolean", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.z);
    LOG_D("+ SGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Byte::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-byte", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.b);
    LOG_D("+ SGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Char::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-char", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.c);
    LOG_D("+ SGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Short::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-short", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.s);
    LOG_D("+ SGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%d,sfield@%u",
          "-normal", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.i = vmc->getRegisterInt(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterInt(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Wide::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%d,sfield@%u",
          "-wide", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.j = vmc->getRegisterLong(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Object::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%d,sfield@%u",
          "-object", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.l = vmc->getRegisterAsObject(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterAsObject(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Boolean::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%d,sfield@%u",
          "-boolean", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.z = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Byte::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%d,sfield@%u",
          "-byte", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.b = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Char::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%d,sfield@%u",
          "-char", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.c = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Short::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%d,sfield@%u",
          "-short", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.s = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegister(vmc->dst));
    vmc->pc_off(2);
}

void CH_Invoke_Virtual::run(VmMethodContext *vmc) {

}

void CH_Invoke_Super::run(VmMethodContext *vmc) {

}

void CH_Invoke_Direct::run(VmMethodContext *vmc) {

}

void CH_Invoke_Static::run(VmMethodContext *vmc) {

}

void CH_Invoke_Interface::run(VmMethodContext *vmc) {

}

void CH_Invoke_Virtual_Range::run(VmMethodContext *vmc) {

}

void CH_Invoke_Super_Range::run(VmMethodContext *vmc) {

}

void CH_Invoke_Direct_Range::run(VmMethodContext *vmc) {

}

void CH_Invoke_Static_Range::run(VmMethodContext *vmc) {

}

void CH_Invoke_Interface_Range::run(VmMethodContext *vmc) {

}

void CH_Neg_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "neg-int", vmc->dst, vmc->src1);
    vmc->setRegisterInt(vmc->dst, -vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(1);
}

void CH_Not_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "not-int", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1) ^ 0xffffffff);
    vmc->pc_off(1);
}

void CH_Neg_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "neg-long", vmc->dst, vmc->src1);
    vmc->setRegisterLong(vmc->dst, -vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(1);
}

void CH_Not_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "not-long", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1) ^ 0xffffffffffffffffULL);
    vmc->pc_off(1);
}

void CH_Neg_Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "neg-float", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst, -vmc->getRegisterFloat(vmc->src1));
    vmc->pc_off(1);
}

void CH_Neg_Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "neg-double", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst, -vmc->getRegisterDouble(vmc->src1));
    vmc->pc_off(1);
}

void CH_Int2Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "int-to-long", vmc->dst, vmc->src1);
    vmc->setRegisterLong(vmc->dst, vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(1);
}

void CH_Int2Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "int-to-float", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst, vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(1);
}

void CH_Int2Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "int-to-double", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst, vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(1);
}

void CH_Long2Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "long-to-int", vmc->dst, vmc->src1);
    vmc->setRegisterInt(vmc->dst, vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(1);
}

void CH_Long2Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "long-to-float", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst, vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(1);
}

void CH_Long2Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "long-to-double", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst, vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(1);
}

void CH_Float2Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "float-to-int", vmc->dst, vmc->src1);
    jint min = (jint) 1 << (sizeof(jint) * 8 - 1);      // get min
    jint max = ~min;                                    // get max
    vmc->tmp1.f = vmc->getRegisterFloat(vmc->src1);
    if (vmc->tmp1.f >= max) {                   /* +inf */
        vmc->tmp2.i = max;
    } else if (vmc->tmp1.f <= min) {            /* -inf */
        vmc->tmp2.i = min;
    } else if (vmc->tmp1.f != vmc->tmp1.f) {    /* NaN */
        vmc->tmp2.i = 0;
    } else {
        vmc->tmp2.i = (jint) vmc->tmp1.f;
    }
    vmc->setRegisterInt(vmc->dst, vmc->tmp2.i);
}

void CH_Float2Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "float-to-long", vmc->dst, vmc->src1);
    jlong min = (jlong) 1 << (sizeof(jlong) * 8 - 1);       // get min
    jlong max = ~min;                                       // get max
    vmc->tmp1.f = vmc->getRegisterFloat(vmc->src1);
    if (vmc->tmp1.f >= max) {                   /* +inf */
        vmc->tmp2.j = max;
    } else if (vmc->tmp1.f <= min) {            /* -inf */
        vmc->tmp2.j = min;
    } else if (vmc->tmp1.f != vmc->tmp1.f) {    /* NaN */
        vmc->tmp2.j = 0;
    } else {
        vmc->tmp2.j = (jint) vmc->tmp1.f;
    }
    vmc->setRegisterLong(vmc->dst, vmc->tmp2.j);
}

void CH_Float2Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "float-to-double", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst, vmc->getRegisterFloat(vmc->src1));
    vmc->pc_off(1);
}

void CH_Double2Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "double-to-int", vmc->dst, vmc->src1);
    jint min = (jint) 1 << (sizeof(jint) * 8 - 1);      // get min
    jint max = ~min;                                    // get max
    vmc->tmp1.d = vmc->getRegisterDouble(vmc->src1);
    if (vmc->tmp1.d >= max) {                   /* +inf */
        vmc->tmp2.i = max;
    } else if (vmc->tmp1.d <= min) {            /* -inf */
        vmc->tmp2.i = min;
    } else if (vmc->tmp1.d != vmc->tmp1.d) {    /* NaN */
        vmc->tmp2.i = 0;
    } else {
        vmc->tmp2.i = (jint) vmc->tmp1.d;
    }
    vmc->setRegisterInt(vmc->dst, vmc->tmp2.i);
}

void CH_Double2Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "double-to-long", vmc->dst, vmc->src1);
    jlong min = (jlong) 1 << (sizeof(jlong) * 8 - 1);       // get min
    jlong max = ~min;                                       // get max
    vmc->tmp1.d = vmc->getRegisterDouble(vmc->src1);
    if (vmc->tmp1.d >= max) {                   /* +inf */
        vmc->tmp2.j = max;
    } else if (vmc->tmp1.d <= min) {            /* -inf */
        vmc->tmp2.j = min;
    } else if (vmc->tmp1.d != vmc->tmp1.d) {    /* NaN */
        vmc->tmp2.j = 0;
    } else {
        vmc->tmp2.j = (jint) vmc->tmp1.d;
    }
    vmc->setRegisterLong(vmc->dst, vmc->tmp2.j);
}

void CH_Double2Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "double-to-float", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst, vmc->getRegisterDouble(vmc->src1));
    vmc->pc_off(1);
}

void CH_Int2Byte::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|int-to-%s v%u,v%u", "byte", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst, (s1) vmc->getRegister(vmc->src1));
    vmc->pc_off(1);
}

void CH_Int2Char::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|int-to-%s v%u,v%u", "char", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst, (u2) vmc->getRegister(vmc->src1));
    vmc->pc_off(1);
}

void CH_Int2Short::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|int-to-%s v%u,v%u", "short", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst, (s2) vmc->getRegister(vmc->src1));
    vmc->pc_off(1);
}

void CH_Add_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "add", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                        + vmc->getRegisterInt(vmc->src2));
    vmc->pc_off(2);
}

void CH_Sub_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "sub", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                        - vmc->getRegisterInt(vmc->src2));
    vmc->pc_off(2);
}

void CH_Mul_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "mul", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                        * vmc->getRegisterInt(vmc->src2));
    vmc->pc_off(2);
}

void CH_Div_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "div", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.i = vmc->getRegisterInt(vmc->src1);
    vmc->tmp2.i = vmc->getRegisterInt(vmc->src2);
    if (vmc->tmp2.i == 0) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u4 == 0x80000000u && vmc->tmp2.i == -1) {
        vmc->tmp2.i = vmc->tmp1.i;
    } else {
        vmc->tmp2.i = vmc->tmp1.i / vmc->tmp2.i;
    }
    vmc->setRegisterInt(vmc->dst, vmc->tmp2.i);
    vmc->pc_off(2);
}

void CH_Rem_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "rem", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.i = vmc->getRegisterInt(vmc->src1);
    vmc->tmp2.i = vmc->getRegisterInt(vmc->src2);
    if (vmc->tmp2.i == 0) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u4 == 0x80000000u && vmc->tmp2.i == -1) {
        vmc->tmp2.i = 0;
    } else {
        vmc->tmp2.i = vmc->tmp1.i / vmc->tmp2.i;
    }
    vmc->setRegisterInt(vmc->dst, vmc->tmp2.i);
    vmc->pc_off(2);
}

void CH_And_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "and", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     & vmc->getRegister(vmc->src2));
    vmc->pc_off(2);
}

void CH_Or_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "or", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     | vmc->getRegister(vmc->src2));
    vmc->pc_off(2);
}

void CH_Xor_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "xor", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     ^ vmc->getRegister(vmc->src2));
    vmc->pc_off(2);
}

void CH_Shl_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "shl", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                                << (vmc->getRegister(vmc->src2) & 0x1fu));
    vmc->pc_off(2);
}

void CH_Shr_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "shr", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                                >> (vmc->getRegister(vmc->src2) & 0x1fu));
    vmc->pc_off(2);
}

void CH_Ushr_Int::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "ushr", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                             >> (vmc->getRegister(vmc->src2) & 0x1fu));
    vmc->pc_off(2);
}

void CH_Add_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "add", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->src1)
                         + vmc->getRegisterLong(vmc->src2));
    vmc->pc_off(2);
}

void CH_Sub_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "sub", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->src1)
                         - vmc->getRegisterLong(vmc->src2));
    vmc->pc_off(2);
}

void CH_Mul_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "mul", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->src1)
                         * vmc->getRegisterLong(vmc->src2));
    vmc->pc_off(2);
}

void CH_Div_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "div", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.j = vmc->getRegisterLong(vmc->src1);
    vmc->tmp2.j = vmc->getRegisterLong(vmc->src2);
    if (vmc->tmp2.j == 0LL) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u8 == 0x8000000000000000ULL && vmc->tmp2.j == -1LL) {
        vmc->tmp2.j = vmc->tmp1.j;
    } else {
        vmc->tmp2.j = vmc->tmp1.j / vmc->tmp2.j;
    }
    vmc->setRegisterLong(vmc->dst, vmc->tmp2.j);
    vmc->pc_off(2);
}

void CH_Rem_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "rem", vmc->dst, vmc->src1, vmc->src2);
    vmc->tmp1.j = vmc->getRegisterLong(vmc->src1);
    vmc->tmp2.j = vmc->getRegisterLong(vmc->src2);
    if (vmc->tmp2.j == 0LL) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u8 == 0x8000000000000000ULL && vmc->tmp2.j == -1LL) {
        vmc->tmp2.j = 0LL;
    } else {
        vmc->tmp2.j = vmc->tmp1.j / vmc->tmp2.j;
    }
    vmc->setRegisterLong(vmc->dst, vmc->tmp2.j);
    vmc->pc_off(2);
}

void CH_And_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "and", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterWide(vmc->src1)
                         & vmc->getRegisterWide(vmc->src2));
    vmc->pc_off(2);
}

void CH_Or_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "or", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterWide(vmc->dst,
                         vmc->getRegisterWide(vmc->src1)
                         | vmc->getRegisterWide(vmc->src2));
    vmc->pc_off(2);
}

void CH_Xor_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "xor", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterWide(vmc->dst,
                         vmc->getRegisterWide(vmc->src1)
                         ^ vmc->getRegisterWide(vmc->src2));
    vmc->pc_off(2);
}

void CH_Shl_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "shl", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->src1)
                                 << (vmc->getRegister(vmc->src2) & 0x3fu));
    vmc->pc_off(2);
}

void CH_Shr_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "shr", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->src1)
                                 >> (vmc->getRegister(vmc->src2) & 0x3fu));
    vmc->pc_off(2);
}

void CH_Ushr_Long::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "ushr", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterWide(vmc->dst,
                         vmc->getRegisterWide(vmc->src1)
                                 >> (vmc->getRegister(vmc->src2) & 0x3fu));
    vmc->pc_off(2);
}

void CH_Add_Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "add", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterFloat(vmc->dst,
                          vmc->getRegisterFloat(vmc->src1)
                          + vmc->getRegisterFloat(vmc->src2));
    vmc->pc_off(2);
}

void CH_Sub_Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "sub", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterFloat(vmc->dst,
                          vmc->getRegisterFloat(vmc->src1)
                          - vmc->getRegisterFloat(vmc->src2));
    vmc->pc_off(2);
}

void CH_Mul_Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "mul", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterFloat(vmc->dst,
                          vmc->getRegisterFloat(vmc->src1)
                          * vmc->getRegisterFloat(vmc->src2));
    vmc->pc_off(2);
}

void CH_Div_Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "div", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterFloat(vmc->dst,
                          vmc->getRegisterFloat(vmc->src1)
                          / vmc->getRegisterFloat(vmc->src2));
    vmc->pc_off(2);
}

void CH_Rem_Float::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "rem", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterFloat(vmc->dst,
                          std::fmodf(vmc->getRegisterFloat(vmc->src1),
                                     vmc->getRegisterFloat(vmc->src2)));
    vmc->pc_off(2);
}

void CH_Add_Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "add", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterDouble(vmc->dst,
                           vmc->getRegisterDouble(vmc->src1)
                           + vmc->getRegisterDouble(vmc->src2));
    vmc->pc_off(2);
}

void CH_Sub_Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "sub", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterDouble(vmc->dst,
                           vmc->getRegisterDouble(vmc->src1)
                           - vmc->getRegisterDouble(vmc->src2));
    vmc->pc_off(2);
}

void CH_Mul_Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "mul", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterDouble(vmc->dst,
                           vmc->getRegisterDouble(vmc->src1)
                           * vmc->getRegisterDouble(vmc->src2));
    vmc->pc_off(2);
}

void CH_Div_Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "div", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterDouble(vmc->dst,
                           vmc->getRegisterDouble(vmc->src1)
                           / vmc->getRegisterDouble(vmc->src2));
    vmc->pc_off(2);
}

void CH_Rem_Double::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->src2 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "rem", vmc->dst, vmc->src1, vmc->src2);
    vmc->setRegisterDouble(vmc->dst,
                           std::fmod(vmc->getRegisterDouble(vmc->src1),
                                     vmc->getRegisterDouble(vmc->src2)));
    vmc->pc_off(2);
}

void CH_Add_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "add", vmc->dst, vmc->src1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->dst)
                        + vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(1);
}

void CH_Sub_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "sub", vmc->dst, vmc->src1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->dst)
                        - vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(1);
}

void CH_Mul_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "mul", vmc->dst, vmc->src1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->dst)
                        * vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(1);
}

void CH_Div_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "div", vmc->dst, vmc->src1);
    vmc->tmp1.i = vmc->getRegisterInt(vmc->dst);
    vmc->tmp2.i = vmc->getRegisterInt(vmc->src1);
    if (vmc->tmp2.i == 0) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u4 == 0x80000000u && vmc->tmp2.i == -1) {
        vmc->tmp2.i = vmc->tmp1.i;
    } else {
        vmc->tmp2.i = vmc->tmp1.i / vmc->tmp2.i;
    }
    vmc->setRegisterInt(vmc->dst, vmc->tmp2.i);
    vmc->pc_off(2);
}

void CH_Rem_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "rem", vmc->dst, vmc->src1);
    vmc->tmp1.i = vmc->getRegisterInt(vmc->dst);
    vmc->tmp2.i = vmc->getRegisterInt(vmc->src1);
    if (vmc->tmp2.i == 0) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u4 == 0x80000000u && vmc->tmp2.i == -1) {
        vmc->tmp2.i = 0;
    } else {
        vmc->tmp2.i = vmc->tmp1.i / vmc->tmp2.i;
    }
    vmc->setRegisterInt(vmc->dst, vmc->tmp2.i);
    vmc->pc_off(2);
}

void CH_And_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "and", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->dst)
                     & vmc->getRegister(vmc->src1));
    vmc->pc_off(1);
}

void CH_Or_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "or", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->dst)
                     * vmc->getRegister(vmc->src1));
    vmc->pc_off(1);
}

void CH_Xor_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "xor", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->dst)
                     ^ vmc->getRegister(vmc->src1));
    vmc->pc_off(1);
}

void CH_Shl_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "shl", vmc->dst, vmc->src1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->dst)
                                << (vmc->getRegister(vmc->src1) & 0x1fu));
    vmc->pc_off(1);
}

void CH_Shr_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "shr", vmc->dst, vmc->src1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->dst)
                                >> (vmc->getRegister(vmc->src1) & 0x1fu));
    vmc->pc_off(1);
}

void CH_Ushr_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%d,v%d", "ushr", vmc->dst, vmc->src1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->dst)
                             >> (vmc->getRegister(vmc->src1) & 0x1fu));
    vmc->pc_off(1);
}

void CH_Add_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "add", vmc->dst, vmc->src1);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->dst)
                         + vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(1);
}

void CH_Sub_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "sub", vmc->dst, vmc->src1);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->dst)
                         - vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(1);
}

void CH_Mul_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "mul", vmc->dst, vmc->src1);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->dst)
                         * vmc->getRegisterLong(vmc->src1));
    vmc->pc_off(1);
}

void CH_Div_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "div", vmc->dst, vmc->src1);
    vmc->tmp1.j = vmc->getRegisterLong(vmc->dst);
    vmc->tmp2.j = vmc->getRegisterLong(vmc->src1);
    if (vmc->tmp2.j == 0) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u8 == 0x8000000000000000ULL && vmc->tmp2.j == -1LL) {
        vmc->tmp2.j = vmc->tmp1.j;
    } else {
        vmc->tmp2.j = vmc->tmp1.j / vmc->tmp2.j;
    }
    vmc->setRegisterLong(vmc->dst, vmc->tmp2.j);
    vmc->pc_off(2);
}

void CH_Rem_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "rem", vmc->dst, vmc->src1);
    vmc->tmp1.j = vmc->getRegisterLong(vmc->dst);
    vmc->tmp2.j = vmc->getRegisterLong(vmc->src1);
    if (vmc->tmp2.j == 0) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u8 == 0x8000000000000000ULL && vmc->tmp2.j == -1LL) {
        vmc->tmp2.j = 0LL;
    } else {
        vmc->tmp2.j = vmc->tmp1.j / vmc->tmp2.j;
    }
    vmc->setRegisterLong(vmc->dst, vmc->tmp2.j);
    vmc->pc_off(2);
}

void CH_And_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "and", vmc->dst, vmc->src1);
    vmc->setRegisterWide(vmc->dst,
                         vmc->getRegisterWide(vmc->dst)
                         & vmc->getRegisterWide(vmc->src1));
    vmc->pc_off(1);
}

void CH_Or_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "or", vmc->dst, vmc->src1);
    vmc->setRegisterWide(vmc->dst,
                         vmc->getRegisterWide(vmc->dst)
                         | vmc->getRegisterWide(vmc->src1));
    vmc->pc_off(1);
}

void CH_Xor_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "xor", vmc->dst, vmc->src1);
    vmc->setRegisterWide(vmc->dst,
                         vmc->getRegisterWide(vmc->dst)
                         ^ vmc->getRegisterWide(vmc->src1));
    vmc->pc_off(1);
}

void CH_Shl_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "shl", vmc->dst, vmc->src1);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->dst)
                                 << (vmc->getRegister(vmc->src1) & 0x3fu));
    vmc->pc_off(1);
}

void CH_Shr_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "shr", vmc->dst, vmc->src1);
    vmc->setRegisterLong(vmc->dst,
                         vmc->getRegisterLong(vmc->dst)
                                 << (vmc->getRegister(vmc->src1) & 0x3fu));
    vmc->pc_off(1);
}

void CH_Ushr_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%d,v%d", "ushr", vmc->dst, vmc->src1);
    vmc->setRegisterWide(vmc->dst,
                         vmc->getRegisterWide(vmc->dst)
                                 << (vmc->getRegister(vmc->src1) & 0x3fu));
    vmc->pc_off(1);
}

void CH_Add_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%d,v%d", "add", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst,
                          vmc->getRegisterFloat(vmc->dst)
                          + vmc->getRegisterFloat(vmc->src1));
    vmc->pc_off(1);
}

void CH_Sub_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%d,v%d", "sub", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst,
                          vmc->getRegisterFloat(vmc->dst)
                          - vmc->getRegisterFloat(vmc->src1));
    vmc->pc_off(1);
}

void CH_Mul_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%d,v%d", "mul", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst,
                          vmc->getRegisterFloat(vmc->dst)
                          * vmc->getRegisterFloat(vmc->src1));
    vmc->pc_off(1);
}

void CH_Div_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%d,v%d", "div", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst,
                          vmc->getRegisterFloat(vmc->dst)
                          / vmc->getRegisterFloat(vmc->src1));
    vmc->pc_off(1);
}

void CH_Rem_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%d,v%d", "rem", vmc->dst, vmc->src1);
    vmc->setRegisterFloat(vmc->dst,
                          std::fmodf(vmc->getRegisterFloat(vmc->dst),
                                     vmc->getRegisterFloat(vmc->src1)));
    vmc->pc_off(1);
}

void CH_Add_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%d,v%d", "add", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst,
                           vmc->getRegisterDouble(vmc->dst)
                           + vmc->getRegisterDouble(vmc->src1));
    vmc->pc_off(1);
}

void CH_Sub_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%d,v%d", "sub", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst,
                           vmc->getRegisterDouble(vmc->dst)
                           - vmc->getRegisterDouble(vmc->src1));
    vmc->pc_off(1);
}

void CH_Mul_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%d,v%d", "mul", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst,
                           vmc->getRegisterDouble(vmc->dst)
                           * vmc->getRegisterDouble(vmc->src1));
    vmc->pc_off(1);
}

void CH_Div_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%d,v%d", "div", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst,
                           vmc->getRegisterDouble(vmc->dst)
                           / vmc->getRegisterDouble(vmc->src1));
    vmc->pc_off(1);
}

void CH_Rem_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%d,v%d", "rem", vmc->dst, vmc->src1);
    vmc->setRegisterDouble(vmc->dst,
                           std::fmod(vmc->getRegisterDouble(vmc->dst),
                                     vmc->getRegisterDouble(vmc->src1)));
    vmc->pc_off(1);
}

void CH_Add_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%d,v%d,#%dx",
          "add", vmc->dst, vmc->src1, vmc->tmp1.s2);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                        + vmc->tmp1.s2);
    vmc->pc_off(2);
}

void CH_RSub_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%d,v%d,#%dx",
          "rsub", vmc->dst, vmc->src1, vmc->tmp1.s2);
    vmc->setRegisterInt(vmc->dst,
                        vmc->tmp1.s2
                        - vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(2);
}

void CH_Mul_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%d,v%d,#%dx",
          "mul", vmc->dst, vmc->src1, vmc->tmp1.s2);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                        * vmc->tmp1.s2);
    vmc->pc_off(2);
}

void CH_Div_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp2.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%d,v%d,#%dx",
          "div", vmc->dst, vmc->src1, vmc->tmp2.s2);
    vmc->tmp1.s4 = vmc->getRegisterInt(vmc->src1);
    if (vmc->tmp2.s2 == 0) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u4 == 0x80000000u && vmc->tmp2.s2 != -1) {
        vmc->tmp2.s4 = vmc->tmp1.s4;
    } else {
        vmc->tmp2.s4 = vmc->tmp1.s4 / vmc->tmp2.s4;
    }
    vmc->setRegisterInt(vmc->dst, vmc->tmp2.s4);
    vmc->pc_off(2);
}

void CH_Rem_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp2.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%d,v%d,#%dx",
          "rem", vmc->dst, vmc->src1, vmc->tmp2.s2);
    vmc->tmp1.s4 = vmc->getRegisterInt(vmc->src1);
    if (vmc->tmp2.s2 == 0) {
        JAVAException::throwArithmeticException("divide by zero");
        JAVAException::throwJavaException(vmc);
        return;
    }
    if (vmc->tmp1.u4 == 0x80000000u && vmc->tmp2.s2 != -1) {
        vmc->tmp2.s4 = 0;
    } else {
        vmc->tmp2.s4 = vmc->tmp1.s4 % vmc->tmp2.s4;
    }
    vmc->setRegisterInt(vmc->dst, vmc->tmp2.s4);
    vmc->pc_off(2);
}

void CH_And_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%d,v%d,#%dx",
          "and", vmc->dst, vmc->src1, vmc->tmp1.s2);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     & vmc->tmp1.u2);
    vmc->pc_off(2);
}

void CH_Or_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%d,v%d,#%dx",
          "or", vmc->dst, vmc->src1, vmc->tmp1.s2);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     | vmc->tmp1.u2);
    vmc->pc_off(2);
}

void CH_Xor_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();
    vmc->tmp1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%d,v%d,#%dx",
          "xor", vmc->dst, vmc->src1, vmc->tmp1.s2);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     ^ vmc->tmp1.u2);
    vmc->pc_off(2);
}

void CH_Add_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "add", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                        + vmc->tmp1.s1);
    vmc->pc_off(2);
}

void CH_RSub_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "rsub", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->tmp1.s1
                        - vmc->getRegisterInt(vmc->src1));
    vmc->pc_off(2);
}

void CH_Mul_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "mul", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                        * vmc->tmp1.s1);
    vmc->pc_off(2);
}

void CH_Div_Int_Lit8::run(VmMethodContext *vmc) {

}

void CH_Rem_Int_Lit8::run(VmMethodContext *vmc) {

}

void CH_And_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "and", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     & vmc->tmp1.u1);
    vmc->pc_off(2);
}

void CH_Or_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "or", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     | vmc->tmp1.u1);
    vmc->pc_off(2);
}

void CH_Xor_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "add", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                     ^ vmc->tmp1.u1);
    vmc->pc_off(2);
}

void CH_Shl_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "shl", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                                << (vmc->tmp1.u1 & 0x1fu));
    vmc->pc_off(2);
}

void CH_Shr_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "shr", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegisterInt(vmc->dst,
                        vmc->getRegisterInt(vmc->src1)
                                >> (vmc->tmp1.u1 & 0x1fu));
    vmc->pc_off(2);
}

void CH_Ushr_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->src1 = vmc->fetch(1);
    vmc->tmp1.u1 = vmc->src1 >> 8u;
    vmc->src1 = vmc->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%d,v%d,#%d",
          "ushr", vmc->dst, vmc->src1, vmc->tmp1.s1);
    vmc->setRegister(vmc->dst,
                     vmc->getRegister(vmc->src1)
                             >> (vmc->tmp1.u1 & 0x1fu));
    vmc->pc_off(2);
}

void CH_Iget_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-normal-volatile", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterInt(vmc->dst, val.i);
    LOG_D("+ IGET '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-normal-volatile", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.i = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterInt(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-normal-volatile", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->dst, val.i);
    LOG_D("+ SGET '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterInt(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%d,sfield@%u",
          "-normal-volatile", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.i = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterInt(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iget_Object_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-object-volatile", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->dst, val.l);
    LOG_D("+ IGET '%s'=0x%08lx",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          (u8) vmc->getRegisterWide(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iget_Wide_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%d,v%d,field@%u",
          "-wide-volatile", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterWide(vmc->dst, val.j);
    LOG_D("+ IGET '%s'=%ldx",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput_Wide_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-wide-volatile", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.j = vmc->getRegisterLong(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Wide_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-wide-volatile", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterLong(vmc->dst, val.j);
    LOG_D("+ SGET '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Wide_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("|sput%s v%d,sfield@%u",
          "-wide-volatile", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.j = vmc->getRegisterLong(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterLong(vmc->dst));
    vmc->pc_off(2);
}

void CH_Iput_Object_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_A();
    vmc->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%d,v%d,field@%u",
          "-object-volatile", vmc->dst, vmc->src1, vmc->tmp1.u4);
    vmc->tmp2.l = vmc->getRegisterAsObject(vmc->src1);
    if (!JAVAException::checkForNull(vmc->tmp2.l)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    RegValue val{};
    val.l = vmc->getRegisterAsObject(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, vmc->tmp2.l, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=0x%08lx",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterWide(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sget_Object_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%d,sfield@%u",
          "-object-volatile", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->dst, val.l);
    LOG_D("+ SGET '%s'=0x%08lx",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterWide(vmc->dst));
    vmc->pc_off(2);
}

void CH_Sput_Object_Volatile::run(VmMethodContext *vmc) {
    vmc->dst = vmc->inst_AA();
    vmc->tmp1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("|sput%s v%d,sfield@%u",
          "-object-volatile", vmc->dst, vmc->tmp1.u4);
    RegValue val{};
    val.i = vmc->getRegister(vmc->dst);
    if (!vmc->method->resolveSetField(vmc->tmp1.u4, nullptr, &val)) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=0x%08lx",
          vmc->method->resolveFieldName(vmc->tmp1.u4),
          vmc->getRegisterWide(vmc->dst));
    vmc->pc_off(2);
}

void JAVAException::throwJavaException(VmMethodContext *vmc) {
    vmc->curException = (*VM_CONTEXT::env).ExceptionOccurred();
    assert(vmc->curException != nullptr);
    (*VM_CONTEXT::env).ExceptionClear();
}

bool JAVAException::handleJavaException(VmMethodContext *vmc) {

}

bool JAVAException::checkForNull(jobject obj) {
    if (obj == nullptr) {
        JAVAException::throwNullPointerException(nullptr);
        return false;
    }
    return true;
}

void JAVAException::throwNullPointerException(const char *msg) {
    JAVAException::throwNew(VM_REFLECT::C_NAME_NullPointerException, msg);
}

void JAVAException::throwNew(const char *exceptionClassName, const char *msg) {
    JNIEnv *env = VM_CONTEXT::env;
    jclass clazz = (*env).FindClass(exceptionClassName);
    assert(clazz != nullptr);
    (*env).ThrowNew(clazz, msg);
}

void
JAVAException::throwClassCastException(jclass actual, jclass desired) {
    std::string msg = VmMethod::getClassDescriptorByJClass(actual);
    msg += " cannot be cast to ";
    msg += VmMethod::getClassDescriptorByJClass(desired);
    JAVAException::throwNew(VM_REFLECT::C_NAME_ClassCastException, msg.data());
}

void JAVAException::throwNegativeArraySizeException(s4 size) {
    char msgBuf[BUFSIZ];
    sprintf(msgBuf, "%d", size);
    JAVAException::throwNew(VM_REFLECT::C_NAME_NegativeArraySizeException, msgBuf);
}

void JAVAException::throwRuntimeException(const char *msg) {
    JAVAException::throwNew(VM_REFLECT::C_NAME_RuntimeException, msg);
}

void JAVAException::throwInternalError(const char *msg) {
    JAVAException::throwNew(VM_REFLECT::C_NAME_InternalError, msg);
}

void JAVAException::throwArrayIndexOutOfBoundsException(u4 length, u4 index) {
    char msgBuf[BUFSIZ];
    sprintf(msgBuf, "length=%d; index=%d", length, index);
    JAVAException::throwNew(VM_REFLECT::C_NAME_ArrayIndexOutOfBoundsException, msgBuf);
}

void JAVAException::throwArithmeticException(const char *msg) {
    JAVAException::throwNew(VM_REFLECT::C_NAME_ArithmeticException, msg);
}


void CodeHandler::filledNewArray(VmMethodContext *vmc, bool range) {
    JNIEnv *env = VM_CONTEXT::env;

    vmc->tmp1.u4 = vmc->fetch(1);
    vmc->dst = vmc->fetch(2);
    if (range) {
        vmc->src1 = vmc->inst_AA();
        LOG_D("|filled-new-array-range args=%u @%u {regs=v%u-v%u}",
              vmc->src1, vmc->tmp1.u4, vmc->dst, vmc->dst + vmc->src1 - 1);
    } else {
        vmc->src1 = vmc->inst_B();
        LOG_D("|filled-new-array args=%u @%u {regs=%u %u}",
              vmc->src1, vmc->tmp1.u4, vmc->dst, vmc->inst_A());
    }

    vmc->tmp1.lc = vmc->method->resolveClass(vmc->tmp1.u4);
    if (vmc->tmp1.lc == nullptr) {
        JAVAException::throwJavaException(vmc);
        return;
    }
    const std::string desc = VmMethod::getClassDescriptorByJClass(vmc->tmp1.lc);
    assert(desc.size() >= 2);
    assert(desc[0] == '[');
    LOG_D("+++ filled-new-array type is '%s'", desc.data());

    const char typeCh = desc[1];
    if (typeCh == 'D' || typeCh == 'J') {
        /* category 2 primitives not allowed */
        LOG_E("category 2 primitives('D' or 'J') not allowed");
        JAVAException::throwRuntimeException("bad filled array req");
        JAVAException::throwJavaException(vmc);
        return;
    } else if (typeCh != 'L' && typeCh != '[' && typeCh != 'I') {
        LOG_E("non-int primitives not implemented");
        JAVAException::throwInternalError(
                "filled-new-array not implemented for anything but 'int'");
        JAVAException::throwJavaException(vmc);
        return;
    }

    if (typeCh == 'L' || typeCh == '[') {
        jclass elementClazz;
        if (typeCh == 'L') {
            assert(desc.size() > 3);
            elementClazz = (*env).FindClass(
                    desc.substr(2, desc.size() - 1).data());
        } else {
            assert(desc.size() > 2);
            elementClazz = (*env).FindClass(
                    desc.substr(1, desc.size() - 1).data());
        }
        auto *contents = new jobject[vmc->src1]();
        if (range) {
            for (int i = 0; i < vmc->src1; i++) {
                contents[i] = vmc->getRegisterAsObject(vmc->dst + i);
            }
        } else {
            assert(vmc->src1 <= 5);
            // no break
            switch (vmc->src1) {
                case 5:
                    contents[4] = vmc->getRegisterAsObject(vmc->inst_A() & 0x0fu);
                case 4:
                    contents[3] = vmc->getRegisterAsObject(vmc->dst >> 12u);
                case 3:
                    contents[2] = vmc->getRegisterAsObject((vmc->dst & 0x0f00u) >> 8u);
                case 2:
                    contents[1] = vmc->getRegisterAsObject((vmc->dst & 0x00f0u) >> 4u);
                case 1:
                    contents[0] = vmc->getRegisterAsObject((vmc->dst & 0x000fu));
            }
        }

        vmc->tmp1.l = (*env).NewObjectArray(
                vmc->src1, elementClazz, nullptr);
        for (int i = 0; i < vmc->src1; i++) {
            (*env).SetObjectArrayElement(vmc->tmp1.lla, i, contents[i]);
        }
//        (*env).DeleteLocalRef(elementClazz);
        delete[] contents;
    } else {
        u4 *contents = new u4[vmc->src1]();
        if (range) {
            for (int i = 0; i < vmc->src1; ++i) {
                contents[i] = vmc->getRegister(vmc->dst + i);
            }
        } else {
            assert(vmc->src1 <= 5);
            // no break
            switch (vmc->src1) {
                case 5:
                    contents[4] = vmc->getRegister(vmc->inst_A() & 0x0fu);
                case 4:
                    contents[3] = vmc->getRegister(vmc->dst >> 12u);
                case 3:
                    contents[2] = vmc->getRegister((vmc->dst & 0x0f00u) >> 8u);
                case 2:
                    contents[1] = vmc->getRegister((vmc->dst & 0x00f0u) >> 4u);
                case 1:
                    contents[0] = vmc->getRegister((vmc->dst & 0x000fu));
            }
        }
        vmc->tmp1.l = (*env).NewIntArray(vmc->src1);
        (*env).SetIntArrayRegion(vmc->tmp1.lia, 0, vmc->src1, (jint *) contents);
    }

    vmc->retVal->l = vmc->tmp1.l;
    vmc->pc_off(3);
}

s4 CodeHandler::handlePackedSwitch(const u2 *switchData, s4 testVal) {
    const int kInstrLen = 3;
    /*
     * Packed switch data format:
     *  ushort ident = 0x0100   magic value
     *  ushort size             number of entries in the table
     *  int first_key           first (and lowest) switch case value
     *  int targets[size]       branch targets, relative to switch opcode
     *
     * Total size is (4+size*2) 16-bit code units.
     */
    if (*switchData++ != kPackedSwitchSignature) {
        /* should have been caught by verifier */
        JAVAException::throwInternalError("bad packed switch magic");
        return kInstrLen;
    }
    u2 size = *switchData++;
    s4 firstKey = *switchData++;
    firstKey |= (*switchData++) << 16u;
    int index = testVal - firstKey;
    if (index < 0 || index >= size) {
        LOG_E("Value %d not found in switch (%d-%d)",
              testVal, firstKey, firstKey + size - 1);
        return kInstrLen;
    }
    /*
     * The entries are guaranteed to be aligned on a 32-bit boundary;
     * we can treat them as a native int array.
     */
    const s4 *entries = (const s4 *) switchData;
    assert(((u8) entries & 0x03u) == 0);
    assert(index >= 0 && index < size);
    return *(s4 *) (entries + index);
}

s4 CodeHandler::handleSparseSwitch(const u2 *switchData, s4 testVal) {
    const int kInstrLen = 3;

    /*
     * Sparse switch data format:
     *  ushort ident = 0x0200   magic value
     *  ushort size             number of entries in the table; > 0
     *  int keys[size]          keys, sorted low-to-high; 32-bit aligned
     *  int targets[size]       branch targets, relative to switch opcode
     *
     * Total size is (2+size*4) 16-bit code units.
     */

    if (*switchData++ != kSparseSwitchSignature) {
        /* should have been caught by verifier */
        JAVAException::throwInternalError("bad sparse switch magic");
        return kInstrLen;
    }

    u2 size = *switchData++;

    /*
     * The keys are guaranteed to be aligned on a 32-bit boundary;
     * we can treat them as a native int array.
     */
    const s4 *keys = (const s4 *) switchData;
    assert(((u8) keys & 0x03u) == 0);

    /*
     * The entries are guaranteed to be aligned on a 32-bit boundary;
     * we can treat them as a native int array.
     */
    const s4 *entries = keys + size;
    assert(((u8) entries & 0x03u) == 0);

    /*
     * Binary-search through the array of keys, which are guaranteed to
     * be sorted low-to-high.
     */
    uint32_t lo = 0;
    uint32_t hi = size - 1;
    while (lo <= hi) {
        uint32_t mid = (lo + hi) >> 1u;

        s4 foundVal = *(s4 *) (keys + mid);
        if (testVal < foundVal) {
            hi = mid - 1;
        } else if (testVal > foundVal) {
            lo = mid + 1;
        } else {
            return *(s4 *) (entries + mid);
        }
    }

    LOG_E("Value %d not found in switch", testVal);
    return kInstrLen;
}
