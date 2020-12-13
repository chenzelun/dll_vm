//
// Created by 陈泽伦 on 12/3/20.
//

#include "StandardInterpret.h"
#include "../../VmContext.h"
#include "../JavaException.h"
#include "../Vm.h"
#include "../VmStack.h"
#include <cmath>

static const char kSpacing[] = "            ";

void StandardInterpret::run(VmMethodContext *vmc) {
    uint16_t code = vmc->fetch_op();
    LOG_D("opcode: 0x%02x, handler: %p", code, StandardInterpret::codeMap[code]);
    StandardInterpret::codeMap[code]->run(vmc);
}

// TODO
StandardInterpret::StandardInterpret() {
    this->codeMap = {
            // code map start
            {0x00, new ST_CH_NOP()},
            {0x01, new ST_CH_Move()},
            {0x02, new ST_CH_Move_From16()},
            {0x03, new ST_CH_Move_16()},
            {0x04, new ST_CH_Move_Wide()},
            {0x05, new ST_CH_Move_Wide_From16()},
            {0x06, new ST_CH_Move_Wide16()},
            {0x07, new ST_CH_Move_Object()},
            {0x08, new ST_CH_Move_Object_From16()},
            {0x09, new ST_CH_Move_Object16()},
            {0x0a, new ST_CH_Move_Result()},
            {0x0b, new ST_CH_Move_Result_Wide()},
            {0x0c, new ST_CH_Move_Result_Object()},
            {0x0d, new ST_CH_Move_Exception()},
            {0x0e, new ST_CH_Return_Void()},
            {0x0f, new ST_CH_Return()},
            {0x10, new ST_CH_Return_Wide()},
            {0x11, new ST_CH_Return_Object()},
            {0x12, new ST_CH_Const4()},
            {0x13, new ST_CH_Const16()},
            {0x14, new ST_CH_Const()},
            {0x15, new ST_CH_Const_High16()},
            {0x16, new ST_CH_Const_Wide16()},
            {0x17, new ST_CH_Const_Wide32()},
            {0x18, new ST_CH_Const_Wide()},
            {0x19, new ST_CH_Const_Wide_High16()},
            {0x1a, new ST_CH_Const_String()},
            {0x1b, new ST_CH_Const_String_Jumbo()},
            {0x1c, new ST_CH_Const_Class()},
            {0x1d, new ST_CH_Monitor_Enter()},
            {0x1e, new ST_CH_Monitor_Exit()},
            {0x1f, new ST_CH_Check_Cast()},
            {0x20, new ST_CH_Instance_Of()},
            {0x21, new ST_CH_Array_Length()},
            {0x22, new ST_CH_New_Instance()},
            {0x23, new ST_CH_New_Array()},
            {0x24, new ST_CH_Filled_New_Array()},
            {0x25, new ST_CH_Filled_New_Array_Range()},
            {0x26, new ST_CH_Fill_Array_Data()},
            {0x27, new ST_CH_Throw()},
            {0x28, new ST_CH_Goto()},
            {0x29, new ST_CH_Goto16()},
            {0x2a, new ST_CH_Goto32()},
            {0x2b, new ST_CH_Packed_Switch()},
            {0x2c, new ST_CH_Sparse_Switch()},
            {0x2d, new ST_CH_CMPL_Float()},
            {0x2e, new ST_CH_CMPG_Float()},
            {0x2f, new ST_CH_CMPL_Double()},
            {0x30, new ST_CH_CMPG_Double()},
            {0x31, new ST_CH_CMP_Long()},
            {0x32, new ST_CH_IF_EQ()},
            {0x33, new ST_CH_IF_NE()},
            {0x34, new ST_CH_IF_LT()},
            {0x35, new ST_CH_IF_GE()},
            {0x36, new ST_CH_IF_GT()},
            {0x37, new ST_CH_IF_LE()},
            {0x38, new ST_CH_IF_EQZ()},
            {0x39, new ST_CH_IF_NEZ()},
            {0x3a, new ST_CH_IF_LTZ()},
            {0x3b, new ST_CH_IF_GEZ()},
            {0x3c, new ST_CH_IF_GTZ()},
            {0x3d, new ST_CH_IF_LEZ()},
            {0x44, new ST_CH_Aget()},
            {0x45, new ST_CH_Aget_Wide()},
            {0x46, new ST_CH_Aget_Object()},
            {0x47, new ST_CH_Aget_Boolean()},
            {0x48, new ST_CH_Aget_Byte()},
            {0x49, new ST_CH_Aget_Char()},
            {0x4a, new ST_CH_Aget_Short()},
            {0x4b, new ST_CH_Aput()},
            {0x4c, new ST_CH_Aput_Wide()},
            {0x4d, new ST_CH_Aput_Object()},
            {0x4e, new ST_CH_Aput_Boolean()},
            {0x4f, new ST_CH_Aput_Byte()},
            {0x50, new ST_CH_Aput_Char()},
            {0x51, new ST_CH_Aput_Short()},
            {0x52, new ST_CH_Iget()},
            {0x53, new ST_CH_Iget_Wide()},
            {0x54, new ST_CH_Iget_Object()},
            {0x55, new ST_CH_Iget_Boolean()},
            {0x56, new ST_CH_Iget_Byte()},
            {0x57, new ST_CH_Iget_Char()},
            {0x58, new ST_CH_Iget_Short()},
            {0x59, new ST_CH_Iput()},
            {0x5a, new ST_CH_Iput_Wide()},
            {0x5b, new ST_CH_Iput_Object()},
            {0x5c, new ST_CH_Iput_Boolean()},
            {0x5d, new ST_CH_Iput_Byte()},
            {0x5e, new ST_CH_Iput_Char()},
            {0x5f, new ST_CH_Iput_Short()},
            {0x60, new ST_CH_Sget()},
            {0x61, new ST_CH_Sget_Wide()},
            {0x62, new ST_CH_Sget_Object()},
            {0x63, new ST_CH_Sget_Boolean()},
            {0x64, new ST_CH_Sget_Byte()},
            {0x65, new ST_CH_Sget_Char()},
            {0x66, new ST_CH_Sget_Short()},
            {0x67, new ST_CH_Sput()},
            {0x68, new ST_CH_Sput_Wide()},
            {0x69, new ST_CH_Sput_Object()},
            {0x6a, new ST_CH_Sput_Boolean()},
            {0x6b, new ST_CH_Sput_Byte()},
            {0x6c, new ST_CH_Sput_Char()},
            {0x6d, new ST_CH_Sput_Short()},
            {0x6e, new ST_CH_Invoke_Virtual()},
            {0x6f, new ST_CH_Invoke_Super()},
            {0x70, new ST_CH_Invoke_Direct()},
            {0x71, new ST_CH_Invoke_Static()},
            {0x72, new ST_CH_Invoke_Interface()},
            {0x74, new ST_CH_Invoke_Virtual_Range()},
            {0x75, new ST_CH_Invoke_Super_Range()},
            {0x76, new ST_CH_Invoke_Direct_Range()},
            {0x77, new ST_CH_Invoke_Static_Range()},
            {0x78, new ST_CH_Invoke_Interface_Range()},
            {0x7b, new ST_CH_Neg_Int()},
            {0x7c, new ST_CH_Not_Int()},
            {0x7d, new ST_CH_Neg_Long()},
            {0x7e, new ST_CH_Not_Long()},
            {0x7f, new ST_CH_Neg_Float()},
            {0x80, new ST_CH_Neg_Double()},
            {0x81, new ST_CH_Int2Long()},
            {0x82, new ST_CH_Int2Float()},
            {0x83, new ST_CH_Int2Double()},
            {0x84, new ST_CH_Long2Int()},
            {0x85, new ST_CH_Long2Float()},
            {0x86, new ST_CH_Long2Double()},
            {0x87, new ST_CH_Float2Int()},
            {0x88, new ST_CH_Float2Long()},
            {0x89, new ST_CH_Float2Double()},
            {0x8a, new ST_CH_Double2Int()},
            {0x8b, new ST_CH_Double2Long()},
            {0x8c, new ST_CH_Double2Float()},
            {0x8d, new ST_CH_Int2Byte()},
            {0x8e, new ST_CH_Int2Char()},
            {0x8f, new ST_CH_Int2Short()},
            {0x90, new ST_CH_Add_Int()},
            {0x91, new ST_CH_Sub_Int()},
            {0x92, new ST_CH_Mul_Int()},
            {0x93, new ST_CH_Div_Int()},
            {0x94, new ST_CH_Rem_Int()},
            {0x95, new ST_CH_And_Int()},
            {0x96, new ST_CH_Or_Int()},
            {0x97, new ST_CH_Xor_Int()},
            {0x98, new ST_CH_Shl_Int()},
            {0x99, new ST_CH_Shr_Int()},
            {0x9a, new ST_CH_Ushr_Int()},
            {0x9b, new ST_CH_Add_Long()},
            {0x9c, new ST_CH_Sub_Long()},
            {0x9d, new ST_CH_Mul_Long()},
            {0x9e, new ST_CH_Div_Long()},
            {0x9f, new ST_CH_Rem_Long()},
            {0xa0, new ST_CH_And_Long()},
            {0xa1, new ST_CH_Or_Long()},
            {0xa2, new ST_CH_Xor_Long()},
            {0xa3, new ST_CH_Shl_Long()},
            {0xa4, new ST_CH_Shr_Long()},
            {0xa5, new ST_CH_Ushr_Long()},
            {0xa6, new ST_CH_Add_Float()},
            {0xa7, new ST_CH_Sub_Float()},
            {0xa8, new ST_CH_Mul_Float()},
            {0xa9, new ST_CH_Div_Float()},
            {0xaa, new ST_CH_Rem_Float()},
            {0xab, new ST_CH_Add_Double()},
            {0xac, new ST_CH_Sub_Double()},
            {0xad, new ST_CH_Mul_Double()},
            {0xae, new ST_CH_Div_Double()},
            {0xaf, new ST_CH_Rem_Double()},
            {0xb0, new ST_CH_Add_Int_2Addr()},
            {0xb1, new ST_CH_Sub_Int_2Addr()},
            {0xb2, new ST_CH_Mul_Int_2Addr()},
            {0xb3, new ST_CH_Div_Int_2Addr()},
            {0xb4, new ST_CH_Rem_Int_2Addr()},
            {0xb5, new ST_CH_And_Int_2Addr()},
            {0xb6, new ST_CH_Or_Int_2Addr()},
            {0xb7, new ST_CH_Xor_Int_2Addr()},
            {0xb8, new ST_CH_Shl_Int_2Addr()},
            {0xb9, new ST_CH_Shr_Int_2Addr()},
            {0xba, new ST_CH_Ushr_Int_2Addr()},
            {0xbb, new ST_CH_Add_Long_2Addr()},
            {0xbc, new ST_CH_Sub_Long_2Addr()},
            {0xbd, new ST_CH_Mul_Long_2Addr()},
            {0xbe, new ST_CH_Div_Long_2Addr()},
            {0xbf, new ST_CH_Rem_Long_2Addr()},
            {0xc0, new ST_CH_And_Long_2Addr()},
            {0xc1, new ST_CH_Or_Long_2Addr()},
            {0xc2, new ST_CH_Xor_Long_2Addr()},
            {0xc3, new ST_CH_Shl_Long_2Addr()},
            {0xc4, new ST_CH_Shr_Long_2Addr()},
            {0xc5, new ST_CH_Ushr_Long_2Addr()},
            {0xc6, new ST_CH_Add_Float_2Addr()},
            {0xc7, new ST_CH_Sub_Float_2Addr()},
            {0xc8, new ST_CH_Mul_Float_2Addr()},
            {0xc9, new ST_CH_Div_Float_2Addr()},
            {0xca, new ST_CH_Rem_Float_2Addr()},
            {0xcb, new ST_CH_Add_Double_2Addr()},
            {0xcc, new ST_CH_Sub_Double_2Addr()},
            {0xcd, new ST_CH_Mul_Double_2Addr()},
            {0xce, new ST_CH_Div_Double_2Addr()},
            {0xcf, new ST_CH_Rem_Double_2Addr()},
            {0xd0, new ST_CH_Add_Int_Lit16()},
            {0xd1, new ST_CH_RSub_Int_Lit16()},
            {0xd2, new ST_CH_Mul_Int_Lit16()},
            {0xd3, new ST_CH_Div_Int_Lit16()},
            {0xd4, new ST_CH_Rem_Int_Lit16()},
            {0xd5, new ST_CH_And_Int_Lit16()},
            {0xd6, new ST_CH_Or_Int_Lit16()},
            {0xd7, new ST_CH_Xor_Int_Lit16()},
            {0xd8, new ST_CH_Add_Int_Lit8()},
            {0xd9, new ST_CH_RSub_Int_Lit8()},
            {0xda, new ST_CH_Mul_Int_Lit8()},
            {0xdb, new ST_CH_Div_Int_Lit8()},
            {0xdc, new ST_CH_Rem_Int_Lit8()},
            {0xdd, new ST_CH_And_Int_Lit8()},
            {0xde, new ST_CH_Or_Int_Lit8()},
            {0xdf, new ST_CH_Xor_Int_Lit8()},
            {0xe0, new ST_CH_Shl_Int_Lit8()},
            {0xe1, new ST_CH_Shr_Int_Lit8()},
            {0xe2, new ST_CH_Ushr_Int_Lit8()},
            {0xe3, new ST_CH_Iget_Volatile()},
            {0xe4, new ST_CH_Iput_Volatile()},
            {0xe5, new ST_CH_Sget_Volatile()},
            {0xe6, new ST_CH_Sput_Volatile()},
            {0xe7, new ST_CH_Iget_Object_Volatile()},
            {0xe8, new ST_CH_Iget_Wide_Volatile()},
            {0xe9, new ST_CH_Iput_Wide_Volatile()},
            {0xea, new ST_CH_Sget_Wide_Volatile()},
            {0xeb, new ST_CH_Sput_Wide_Volatile()},
            {0xfc, new ST_CH_Iput_Object_Volatile()},
            {0xfd, new ST_CH_Sget_Object_Volatile()},
            {0xfe, new ST_CH_Sput_Object_Volatile()},
            // code map end
    };
}

void StandardInterpret::filledNewArray(VmMethodContext *vmc, bool range) {
    JNIEnv *env = VM_CONTEXT::env;
    vmc->tmp->val_1.u4 = vmc->fetch(1);
    vmc->tmp->dst = vmc->fetch(2);
    if (range) {
        vmc->tmp->src1 = vmc->inst_AA();
        LOG_D("|filled-new-array-range args=%u @%u {regs=v%u-v%u}",
              vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst,
              vmc->tmp->dst + vmc->tmp->src1 - 1);
    } else {
        vmc->tmp->src1 = vmc->inst_B();
        LOG_D("|filled-new-array args=%u @%u {regs=%u %u}",
              vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->inst_A());
    }

    vmc->tmp->val_1.lc = vmc->method->resolveClass(vmc->tmp->val_1.u4);
    if (vmc->tmp->val_1.lc == nullptr) {
        JavaException::throwJavaException(vmc);
        return;
    }
    const std::string desc = VmMethod::getClassDescriptorByJClass(vmc->tmp->val_1.lc);
    assert(desc.size() >= 2);
    assert(desc[0] == '[');
    LOG_D("+++ filled-new-array type is '%s'", desc.data());

    const char typeCh = desc[1];
    if (typeCh == 'D' || typeCh == 'J') {
        /* category 2 primitives not allowed */
        LOG_E("category 2 primitives('D' or 'J') not allowed");
        JavaException::throwRuntimeException(vmc, "bad filled array req");
        return;
    } else if (typeCh != 'L' && typeCh != '[' && typeCh != 'I') {
        LOG_E("non-int primitives not implemented");
        JavaException::throwInternalError(
                vmc, "filled-new-array not implemented for anything but 'int'");
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
        auto *contents = new jobject[vmc->tmp->src1]();
        if (range) {
            for (int i = 0; i < vmc->tmp->src1; i++) {
                contents[i] = vmc->getRegisterAsObject(vmc->tmp->dst + i);
            }
        } else {
            assert(vmc->tmp->src1 <= 5);
            // no break
            switch (vmc->tmp->src1) {
                case 5:
                    contents[4] = vmc->getRegisterAsObject(vmc->inst_A() & 0x0fu);
                case 4:
                    contents[3] = vmc->getRegisterAsObject(vmc->tmp->dst >> 12u);
                case 3:
                    contents[2] = vmc->getRegisterAsObject((vmc->tmp->dst & 0x0f00u) >> 8u);
                case 2:
                    contents[1] = vmc->getRegisterAsObject((vmc->tmp->dst & 0x00f0u) >> 4u);
                case 1:
                    contents[0] = vmc->getRegisterAsObject((vmc->tmp->dst & 0x000fu));
            }
        }

        vmc->tmp->val_1.l = (*env).NewObjectArray(
                vmc->tmp->src1, elementClazz, nullptr);
        for (int i = 0; i < vmc->tmp->src1; i++) {
            (*env).SetObjectArrayElement(vmc->tmp->val_1.lla, i, contents[i]);
        }
//        (*env).DeleteLocalRef(elementClazz);
        delete[] contents;
    } else {
        u4 *contents = new u4[vmc->tmp->src1]();
        if (range) {
            for (int i = 0; i < vmc->tmp->src1; ++i) {
                contents[i] = vmc->getRegister(vmc->tmp->dst + i);
            }
        } else {
            assert(vmc->tmp->src1 <= 5);
            // no break
            switch (vmc->tmp->src1) {
                case 5:
                    contents[4] = vmc->getRegister(vmc->inst_A() & 0x0fu);
                case 4:
                    contents[3] = vmc->getRegister(vmc->tmp->dst >> 12u);
                case 3:
                    contents[2] = vmc->getRegister((vmc->tmp->dst & 0x0f00u) >> 8u);
                case 2:
                    contents[1] = vmc->getRegister((vmc->tmp->dst & 0x00f0u) >> 4u);
                case 1:
                    contents[0] = vmc->getRegister((vmc->tmp->dst & 0x000fu));
            }
        }
        vmc->tmp->val_1.l = (*env).NewIntArray(vmc->tmp->src1);
        (*env).SetIntArrayRegion(vmc->tmp->val_1.lia, 0, vmc->tmp->src1, (jint *) contents);
    }

    vmc->retVal->l = vmc->tmp->val_1.l;
    vmc->pc_off(3);
}

s4 StandardInterpret::handlePackedSwitch(VmMethodContext *vmc, const u2 *switchData, s4 testVal) {
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
        JavaException::throwInternalError(vmc, "bad packed switch magic");
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

s4 StandardInterpret::handleSparseSwitch(VmMethodContext *vmc, const u2 *switchData, s4 testVal) {
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
        JavaException::throwInternalError(vmc, "bad sparse switch magic");
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

#if defined(VM_DEBUG)

void
StandardInterpret::debugInvokeMethod(VmMethodContext *vmc, jmethodID methodCalled,
                                     const char *shorty, const jvalue retVal,
                                     const jvalue *params) {
    LOG_D("methodCalled info:");
    VmMethod vmMethod(methodCalled, false);
    LOG_D("methodCalled: %s#%s", vmMethod.clazzDescriptor, vmMethod.name);
    LOG_D("methodCalled sign: %s", vmMethod.resolveMethodSign(
            vmMethod.dexFile->dexGetMethodId(vmMethod.method_id)->protoIdx).data());
    LOG_D("");
    LOG_D("VmMethodContext info:");
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


void ST_CH_NOP::run(VmMethodContext *vmc) {
    LOG_D("NOP");
    vmc->pc_off(1);
}

void ST_CH_Move::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|move%s v%u,v%u %s(v%u=%d)",
          "", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst,
          vmc->getRegisterInt(vmc->tmp->src1));
    vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Move_From16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    LOG_D("|move%s/from16 v%u,v%u %s(v%u=%d)",
          "", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst,
          vmc->getRegisterInt(vmc->tmp->src1));
    vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(2);
}

void ST_CH_Move_16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->fetch(1);
    vmc->tmp->src1 = vmc->fetch(2);
    LOG_D("|move%s/16 v%u,v%u %s(v%u=%d)",
          "", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst,
          vmc->getRegisterInt(vmc->tmp->src1));
    vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(3);
}

void ST_CH_Move_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|move-wide v%u,v%u %s(v%u=%ld)",
          vmc->tmp->dst, vmc->tmp->src1, kSpacing + 5, vmc->tmp->dst,
          vmc->getRegisterLong(vmc->tmp->src1));
    vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Move_Wide_From16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    LOG_D("|move-wide/from16 v%u,v%u  (v%u=%ld)",
          vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->dst,
          vmc->getRegisterLong(vmc->tmp->src1));
    vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(2);
}

void ST_CH_Move_Wide16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->fetch(1);
    vmc->tmp->src1 = vmc->fetch(2);
    LOG_D("|move-wide/16 v%u,v%u %s(v%u=%ld)",
          vmc->tmp->dst, vmc->tmp->src1, kSpacing + 8, vmc->tmp->dst,
          vmc->getRegisterWide(vmc->tmp->src1));
    vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(3);
}

void ST_CH_Move_Object::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|move%s v%u,v%u %s(v%u=%p)", "-object",
          vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst,
          vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Move_Object_From16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    LOG_D("|move%s/from16 v%u,v%u %s(v%u=%p)",
          "-object", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst,
          vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->pc_off(2);
}

void ST_CH_Move_Object16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->fetch(1);
    vmc->tmp->src1 = vmc->fetch(2);
    LOG_D("|move%s/16 v%u,v%u %s(v%u=%p)",
          "-object", vmc->tmp->dst, vmc->tmp->src1, kSpacing, vmc->tmp->dst,
          vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->pc_off(3);
}

void ST_CH_Move_Result::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    LOG_D("|move-result%s v%u %s(v%u=%d)",
          "", vmc->tmp->dst, kSpacing + 4, vmc->tmp->dst, vmc->retVal->i);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);
    vmc->pc_off(1);
}

void ST_CH_Move_Result_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    LOG_D("|move-result-wide v%u %s(v%u=%ld)",
          vmc->tmp->dst, kSpacing, vmc->tmp->dst, vmc->retVal->j);
    vmc->setRegisterLong(vmc->tmp->dst, vmc->retVal->j);
    vmc->pc_off(1);
}

void ST_CH_Move_Result_Object::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    LOG_D("|move-result%s v%u %s(v%u=0x%p)",
          "-object", vmc->tmp->dst, kSpacing + 4, vmc->tmp->dst, vmc->retVal->l);
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->retVal->l);
    vmc->pc_off(1);
}

void ST_CH_Move_Exception::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    LOG_D("|move-exception v%u", vmc->tmp->dst);
    assert(vmc->curException != nullptr);
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->curException);
    vmc->curException = nullptr;
    vmc->pc_off(1);
}

void ST_CH_Return_Void::run(VmMethodContext *vmc) {
    LOG_D("|return-void");
    vmc->finish();
}

void ST_CH_Return::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    LOG_D("|return%s v%u", "", vmc->tmp->src1);
    vmc->retVal->j = 0L;    // set 0
    vmc->retVal->i = vmc->getRegisterInt(vmc->tmp->src1);
    vmc->finish();
}

void ST_CH_Return_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    LOG_D("return-wide v%u", vmc->tmp->src1);
    vmc->retVal->j = vmc->getRegisterLong(vmc->tmp->src1);
    vmc->finish();
}

void ST_CH_Return_Object::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    LOG_D("|return%s v%u", "-object", vmc->tmp->src1);
    vmc->retVal->l = vmc->getRegisterAsObject(vmc->tmp->src1);
    vmc->finish();
}

void ST_CH_Const4::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->val_1.s4 = (s4) (vmc->inst_B() << 28) >> 28;  // sign extend 4-bit value
    LOG_D("|const/4 v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s4);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_1.s4);
    vmc->pc_off(1);
}

void ST_CH_Const16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|const/16 v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s2);
    vmc->setRegister(vmc->tmp->dst, vmc->tmp->val_1.s2);
    vmc->pc_off(2);
}

void ST_CH_Const::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);
    vmc->tmp->val_1.u4 |= (u4) vmc->fetch(2) << 16u;
    LOG_D("|const v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s4);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_1.s4);
    vmc->pc_off(3);
}

void ST_CH_Const_High16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    LOG_D("|const/high16 v%u,#0x%04x0000", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst, (u4) vmc->tmp->src1 << 16u);
    vmc->pc_off(2);
}

void ST_CH_Const_Wide16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|const-wide/16 v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s2);
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_1.s2);
    vmc->pc_off(2);
}

void ST_CH_Const_Wide32::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);
    vmc->tmp->val_1.u4 |= (u4) vmc->fetch(2) << 16u;
    LOG_D("|const-wide/32 v%u,#%d", vmc->tmp->dst, vmc->tmp->val_1.s4);
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_1.s4);
    vmc->pc_off(3);
}

void ST_CH_Const_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u8 = vmc->fetch(1);
    vmc->tmp->val_1.u8 |= (u8) vmc->fetch(2) << 16u;
    vmc->tmp->val_1.u8 |= (u8) vmc->fetch(3) << 32u;
    vmc->tmp->val_1.u8 |= (u8) vmc->fetch(4) << 48u;
    LOG_D("|const-wide v%u,#%ld", vmc->tmp->dst, vmc->tmp->val_1.s8);
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_1.s8);
    vmc->pc_off(5);
}

void ST_CH_Const_Wide_High16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|const-wide/high16 v%u,#0x%04x000000000000",
          vmc->tmp->dst, vmc->tmp->src1);
    vmc->tmp->val_2.u8 = ((u8) vmc->tmp->src1) << 48u;
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.s8);
    vmc->pc_off(2);
}

void ST_CH_Const_String::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);
    LOG_D("|const-string v%u string@%u", vmc->tmp->dst, vmc->tmp->val_1.u4);
    vmc->tmp->val_1.l = vmc->method->resolveString(vmc->tmp->val_1.u4);
    assert(vmc->tmp->val_1.l != nullptr);
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_1.l);
    vmc->pc_off(2);
}

void ST_CH_Const_String_Jumbo::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);
    vmc->tmp->val_1.u4 |= (u4) vmc->fetch(2) << 16u;
    LOG_D("|const-string/jumbo v%u string@%u", vmc->tmp->dst, vmc->tmp->val_1.u4);
    vmc->tmp->val_1.l = vmc->method->resolveString(vmc->tmp->val_1.u4);
    assert(vmc->tmp->val_1.l != nullptr);
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_1.l);
    vmc->pc_off(3);
}

void ST_CH_Const_Class::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);
    LOG_D("|const-class v%u class@%u", vmc->tmp->dst, vmc->tmp->val_1.u4);
    vmc->tmp->val_1.l = vmc->method->resolveClass(vmc->tmp->val_1.u4);
    if (vmc->tmp->val_1.l == nullptr) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_1.l);
    vmc->pc_off(2);
}

void ST_CH_Monitor_Enter::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    LOG_D("|monitor-enter v%u %s(%p)",
          vmc->tmp->src1, kSpacing + 6, vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.l)) {
        return;
    }
    (*VM_CONTEXT::env).MonitorEnter(vmc->tmp->val_1.l);
    vmc->pc_off(1);
}

void ST_CH_Monitor_Exit::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    LOG_D("|monitor-exit v%u %s(%p)",
          vmc->tmp->src1, kSpacing + 5, vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    /**
     * 注意：如果该指令需要抛出异常，则必须像 PC 已超出该指令那样抛出。
     * 不妨将其想象成，该指令（在某种意义上）已成功执行，并在该指令执行后
     * 但下一条指令找到机会执行前抛出异常。这种定义使得某个方法有可能将
     * 监视锁清理 catch-all（例如 finally）分块用作该分块自身的
     * 监视锁清理，以便处理可能由于 Thread.stop() 的既往实现而
     * 抛出的任意异常，同时仍尽力维持适当的监视锁安全机制。
     */
    vmc->pc_off(1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.l)) {
        return;
    }
    if (!(*VM_CONTEXT::env).MonitorExit(vmc->tmp->val_1.l)) {
        JavaException::throwJavaException(vmc);
        return;
    }
}

void ST_CH_Check_Cast::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    vmc->tmp->val_2.u4 = vmc->fetch(1);       /* class to check against */
    LOG_D("|check-cast v%u,class@%u", vmc->tmp->src1, vmc->tmp->val_2.u4);
    vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (vmc->tmp->val_1.l) {
        vmc->tmp->val_2.lc = vmc->method->resolveClass(vmc->tmp->val_2.u4);
        if (vmc->tmp->val_2.lc == nullptr) {
            JavaException::throwJavaException(vmc);
            return;
        }
        if (!(*VM_CONTEXT::env).IsInstanceOf(vmc->tmp->val_1.l, vmc->tmp->val_2.lc)) {
            JavaException::throwClassCastException(
                    vmc,
                    (*VM_CONTEXT::env).GetObjectClass(vmc->tmp->val_1.l), vmc->tmp->val_2.lc);
            return;
        }
//        (*VM_CONTEXT::env).DeleteLocalRef(clazz);
    }
    vmc->pc_off(2);
}

void ST_CH_Instance_Of::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_2.u4 = vmc->fetch(1);
    LOG_D("|instance-of v%u,v%u,class@%u",
          vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.u4);
    vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (vmc->tmp->val_1.l == nullptr) {
        vmc->setRegister(vmc->tmp->dst, 0);
    } else {
        vmc->tmp->val_2.lc = vmc->method->resolveClass(vmc->tmp->val_2.u4);
        if (vmc->tmp->val_2.lc == nullptr) {
            JavaException::throwJavaException(vmc);
            return;
        }
        vmc->tmp->val_1.z = (*VM_CONTEXT::env).IsInstanceOf(vmc->tmp->val_1.l, vmc->tmp->val_2.lc);
        vmc->setRegister(vmc->tmp->dst, vmc->tmp->val_1.z);
    }
//    (*VM_CONTEXT::env).DeleteLocalRef(clazz);
    vmc->pc_off(2);
}

void ST_CH_Array_Length::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    LOG_D("|array-length v%u,v%u  (%p)",
          vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.l);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.l)) {
        return;
    }
    vmc->tmp->val_1.u4 = (u4) (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.la);
    vmc->setRegister(vmc->tmp->dst, vmc->tmp->val_1.u4);
    vmc->pc_off(1);
}

void ST_CH_New_Instance::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);
    LOG_D("|new-instance v%u,class@%u", vmc->tmp->dst, vmc->tmp->val_1.u4);
    vmc->tmp->val_1.l = vmc->method->resolveClass(vmc->tmp->val_1.u4);
    if (vmc->tmp->val_1.l == nullptr) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->tmp->val_2.l = (*VM_CONTEXT::env).AllocObject(vmc->tmp->val_1.lc);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_2.l);
//    (*VM_CONTEXT::env).DeleteLocalRef(vmc->tmp->val_1.l);
    vmc->pc_off(2);
}

void ST_CH_New_Array::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_1.u4 = vmc->fetch(1);
    LOG_D("|new-array v%u,v%u,class@%u  (%d elements)",
          vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->getRegisterInt(vmc->tmp->src1));
    vmc->tmp->val_2.s4 = vmc->getRegisterInt(vmc->tmp->src1);
    if (vmc->tmp->val_2.s4 < 0) {
        JavaException::throwNegativeArraySizeException(vmc, vmc->tmp->val_2.s4);
        return;
    }
    vmc->tmp->val_1.la = vmc->method->allocArray(vmc->tmp->val_2.s4, vmc->tmp->val_1.u4);
    if (vmc->tmp->val_1.la == nullptr) {
        JavaException::throwRuntimeException(vmc, "error type of field... cc");
        return;
    }
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_1.l);
    vmc->pc_off(2);
}

void ST_CH_Filled_New_Array::run(VmMethodContext *vmc) {
    LOG_D("|filled_new_array");
    StandardInterpret::filledNewArray(vmc, false);
}

void ST_CH_Filled_New_Array_Range::run(VmMethodContext *vmc) {
    LOG_D("|filled_new_array_range");
    StandardInterpret::filledNewArray(vmc, true);
}

void ST_CH_Fill_Array_Data::run(VmMethodContext *vmc) {
    JNIEnv *env = VM_CONTEXT::env;
    vmc->tmp->src1 = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1) | (((u4) vmc->fetch(2)) << 16u);
    LOG_D("|fill-array-data v%u +%d", vmc->tmp->src1, vmc->tmp->val_1.s4);

    const u2 *data = vmc->arrayData(vmc->tmp->val_1.s4);
    vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);
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
        JavaException::throwInternalError(vmc, "bad array data magic");
        return;
    }
    u4 size = data[2] | ((u4) data[3] << 16u);
    if (size > (*env).GetArrayLength(vmc->tmp->val_1.la)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, (*env).GetArrayLength(vmc->tmp->val_1.la), size);
        return;
    }
    const std::string desc = VmMethod::getClassDescriptorByJClass(
            (*env).GetObjectClass(vmc->tmp->val_1.l));
    switch (desc[1]) {
        case 'I':
            (*env).SetIntArrayRegion(vmc->tmp->val_1.lia, 0, size, (jint *) (data + 4));
            break;

        case 'C':
            (*env).SetCharArrayRegion(vmc->tmp->val_1.lca, 0, size, (jchar *) (data + 4));
            break;

        case 'Z':
            (*env).SetBooleanArrayRegion(vmc->tmp->val_1.lza, 0, size, (jboolean *) (data + 4));
            break;

        case 'B':
            (*env).SetByteArrayRegion(vmc->tmp->val_1.lba, 0, size, (jbyte *) (data + 4));
            break;

        case 'F':
            (*env).SetFloatArrayRegion(vmc->tmp->val_1.lfa, 0, size, (jfloat *) (data + 4));
            break;

        case 'D':
            (*env).SetDoubleArrayRegion(vmc->tmp->val_1.lda, 0, size, (jdouble *) (data + 4));
            break;

        case 'S':
            (*env).SetShortArrayRegion(vmc->tmp->val_1.lsa, 0, size, (jshort *) (data + 4));
            break;

        case 'J':
            (*env).SetLongArrayRegion(vmc->tmp->val_1.lja, 0, size, (jlong *) (data + 4));
            break;

        default:
            LOG_E("Unknown primitive type '%c'", desc[1]);
            throw VMException("error type of field... cc");
            return;
    }
    vmc->pc_off(3);
}

void ST_CH_Throw::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    LOG_D("throw v%u  (%p)",
          vmc->tmp->src1, vmc->getRegisterAsObject(vmc->tmp->src1));
    vmc->tmp->val_1.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.l)) {
        /* will throw a null pointer exception */
        LOG_E("Bad exception");
        return;
    } else {
        /* use the requested exception */
        (*VM_CONTEXT::env).Throw(vmc->tmp->val_1.lt);
        JavaException::throwJavaException(vmc);
    }
    // no pc_off.
}

void ST_CH_Goto::run(VmMethodContext *vmc) {
    vmc->tmp->val_1.s1 = vmc->inst_AA();
    if (vmc->tmp->val_1.s1 < 0) {
        LOG_D("|goto -%d", -(vmc->tmp->val_1.s1));
    } else {
        LOG_D("|goto +%d", (vmc->tmp->val_1.s1));
        LOG_D("> branch taken");
    }
    vmc->goto_off(vmc->tmp->val_1.s1);
    // no pc_off
}

void ST_CH_Goto16::run(VmMethodContext *vmc) {
    vmc->tmp->val_1.s2 = vmc->fetch(1);   /* sign-extend next code unit */
    if (vmc->tmp->val_1.s2 < 0) {
        LOG_D("|goto -%d", -(vmc->tmp->val_1.s2));
    } else {
        LOG_D("|goto +%d", (vmc->tmp->val_1.s2));
        LOG_D("> branch taken");
    }
    vmc->goto_off(vmc->tmp->val_1.s2);
    // no pc_off
}

void ST_CH_Goto32::run(VmMethodContext *vmc) {
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* low-order 16 bits */
    vmc->tmp->val_1.u4 |= ((u4) vmc->fetch(2)) << 16u;    /* high-order 16 bits */
    if (vmc->tmp->val_1.s4 < 0) {
        LOG_D("|goto -%d", -(vmc->tmp->val_1.s4));
    } else {
        LOG_D("|goto +%d", (vmc->tmp->val_1.s4));
        LOG_D("> branch taken");
    }
    vmc->goto_off(vmc->tmp->val_1.s4);
    // no pc_off
}

void ST_CH_Packed_Switch::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1) | ((u4) vmc->fetch(2) << 16u);
    LOG_D("|packed-switch v%u +%d", vmc->tmp->src1, vmc->tmp->val_1.s4);
    const u2 *data = vmc->arrayData(vmc->tmp->val_1.s4);   // offset in 16-bit units
    vmc->tmp->val_1.u4 = vmc->getRegister(vmc->tmp->src1);
    vmc->tmp->val_1.s4 = StandardInterpret::handlePackedSwitch(
            vmc, data, vmc->tmp->val_1.u4);
    LOG_D("> branch taken (%d)", vmc->tmp->val_1.s4);
    vmc->goto_off(vmc->tmp->val_1.s4);
    // no pc_off
}

void ST_CH_Sparse_Switch::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1) | ((u4) vmc->fetch(2) << 16u);
    LOG_D("|packed-switch v%u +%d", vmc->tmp->src1, vmc->tmp->val_1.s4);
    const u2 *data = vmc->arrayData(vmc->tmp->val_1.s4);   // offset in 16-bit units
    vmc->tmp->val_1.u4 = vmc->getRegister(vmc->tmp->src1);
    vmc->tmp->val_1.s4 = StandardInterpret::handleSparseSwitch(
            vmc, data, vmc->tmp->val_1.u4);
    LOG_D("> branch taken (%d)", vmc->tmp->val_1.s4);
    vmc->goto_off(vmc->tmp->val_1.s4);
    // no pc_off
}

void ST_CH_CMPL_Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "l-float", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.f = vmc->getRegisterFloat(vmc->tmp->src1);
    vmc->tmp->val_2.f = vmc->getRegisterFloat(vmc->tmp->src2);
    if (vmc->tmp->val_1.f == vmc->tmp->val_2.f) {
        vmc->retVal->i = 0;
    } else if (vmc->tmp->val_1.f > vmc->tmp->val_2.f) {
        vmc->retVal->i = 1;
    } else {
        vmc->retVal->i = -1;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void ST_CH_CMPG_Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "g-float", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.f = vmc->getRegisterFloat(vmc->tmp->src1);
    vmc->tmp->val_2.f = vmc->getRegisterFloat(vmc->tmp->src2);
    if (vmc->tmp->val_1.f == vmc->tmp->val_2.f) {
        vmc->retVal->i = 0;
    } else if (vmc->tmp->val_1.f < vmc->tmp->val_2.f) {
        vmc->retVal->i = -1;
    } else {
        vmc->retVal->i = 1;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void ST_CH_CMPL_Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "l-double", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.d = vmc->getRegisterDouble(vmc->tmp->src1);
    vmc->tmp->val_2.d = vmc->getRegisterDouble(vmc->tmp->src2);
    if (vmc->tmp->val_1.d == vmc->tmp->val_2.d) {
        vmc->retVal->i = 0;
    } else if (vmc->tmp->val_1.d > vmc->tmp->val_2.d) {
        vmc->retVal->i = 1;
    } else {
        vmc->retVal->i = -1;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void ST_CH_CMPG_Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "g-double", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.d = vmc->getRegisterDouble(vmc->tmp->src1);
    vmc->tmp->val_2.d = vmc->getRegisterDouble(vmc->tmp->src2);
    if (vmc->tmp->val_1.d == vmc->tmp->val_2.d) {
        vmc->retVal->i = 0;
    } else if (vmc->tmp->val_1.d < vmc->tmp->val_2.d) {
        vmc->retVal->i = -1;
    } else {
        vmc->retVal->i = 1;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void ST_CH_CMP_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|cmp%s v%u,v%u,v%u", "-long", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.s8 = vmc->getRegisterWide(vmc->tmp->src1);
    vmc->tmp->val_2.s8 = vmc->getRegisterWide(vmc->tmp->src2);
    if (vmc->tmp->val_1.s8 > vmc->tmp->val_2.s8) {
        vmc->retVal->i = 1;
    } else if (vmc->tmp->val_1.s8 < vmc->tmp->val_2.s8) {
        vmc->retVal->i = -1;
    } else {
        vmc->retVal->i = 0;
    }
    LOG_D("+ result=%d", vmc->retVal->i);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->retVal->i);
    vmc->pc_off(2);
}

void ST_CH_IF_EQ::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    vmc->tmp->src2 = vmc->inst_B();
    vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);
    vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);
    if (vmc->tmp->val_1.s4 == vmc->tmp->val_2.s4) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "eq", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u",
              "eq", vmc->tmp->src1, vmc->tmp->src2);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_NE::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    vmc->tmp->src2 = vmc->inst_B();
    vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);
    vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);
    if (vmc->tmp->val_1.s4 != vmc->tmp->val_2.s4) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "ne", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u",
              "ne", vmc->tmp->src1, vmc->tmp->src2);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_LT::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    vmc->tmp->src2 = vmc->inst_B();
    vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);
    vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);
    if (vmc->tmp->val_1.s4 < vmc->tmp->val_2.s4) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "lt", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u",
              "lt", vmc->tmp->src1, vmc->tmp->src2);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_LE::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    vmc->tmp->src2 = vmc->inst_B();
    vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);
    vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);
    if (vmc->tmp->val_1.s4 <= vmc->tmp->val_2.s4) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "le", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u",
              "le", vmc->tmp->src1, vmc->tmp->src2);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_GT::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    vmc->tmp->src2 = vmc->inst_B();
    vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);
    vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);
    if (vmc->tmp->val_1.s4 > vmc->tmp->val_2.s4) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "gt", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u",
              "gt", vmc->tmp->src1, vmc->tmp->src2);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_GE::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    vmc->tmp->src2 = vmc->inst_B();
    vmc->tmp->val_1.s4 = (s4) vmc->getRegister(vmc->tmp->src1);
    vmc->tmp->val_2.s4 = (s4) vmc->getRegister(vmc->tmp->src2);
    if (vmc->tmp->val_1.s4 >= vmc->tmp->val_2.s4) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u,+%d",
              "ge", vmc->tmp->src1, vmc->tmp->src2, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u,v%u",
              "ge", vmc->tmp->src1, vmc->tmp->src2);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_EQZ::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->tmp->src1) == 0) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "eqz", vmc->tmp->src1, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u", "eqz", vmc->tmp->src1);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_NEZ::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->tmp->src1) != 0) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "nez", vmc->tmp->src1, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u", "nez", vmc->tmp->src1);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_LTZ::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->tmp->src1) < 0) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "ltz", vmc->tmp->src1, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u", "ltz", vmc->tmp->src1);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_GEZ::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->tmp->src1) >= 0) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "gez", vmc->tmp->src1, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u", "gez", vmc->tmp->src1);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_GTZ::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->tmp->src1) > 0) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "gtz", vmc->tmp->src1, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u", "gtz", vmc->tmp->src1);
        vmc->pc_off(2);
    }
}

void ST_CH_IF_LEZ::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_A();
    if ((s4) vmc->getRegister(vmc->tmp->src1) <= 0) {
        vmc->tmp->val_1.s4 = (s2) vmc->fetch(1); /* sign-extended */
        LOG_D("|if-%s v%u,v%u",
              "lez", vmc->tmp->src1, vmc->tmp->val_1.s4);
        LOG_D("> branch taken");
        vmc->goto_off(vmc->tmp->val_1.s4);
    } else {
        LOG_D("|if-%s v%u", "lez", vmc->tmp->src1);
        vmc->pc_off(2);
    }
}

void ST_CH_Aget::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-normal", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lia = (jintArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lia)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lia);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetIntArrayRegion(
            vmc->tmp->val_1.lia, vmc->getRegister(vmc->tmp->src2), 1, (jint *) buf);
    vmc->setRegisterInt(vmc->tmp->dst, *(jint *) buf);
    LOG_D("+ AGET[%u]=%d",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegisterInt(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Aget_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-wide", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lja = (jlongArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lja)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lja);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetLongArrayRegion(
            vmc->tmp->val_1.lja, vmc->getRegister(vmc->tmp->src2), 1, (jlong *) buf);
    vmc->setRegisterLong(vmc->tmp->dst, *(jlong *) buf);
    LOG_D("+ AGET[%u]=%ld",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Aget_Object::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("|aget%s v%u,v%u,v%u",
          "-object", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lla = (jobjectArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lla)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lla);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    vmc->tmp->val_2.l = (*VM_CONTEXT::env).GetObjectArrayElement(
            vmc->tmp->val_1.lla, vmc->getRegister(vmc->tmp->src2));
    vmc->setRegisterAsObject(vmc->tmp->dst, vmc->tmp->val_2.l);
    LOG_D("+ AGET[%u]=%p",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Aget_Boolean::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-boolean", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lza = (jbooleanArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lza)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lza);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetBooleanArrayRegion(
            vmc->tmp->val_1.lza, vmc->getRegister(vmc->tmp->src2), 1, (jboolean *) buf);
    vmc->setRegister(vmc->tmp->dst, *(jboolean *) buf);
    LOG_D("+ AGET[%u]=%u",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Aget_Byte::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-byte", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lba = (jbyteArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lba)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lba);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetByteArrayRegion(
            vmc->tmp->val_1.lba, vmc->getRegister(vmc->tmp->src2), 1, (jbyte *) buf);
    vmc->setRegister(vmc->tmp->dst, *(jbyte *) buf);
    LOG_D("+ AGET[%u]=%u",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Aget_Char::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-char", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lca = (jcharArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lca)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lca);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetCharArrayRegion(
            vmc->tmp->val_1.lca, vmc->getRegister(vmc->tmp->src2), 1, (jchar *) buf);
    vmc->setRegister(vmc->tmp->dst, *(jchar *) buf);
    LOG_D("+ AGET[%u]=%u",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Aget_Short::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-short", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lsa = (jshortArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lsa)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lsa);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    u8 buf[1];
    (*VM_CONTEXT::env).GetShortArrayRegion(
            vmc->tmp->val_1.lsa, vmc->getRegister(vmc->tmp->src2), 1, (jshort *) buf);
    vmc->setRegister(vmc->tmp->dst, *(jshort *) buf);
    LOG_D("+ AGET[%u]=%u",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Aput::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();      /* AA: source value */
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-normal", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lia = (jintArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lia)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lia);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    LOG_D("+ APUT[%u]=%d",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegisterInt(vmc->tmp->dst));
    u8 buf[1];
    *(jint *) buf = vmc->getRegister(vmc->tmp->dst);
    (*VM_CONTEXT::env).SetIntArrayRegion(
            vmc->tmp->val_1.lia, vmc->getRegister(vmc->tmp->src2), 1, (jint *) buf);
    vmc->pc_off(2);
}

void ST_CH_Aput_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();      /* AA: source value */
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-wide", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lja = (jlongArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lja)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lja);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    LOG_D("+ APUT[%u]=%ld",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegisterLong(vmc->tmp->dst));
    u8 buf[1];
    *(jlong *) buf = vmc->getRegisterLong(vmc->tmp->dst);
    (*VM_CONTEXT::env).SetLongArrayRegion(
            vmc->tmp->val_1.lja, vmc->getRegister(vmc->tmp->src2), 1, (jlong *) buf);
    vmc->pc_off(2);
}


void ST_CH_Aput_Object::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();      /* AA: source value */
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u", "-object", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lla = (jobjectArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lla)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lla);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    LOG_D("+ APUT[%u]=%p", vmc->getRegister(vmc->tmp->src2),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->dst);
    (*VM_CONTEXT::env).SetObjectArrayElement(
            vmc->tmp->val_1.lla, vmc->getRegister(vmc->tmp->src2), vmc->tmp->val_2.l);
    vmc->pc_off(2);
}

void ST_CH_Aput_Boolean::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();      /* AA: source value */
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-boolean", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lza = (jbooleanArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lza)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lza);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    LOG_D("+ APUT[%u]=%u",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegister(vmc->tmp->dst));
    u8 buf[1];
    *(jboolean *) buf = vmc->getRegister(vmc->tmp->dst);
    (*VM_CONTEXT::env).SetBooleanArrayRegion(
            vmc->tmp->val_1.lza, vmc->getRegister(vmc->tmp->src2), 1, (jboolean *) buf);
    vmc->pc_off(2);
}

void ST_CH_Aput_Byte::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();      /* AA: source value */
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-byte", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lba = (jbyteArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lba)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lba);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    LOG_D("+ APUT[%u]=%u",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegister(vmc->tmp->dst));
    u8 buf[1];
    *(jbyte *) buf = vmc->getRegister(vmc->tmp->dst);
    (*VM_CONTEXT::env).SetByteArrayRegion(
            vmc->tmp->val_1.lba, vmc->getRegister(vmc->tmp->src2), 1, (jbyte *) buf);
    vmc->pc_off(2);
}

void ST_CH_Aput_Char::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();      /* AA: source value */
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-char", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lca = (jcharArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lca)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lca);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    LOG_D("+ APUT[%u]=%u",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegister(vmc->tmp->dst));
    u8 buf[1];
    *(jchar *) buf = vmc->getRegister(vmc->tmp->dst);
    (*VM_CONTEXT::env).SetCharArrayRegion(
            vmc->tmp->val_1.lca, vmc->getRegister(vmc->tmp->src2), 1, (jchar *) buf);
    vmc->pc_off(2);
}

void ST_CH_Aput_Short::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();      /* AA: source value */
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;    /* index */
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;  /* array ptr */
    LOG_D("aget%s v%u,v%u,v%u",
          "-short", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.lsa = (jshortArray) vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_1.lsa)) {
        return;
    }
    vmc->tmp->val_2.u4 = (*VM_CONTEXT::env).GetArrayLength(vmc->tmp->val_1.lsa);
    if (vmc->tmp->val_2.u4 <= vmc->getRegister(vmc->tmp->src2)) {
        JavaException::throwArrayIndexOutOfBoundsException(
                vmc, vmc->tmp->val_2.u4, vmc->getRegister(vmc->tmp->src2));
        return;
    }
    LOG_D("+ APUT[%u]=%u",
          vmc->getRegister(vmc->tmp->src2),
          vmc->getRegister(vmc->tmp->dst));
    u8 buf[1];
    *(jshort *) buf = vmc->getRegister(vmc->tmp->dst);
    (*VM_CONTEXT::env).SetShortArrayRegion(
            vmc->tmp->val_1.lsa, vmc->getRegister(vmc->tmp->src2), 1, (jshort *) buf);
    vmc->pc_off(2);
}

void ST_CH_Iget::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-normal", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterInt(vmc->tmp->dst, val.i);
    LOG_D("+ IGET '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterInt(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iget_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-wide", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterLong(vmc->tmp->dst, val.j);
    LOG_D("+ IGET '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iget_Object::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-object", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->tmp->dst, val.l);
    LOG_D("+ IGET '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iget_Boolean::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-bool", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.z);
    LOG_D("+ IGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iget_Byte::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-byte", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.b);
    LOG_D("+ IGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iget_Char::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-char", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.c);
    LOG_D("+ IGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iget_Short::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-short", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.s);
    LOG_D("+ IGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-normal", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.i = vmc->getRegisterInt(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterInt(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-wide", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.j = vmc->getRegisterLong(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Object::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-object", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.l = vmc->getRegisterAsObject(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Boolean::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-bool", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.z = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Byte::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-byte", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.b = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Char::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-char", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.c = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Short::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-short", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.s = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-normal", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterInt(vmc->tmp->dst, val.i);
    LOG_D("+ SGET '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterInt(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-wide", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterLong(vmc->tmp->dst, val.j);
    LOG_D("+ SGET '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Object::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-object", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->tmp->dst, val.l);
    LOG_D("+ SGET '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Boolean::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-boolean", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.z);
    LOG_D("+ SGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Byte::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-byte", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.b);
    LOG_D("+ SGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Char::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-char", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.c);
    LOG_D("+ SGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Short::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-short", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.s);
    LOG_D("+ SGET '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%u,sfield@%u",
          "-normal", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.i = vmc->getRegisterInt(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterInt(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Wide::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%u,sfield@%u",
          "-wide", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.j = vmc->getRegisterLong(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Object::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%u,sfield@%u",
          "-object", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.l = vmc->getRegisterAsObject(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Boolean::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%u,sfield@%u",
          "-boolean", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.z = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Byte::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%u,sfield@%u",
          "-byte", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.b = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Char::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%u,sfield@%u",
          "-char", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.c = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Short::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%u,sfield@%u",
          "-short", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.s = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%u",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegister(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Invoke_Virtual::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-virtual args=%u @%u {regs=0x%08x %x}",
          vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);

    vmc->setState(VmMethodContextState::Method);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Super::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-super args=%u @%u {regs=0x%08x %x}",
          vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);
    vmc->setState(VmMethodContextState::SuperMethod);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Direct::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-direct args=%u @%u {regs=0x%08x %x}",
          vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);
    vmc->setState(VmMethodContextState::Method);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Static::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-static args=%u @%u {regs=0x%08x %x}",
          vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);
    vmc->setState(VmMethodContextState::StaticMethod);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Interface::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-interface args=%u @%u {regs=0x%08x %x}",
          vmc->tmp->src1 >> 4u, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->src1 & 0x0fu);
    vmc->setState(VmMethodContextState::Method);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Virtual_Range::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-virtual-range args=%u @%u {regs=v%u-v%u}",
          vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);
    vmc->setState(VmMethodContextState::MethodRange);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Super_Range::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-super-range args=%u @%u {regs=v%u-v%u}",
          vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);
    vmc->setState(VmMethodContextState::SuperMethodRange);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Direct_Range::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-direct-range args=%u @%u {regs=v%u-v%u}",
          vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);
    vmc->setState(VmMethodContextState::MethodRange);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Static_Range::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-static-range args=%u @%u {regs=v%u-v%u}",
          vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);
    vmc->setState(VmMethodContextState::StaticMethodRange);
    // no vmc->pc_off(3);
}

void ST_CH_Invoke_Interface_Range::run(VmMethodContext *vmc) {
    vmc->tmp->src1 = vmc->inst_AA();         /* AA (count) or BA (count + arg 5) */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* method ref */
    vmc->tmp->dst = vmc->fetch(2);       /* 4 regs -or- first reg */
    LOG_D("|invoke-interface-range args=%u @%u {regs=v%u-v%u}",
          vmc->tmp->src1, vmc->tmp->val_1.u4, vmc->tmp->dst, vmc->tmp->dst + vmc->tmp->src1 - 1);
    vmc->setState(VmMethodContextState::MethodRange);
    // no vmc->pc_off(3);
}

void ST_CH_Neg_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "neg-int", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterInt(vmc->tmp->dst, -vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Not_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "not-int", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1) ^ 0xffffffff);
    vmc->pc_off(1);
}

void ST_CH_Neg_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "neg-long", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterLong(vmc->tmp->dst, -vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Not_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "not-long", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1) ^ 0xffffffffffffffffULL);
    vmc->pc_off(1);
}

void ST_CH_Neg_Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "neg-float", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst, -vmc->getRegisterFloat(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Neg_Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "neg-double", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst, -vmc->getRegisterDouble(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Int2Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "int-to-long", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterLong(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Int2Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "int-to-float", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Int2Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "int-to-double", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Long2Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "long-to-int", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterInt(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Long2Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "long-to-float", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Long2Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "long-to-double", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Float2Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "float-to-int", vmc->tmp->dst, vmc->tmp->src1);
    jint min = (jint) 1 << (sizeof(jint) * 8 - 1);      // get min
    jint max = ~min;                                    // get max
    vmc->tmp->val_1.f = vmc->getRegisterFloat(vmc->tmp->src1);
    if (vmc->tmp->val_1.f >= max) {                   /* +inf */
        vmc->tmp->val_2.i = max;
    } else if (vmc->tmp->val_1.f <= min) {            /* -inf */
        vmc->tmp->val_2.i = min;
    } else if (vmc->tmp->val_1.f != vmc->tmp->val_1.f) {    /* NaN */
        vmc->tmp->val_2.i = 0;
    } else {
        vmc->tmp->val_2.i = (jint) vmc->tmp->val_1.f;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);
}

void ST_CH_Float2Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "float-to-long", vmc->tmp->dst, vmc->tmp->src1);
    jlong min = (jlong) 1 << (sizeof(jlong) * 8 - 1);       // get min
    jlong max = ~min;                                       // get max
    vmc->tmp->val_1.f = vmc->getRegisterFloat(vmc->tmp->src1);
    if (vmc->tmp->val_1.f >= max) {                   /* +inf */
        vmc->tmp->val_2.j = max;
    } else if (vmc->tmp->val_1.f <= min) {            /* -inf */
        vmc->tmp->val_2.j = min;
    } else if (vmc->tmp->val_1.f != vmc->tmp->val_1.f) {    /* NaN */
        vmc->tmp->val_2.j = 0;
    } else {
        vmc->tmp->val_2.j = (jint) vmc->tmp->val_1.f;
    }
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);
}

void ST_CH_Float2Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "float-to-double", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst, vmc->getRegisterFloat(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Double2Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "double-to-int", vmc->tmp->dst, vmc->tmp->src1);
    jint min = (jint) 1 << (sizeof(jint) * 8 - 1);      // get min
    jint max = ~min;                                    // get max
    vmc->tmp->val_1.d = vmc->getRegisterDouble(vmc->tmp->src1);
    if (vmc->tmp->val_1.d >= max) {                   /* +inf */
        vmc->tmp->val_2.i = max;
    } else if (vmc->tmp->val_1.d <= min) {            /* -inf */
        vmc->tmp->val_2.i = min;
    } else if (vmc->tmp->val_1.d != vmc->tmp->val_1.d) {    /* NaN */
        vmc->tmp->val_2.i = 0;
    } else {
        vmc->tmp->val_2.i = (jint) vmc->tmp->val_1.d;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);
}

void ST_CH_Double2Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "double-to-long", vmc->tmp->dst, vmc->tmp->src1);
    jlong min = (jlong) 1 << (sizeof(jlong) * 8 - 1);       // get min
    jlong max = ~min;                                       // get max
    vmc->tmp->val_1.d = vmc->getRegisterDouble(vmc->tmp->src1);
    if (vmc->tmp->val_1.d >= max) {                   /* +inf */
        vmc->tmp->val_2.j = max;
    } else if (vmc->tmp->val_1.d <= min) {            /* -inf */
        vmc->tmp->val_2.j = min;
    } else if (vmc->tmp->val_1.d != vmc->tmp->val_1.d) {    /* NaN */
        vmc->tmp->val_2.j = 0;
    } else {
        vmc->tmp->val_2.j = (jint) vmc->tmp->val_1.d;
    }
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);
}

void ST_CH_Double2Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s v%u,v%u", "double-to-float", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst, vmc->getRegisterDouble(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Int2Byte::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|int-to-%s v%u,v%u", "byte", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst, (s1) vmc->getRegister(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Int2Char::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|int-to-%s v%u,v%u", "char", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst, (u2) vmc->getRegister(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Int2Short::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|int-to-%s v%u,v%u", "short", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst, (s2) vmc->getRegister(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Add_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                        + vmc->getRegisterInt(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Sub_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                        - vmc->getRegisterInt(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Mul_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                        * vmc->getRegisterInt(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Div_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.i = vmc->getRegisterInt(vmc->tmp->src1);
    vmc->tmp->val_2.i = vmc->getRegisterInt(vmc->tmp->src2);
    if (vmc->tmp->val_2.i == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.i == -1) {
        vmc->tmp->val_2.i = vmc->tmp->val_1.i;
    } else {
        vmc->tmp->val_2.i = vmc->tmp->val_1.i / vmc->tmp->val_2.i;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);
    vmc->pc_off(2);
}

void ST_CH_Rem_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.i = vmc->getRegisterInt(vmc->tmp->src1);
    vmc->tmp->val_2.i = vmc->getRegisterInt(vmc->tmp->src2);
    if (vmc->tmp->val_2.i == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.i == -1) {
        vmc->tmp->val_2.i = 0;
    } else {
        vmc->tmp->val_2.i = vmc->tmp->val_1.i % vmc->tmp->val_2.i;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);
    vmc->pc_off(2);
}

void ST_CH_And_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "and", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     & vmc->getRegister(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Or_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "or", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     | vmc->getRegister(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Xor_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "xor", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     ^ vmc->getRegister(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Shl_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "shl", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                                << (vmc->getRegister(vmc->tmp->src2) & 0x1fu));
    vmc->pc_off(2);
}

void ST_CH_Shr_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "shr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                                >> (vmc->getRegister(vmc->tmp->src2) & 0x1fu));
    vmc->pc_off(2);
}

void ST_CH_Ushr_Int::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-int v%u,v%u,v%u", "ushr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                             >> (vmc->getRegister(vmc->tmp->src2) & 0x1fu));
    vmc->pc_off(2);
}

void ST_CH_Add_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->src1)
                         + vmc->getRegisterLong(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Sub_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->src1)
                         - vmc->getRegisterLong(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Mul_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->src1)
                         * vmc->getRegisterLong(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Div_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.j = vmc->getRegisterLong(vmc->tmp->src1);
    vmc->tmp->val_2.j = vmc->getRegisterLong(vmc->tmp->src2);
    if (vmc->tmp->val_2.j == 0LL) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u8 == 0x8000000000000000ULL && vmc->tmp->val_2.j == -1LL) {
        vmc->tmp->val_2.j = vmc->tmp->val_1.j;
    } else {
        vmc->tmp->val_2.j = vmc->tmp->val_1.j / vmc->tmp->val_2.j;
    }
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);
    vmc->pc_off(2);
}

void ST_CH_Rem_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->tmp->val_1.j = vmc->getRegisterLong(vmc->tmp->src1);
    vmc->tmp->val_2.j = vmc->getRegisterLong(vmc->tmp->src2);
    if (vmc->tmp->val_2.j == 0LL) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u8 == 0x8000000000000000ULL && vmc->tmp->val_2.j == -1LL) {
        vmc->tmp->val_2.j = 0LL;
    } else {
        vmc->tmp->val_2.j = vmc->tmp->val_1.j % vmc->tmp->val_2.j;
    }
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);
    vmc->pc_off(2);
}

void ST_CH_And_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "and", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterWide(vmc->tmp->src1)
                         & vmc->getRegisterWide(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Or_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "or", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterWide(vmc->tmp->dst,
                         vmc->getRegisterWide(vmc->tmp->src1)
                         | vmc->getRegisterWide(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Xor_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "xor", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterWide(vmc->tmp->dst,
                         vmc->getRegisterWide(vmc->tmp->src1)
                         ^ vmc->getRegisterWide(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Shl_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "shl", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->src1)
                                 << (vmc->getRegister(vmc->tmp->src2) & 0x3fu));
    vmc->pc_off(2);
}

void ST_CH_Shr_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "shr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->src1)
                                 >> (vmc->getRegister(vmc->tmp->src2) & 0x3fu));
    vmc->pc_off(2);
}

void ST_CH_Ushr_Long::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-long v%u,v%u,v%u", "ushr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterWide(vmc->tmp->dst,
                         vmc->getRegisterWide(vmc->tmp->src1)
                                 >> (vmc->getRegister(vmc->tmp->src2) & 0x3fu));
    vmc->pc_off(2);
}

void ST_CH_Add_Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          vmc->getRegisterFloat(vmc->tmp->src1)
                          + vmc->getRegisterFloat(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Sub_Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          vmc->getRegisterFloat(vmc->tmp->src1)
                          - vmc->getRegisterFloat(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Mul_Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          vmc->getRegisterFloat(vmc->tmp->src1)
                          * vmc->getRegisterFloat(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Div_Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          vmc->getRegisterFloat(vmc->tmp->src1)
                          / vmc->getRegisterFloat(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Rem_Float::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-float v%u,v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          std::fmodf(vmc->getRegisterFloat(vmc->tmp->src1),
                                     vmc->getRegisterFloat(vmc->tmp->src2)));
    vmc->pc_off(2);
}

void ST_CH_Add_Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           vmc->getRegisterDouble(vmc->tmp->src1)
                           + vmc->getRegisterDouble(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Sub_Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           vmc->getRegisterDouble(vmc->tmp->src1)
                           - vmc->getRegisterDouble(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Mul_Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           vmc->getRegisterDouble(vmc->tmp->src1)
                           * vmc->getRegisterDouble(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Div_Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           vmc->getRegisterDouble(vmc->tmp->src1)
                           / vmc->getRegisterDouble(vmc->tmp->src2));
    vmc->pc_off(2);
}

void ST_CH_Rem_Double::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->src2 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("|%s-double v%u,v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->src2);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           std::fmod(vmc->getRegisterDouble(vmc->tmp->src1),
                                     vmc->getRegisterDouble(vmc->tmp->src2)));
    vmc->pc_off(2);
}

void ST_CH_Add_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->dst)
                        + vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Sub_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->dst)
                        - vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Mul_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->dst)
                        * vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Div_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1);
    vmc->tmp->val_1.i = vmc->getRegisterInt(vmc->tmp->dst);
    vmc->tmp->val_2.i = vmc->getRegisterInt(vmc->tmp->src1);
    if (vmc->tmp->val_2.i == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.i == -1) {
        vmc->tmp->val_2.i = vmc->tmp->val_1.i;
    } else {
        vmc->tmp->val_2.i = vmc->tmp->val_1.i / vmc->tmp->val_2.i;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);
    vmc->pc_off(2);
}

void ST_CH_Rem_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1);
    vmc->tmp->val_1.i = vmc->getRegisterInt(vmc->tmp->dst);
    vmc->tmp->val_2.i = vmc->getRegisterInt(vmc->tmp->src1);
    if (vmc->tmp->val_2.i == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.i == -1) {
        vmc->tmp->val_2.i = 0;
    } else {
        vmc->tmp->val_2.i = vmc->tmp->val_1.i % vmc->tmp->val_2.i;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.i);
    vmc->pc_off(2);
}

void ST_CH_And_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "and", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->dst)
                     & vmc->getRegister(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Or_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "or", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->dst)
                     * vmc->getRegister(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Xor_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "xor", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->dst)
                     ^ vmc->getRegister(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Shl_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "shl", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->dst)
                                << (vmc->getRegister(vmc->tmp->src1) & 0x1fu));
    vmc->pc_off(1);
}

void ST_CH_Shr_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "shr", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->dst)
                                >> (vmc->getRegister(vmc->tmp->src1) & 0x1fu));
    vmc->pc_off(1);
}

void ST_CH_Ushr_Int_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-int-2addr v%u,v%u", "ushr", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->dst)
                             >> (vmc->getRegister(vmc->tmp->src1) & 0x1fu));
    vmc->pc_off(1);
}

void ST_CH_Add_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->dst)
                         + vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Sub_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->dst)
                         - vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Mul_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->dst)
                         * vmc->getRegisterLong(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Div_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1);
    vmc->tmp->val_1.j = vmc->getRegisterLong(vmc->tmp->dst);
    vmc->tmp->val_2.j = vmc->getRegisterLong(vmc->tmp->src1);
    if (vmc->tmp->val_2.j == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u8 == 0x8000000000000000ULL && vmc->tmp->val_2.j == -1LL) {
        vmc->tmp->val_2.j = vmc->tmp->val_1.j;
    } else {
        vmc->tmp->val_2.j = vmc->tmp->val_1.j / vmc->tmp->val_2.j;
    }
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);
    vmc->pc_off(2);
}

void ST_CH_Rem_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1);
    vmc->tmp->val_1.j = vmc->getRegisterLong(vmc->tmp->dst);
    vmc->tmp->val_2.j = vmc->getRegisterLong(vmc->tmp->src1);
    if (vmc->tmp->val_2.j == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u8 == 0x8000000000000000ULL && vmc->tmp->val_2.j == -1LL) {
        vmc->tmp->val_2.j = 0LL;
    } else {
        vmc->tmp->val_2.j = vmc->tmp->val_1.j % vmc->tmp->val_2.j;
    }
    vmc->setRegisterLong(vmc->tmp->dst, vmc->tmp->val_2.j);
    vmc->pc_off(2);
}

void ST_CH_And_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "and", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterWide(vmc->tmp->dst,
                         vmc->getRegisterWide(vmc->tmp->dst)
                         & vmc->getRegisterWide(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Or_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "or", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterWide(vmc->tmp->dst,
                         vmc->getRegisterWide(vmc->tmp->dst)
                         | vmc->getRegisterWide(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Xor_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "xor", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterWide(vmc->tmp->dst,
                         vmc->getRegisterWide(vmc->tmp->dst)
                         ^ vmc->getRegisterWide(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Shl_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "shl", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->dst)
                                 << (vmc->getRegister(vmc->tmp->src1) & 0x3fu));
    vmc->pc_off(1);
}

void ST_CH_Shr_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "shr", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterLong(vmc->tmp->dst,
                         vmc->getRegisterLong(vmc->tmp->dst)
                                 << (vmc->getRegister(vmc->tmp->src1) & 0x3fu));
    vmc->pc_off(1);
}

void ST_CH_Ushr_Long_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-long-2addr v%u,v%u", "ushr", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterWide(vmc->tmp->dst,
                         vmc->getRegisterWide(vmc->tmp->dst)
                                 << (vmc->getRegister(vmc->tmp->src1) & 0x3fu));
    vmc->pc_off(1);
}

void ST_CH_Add_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          vmc->getRegisterFloat(vmc->tmp->dst)
                          + vmc->getRegisterFloat(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Sub_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          vmc->getRegisterFloat(vmc->tmp->dst)
                          - vmc->getRegisterFloat(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Mul_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          vmc->getRegisterFloat(vmc->tmp->dst)
                          * vmc->getRegisterFloat(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Div_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          vmc->getRegisterFloat(vmc->tmp->dst)
                          / vmc->getRegisterFloat(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Rem_Float_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-float-2addr v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterFloat(vmc->tmp->dst,
                          std::fmodf(vmc->getRegisterFloat(vmc->tmp->dst),
                                     vmc->getRegisterFloat(vmc->tmp->src1)));
    vmc->pc_off(1);
}

void ST_CH_Add_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%u,v%u", "add", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           vmc->getRegisterDouble(vmc->tmp->dst)
                           + vmc->getRegisterDouble(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Sub_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%u,v%u", "sub", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           vmc->getRegisterDouble(vmc->tmp->dst)
                           - vmc->getRegisterDouble(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Mul_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%u,v%u", "mul", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           vmc->getRegisterDouble(vmc->tmp->dst)
                           * vmc->getRegisterDouble(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Div_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%u,v%u", "div", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           vmc->getRegisterDouble(vmc->tmp->dst)
                           / vmc->getRegisterDouble(vmc->tmp->src1));
    vmc->pc_off(1);
}

void ST_CH_Rem_Double_2Addr::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    LOG_D("|%s-double-2addr v%u,v%u", "rem", vmc->tmp->dst, vmc->tmp->src1);
    vmc->setRegisterDouble(vmc->tmp->dst,
                           std::fmod(vmc->getRegisterDouble(vmc->tmp->dst),
                                     vmc->getRegisterDouble(vmc->tmp->src1)));
    vmc->pc_off(1);
}

void ST_CH_Add_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%u,v%u,#%d",
          "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                        + vmc->tmp->val_1.s2);
    vmc->pc_off(2);
}

void ST_CH_RSub_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%u,v%u,#%d",
          "rsub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->tmp->val_1.s2
                        - vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(2);
}

void ST_CH_Mul_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%u,v%u,#%d",
          "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                        * vmc->tmp->val_1.s2);
    vmc->pc_off(2);
}

void ST_CH_Div_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_2.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%u,v%u,#%d",
          "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.s2);
    vmc->tmp->val_1.s4 = vmc->getRegisterInt(vmc->tmp->src1);
    if (vmc->tmp->val_2.s2 == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.s2 != -1) {
        vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4;
    } else {
        vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4 / vmc->tmp->val_2.s4;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.s4);
    vmc->pc_off(2);
}

void ST_CH_Rem_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_2.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%u,v%u,#%d",
          "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.s2);
    vmc->tmp->val_1.s4 = vmc->getRegisterInt(vmc->tmp->src1);
    if (vmc->tmp->val_2.s2 == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.s2 != -1) {
        vmc->tmp->val_2.s4 = 0;
    } else {
        vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4 % vmc->tmp->val_2.s4;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.s4);
    vmc->pc_off(2);
}

void ST_CH_And_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%u,v%u,#%d",
          "and", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     & vmc->tmp->val_1.u2);
    vmc->pc_off(2);
}

void ST_CH_Or_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%u,v%u,#%d",
          "or", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     | vmc->tmp->val_1.u2);
    vmc->pc_off(2);
}

void ST_CH_Xor_Int_Lit16::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();
    vmc->tmp->val_1.u2 = vmc->fetch(1);
    LOG_D("|%s-int/lit16 v%u,v%u,#%d",
          "xor", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s2);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     ^ vmc->tmp->val_1.u2);
    vmc->pc_off(2);
}

void ST_CH_Add_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                        + vmc->tmp->val_1.s1);
    vmc->pc_off(2);
}

void ST_CH_RSub_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "rsub", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->tmp->val_1.s1
                        - vmc->getRegisterInt(vmc->tmp->src1));
    vmc->pc_off(2);
}

void ST_CH_Mul_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "mul", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                        * vmc->tmp->val_1.s1);
    vmc->pc_off(2);
}

void ST_CH_Div_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_2.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "div", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.s1);
    vmc->tmp->val_1.s4 = vmc->getRegisterInt(vmc->tmp->src1);
    if (vmc->tmp->val_2.s1 == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.s1 != -1) {
        vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4;
    } else {
        vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4 / vmc->tmp->val_2.s4;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.s4);
    vmc->pc_off(2);
}

void ST_CH_Rem_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_2.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "rem", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_2.s1);
    vmc->tmp->val_1.s4 = vmc->getRegisterInt(vmc->tmp->src1);
    if (vmc->tmp->val_2.s1 == 0) {
        JavaException::throwArithmeticException(vmc, "divide by zero");
        return;
    }
    if (vmc->tmp->val_1.u4 == 0x80000000u && vmc->tmp->val_2.s1 != -1) {
        vmc->tmp->val_2.s4 = 0;
    } else {
        vmc->tmp->val_2.s4 = vmc->tmp->val_1.s4 % vmc->tmp->val_2.s4;
    }
    vmc->setRegisterInt(vmc->tmp->dst, vmc->tmp->val_2.s4);
    vmc->pc_off(2);
}

void ST_CH_And_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "and", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     & vmc->tmp->val_1.u1);
    vmc->pc_off(2);
}

void ST_CH_Or_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "or", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     | vmc->tmp->val_1.u1);
    vmc->pc_off(2);
}

void ST_CH_Xor_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "add", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                     ^ vmc->tmp->val_1.u1);
    vmc->pc_off(2);
}

void ST_CH_Shl_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "shl", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                                << (vmc->tmp->val_1.u1 & 0x1fu));
    vmc->pc_off(2);
}

void ST_CH_Shr_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "shr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegisterInt(vmc->tmp->dst,
                        vmc->getRegisterInt(vmc->tmp->src1)
                                >> (vmc->tmp->val_1.u1 & 0x1fu));
    vmc->pc_off(2);
}

void ST_CH_Ushr_Int_Lit8::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->src1 = vmc->fetch(1);
    vmc->tmp->val_1.u1 = vmc->tmp->src1 >> 8u;
    vmc->tmp->src1 = vmc->tmp->src1 & 0xffu;
    LOG_D("%s-int/lit8 v%u,v%u,#%d",
          "ushr", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.s1);
    vmc->setRegister(vmc->tmp->dst,
                     vmc->getRegister(vmc->tmp->src1)
                             >> (vmc->tmp->val_1.u1 & 0x1fu));
    vmc->pc_off(2);
}

void ST_CH_Iget_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-normal-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterInt(vmc->tmp->dst, val.i);
    LOG_D("+ IGET '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-normal-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.i = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterInt(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-normal-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegister(vmc->tmp->dst, val.i);
    LOG_D("+ SGET '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterInt(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("sput%s v%u,sfield@%u",
          "-normal-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.i = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%d",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterInt(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iget_Object_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-object-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->tmp->dst, val.l);
    LOG_D("+ IGET '%s'=0x%08lx",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          (u8) vmc->getRegisterWide(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iget_Wide_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iget%s v%u,v%u,field@%u",
          "-wide-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterWide(vmc->tmp->dst, val.j);
    LOG_D("+ IGET '%s'=%ldx",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Wide_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-wide-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.j = vmc->getRegisterLong(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Wide_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-wide-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterLong(vmc->tmp->dst, val.j);
    LOG_D("+ SGET '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Wide_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("|sput%s v%u,sfield@%u",
          "-wide-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.j = vmc->getRegisterLong(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%ld",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterLong(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Iput_Object_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_A();
    vmc->tmp->src1 = vmc->inst_B();          /* object ptr */
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|iput%s v%u,v%u,field@%u",
          "-object-volatile", vmc->tmp->dst, vmc->tmp->src1, vmc->tmp->val_1.u4);
    vmc->tmp->val_2.l = vmc->getRegisterAsObject(vmc->tmp->src1);
    if (!JavaException::checkForNull(vmc, vmc->tmp->val_2.l)) {
        return;
    }
    RegValue val{};
    val.l = vmc->getRegisterAsObject(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, vmc->tmp->val_2.l, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ IPUT '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sget_Object_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);   /* field ref */
    LOG_D("|sget%s v%u,sfield@%u",
          "-object-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    if (!vmc->method->resolveField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    vmc->setRegisterAsObject(vmc->tmp->dst, val.l);
    LOG_D("+ SGET '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->pc_off(2);
}

void ST_CH_Sput_Object_Volatile::run(VmMethodContext *vmc) {
    vmc->tmp->dst = vmc->inst_AA();
    vmc->tmp->val_1.u4 = vmc->fetch(1);    /* field ref */
    LOG_D("|sput%s v%u,sfield@%u",
          "-object-volatile", vmc->tmp->dst, vmc->tmp->val_1.u4);
    RegValue val{};
    val.i = vmc->getRegister(vmc->tmp->dst);
    if (!vmc->method->resolveSetField(vmc->tmp->val_1.u4, nullptr, &val)) {
        JavaException::throwJavaException(vmc);
        return;
    }
    LOG_D("+ SPUT '%s'=%p",
          vmc->method->resolveFieldName(vmc->tmp->val_1.u4),
          vmc->getRegisterAsObject(vmc->tmp->dst));
    vmc->pc_off(2);
}
