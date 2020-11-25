//
// Created by 陈泽伦 on 11/17/20.
//

#ifndef VM_VM_H
#define VM_VM_H

#include <jni.h>
#include <map>
#include <string>
#include "VmMethod.h"

#define kPackedSwitchSignature  0x0100
#define kSparseSwitchSignature  0x0200
#define kArrayDataSignature     0x0300

class CodeHandler {
public:
    virtual void run(VmMethodContext *vmc) = 0;

    static void filledNewArray(VmMethodContext *vmc, bool range);

    static s4 handlePackedSwitch(const u2 *switchData, s4 testVal);

    static s4 handleSparseSwitch(const u2 *switchData, s4 testVal);
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

#define GET_REGISTER(off)               (vmc->reg[off].u4)
#define SET_REGISTER(off, val)          (vmc->reg[off].u4 = (uint32_t)(val))

#define GET_REGISTER_INT(off)           (vmc->reg[off].i)

#define GET_REGISTER_WIDE(off)          (vmc->reg[off].u8)
#define SET_REGISTER_WIDE(off, val)     (vmc->reg[off].u8 = (uint64_t)(val))

#define GET_REGISTER_AS_OBJECT(off)          (vmc->reg[off].l)
#define SET_REGISTER_AS_OBJECT(off, val)     (vmc->reg[off].l = (jobject)(val))

// Standard Interpret's code handler

class CH_NOP : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Move : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Move_From16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Wide_From16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Wide16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Object_From16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Object16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Result : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Result_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Result_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Move_Exception : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Return_Void : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Return : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Return_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Return_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const4 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const_High16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const_Wide16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const_Wide32 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const_Wide_High16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const_String : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const_String_Jumbo : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Const_Class : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Monitor_Enter : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Monitor_Exit : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Check_Cast : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Instance_Of : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Array_Length : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_New_Instance : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_New_Array : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Filled_New_Array : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Filled_New_Array_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Fill_Array_Data : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Throw : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Goto : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Goto16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Goto32 : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Packed_Switch : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class CH_Sparse_Switch : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_CMPL_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_CMPG_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_CMPL_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_CMPG_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_CMP_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_EQ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_NE : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_LT : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_LE : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_GT : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_GE : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_EQZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_NEZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_LTZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_GEZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_GTZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_LEZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aget : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aget_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aget_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aget_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aget_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aget_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aget_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aput : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aput_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aput_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aput_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aput_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Aput_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class OP_Iget : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class OP_Iget_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class OP_Iget_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class OP_Iget_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class OP_Iget_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class OP_Iget_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class OP_Iget_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_IF_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Virtual : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Super : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Direct : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Static : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Interface : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Virtual_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Super_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Direct_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Static_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Invoke_Interface_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Neg_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Not_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Neg_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Not_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Neg_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Neg_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Int2Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Int2Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Int2Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Long2Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Long2Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Long2Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Float2Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Float2Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Float2Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Double2Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Double2Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Double2Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Int2Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Int2Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Int2Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_And_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Or_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Xor_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shl_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shr_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Ushr_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_And_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Or_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Xor_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shl_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shr_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Ushr_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_And_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Or_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Xor_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shl_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shr_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Ushr_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_And_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Or_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Xor_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shl_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shr_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Ushr_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_And_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Or_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Xor_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Add_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sub_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Mul_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Div_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Rem_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_And_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Or_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Xor_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shl_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Shr_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Ushr_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};


class CH_Iget_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iget_Object_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iget_Wide_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput_Wide_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Wide_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Wide_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Throw_Verification_Error : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Iput_Object_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sget_Object_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class CH_Sput_Object_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class VMException : public std::runtime_error {
public:
    VMException(const char *exp) : runtime_error(std::string("VMException: ") + exp) {}

    VMException(const std::string &exp) : runtime_error(std::string("VMException: ") + exp) {}
};

class JAVAException {
public:
    static void throwJavaException(VmMethodContext *vmc);

    static void handleJavaException(VmMethodContext *vmc);

    static bool checkForNull(jobject obj);

    static void throwNullPointerException(const char *msg);

    static void throwClassCastException(jclass actual, jclass desired);

    static void throwNegativeArraySizeException(s4 size);

    static void throwRuntimeException(const char *msg);

    static void throwInternalError(const char *msg);

    static void throwArrayIndexOutOfBoundsException(u4 length, u4 index);

private:
    static void throwNew(const char *exceptionClassName, const char *msg);
};

#define  PRIMITIVE_TYPE_SIZE 9

class Vm {
public:
    Interpret *interpret;

public:
    static void
    callMethod(jobject instance, jmethodID method, jvalue *pResult, ...);

    Vm();

    void run(VmMethodContext *vmc);

    jclass findPrimitiveClass(const char type) const;


private:
    const char primitiveType[PRIMITIVE_TYPE_SIZE] = {'V', 'B', 'Z', 'I', 'S', 'C', 'F', 'D', 'J'};
    jclass primitiveClass[PRIMITIVE_TYPE_SIZE]{};
private:
    void initPrimitiveClass();

};


#endif //VM_VM_H
