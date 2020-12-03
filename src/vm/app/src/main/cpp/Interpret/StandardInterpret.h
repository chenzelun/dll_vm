//
// Created by 陈泽伦 on 12/3/20.
//

#ifndef VM_STANDARDINTERPRET_H
#define VM_STANDARDINTERPRET_H

#include "Interpret.h"

#define kPackedSwitchSignature  0x0100
#define kSparseSwitchSignature  0x0200
#define kArrayDataSignature     0x0300


class StandardInterpret : public Interpret{
public:
    StandardInterpret();

    void run(VmMethodContext *vmc);

    static void filledNewArray(VmMethodContext *vmc, bool range);

    static s4 handlePackedSwitch(const u2 *switchData, s4 testVal);

    static s4 handleSparseSwitch(const u2 *switchData, s4 testVal);

    static const jvalue *pushMethodParams(VmMethodContext *vmc, bool isStatic);

    static const jvalue *pushMethodParamsRange(VmMethodContext *vmc, bool isStatic);

    static void invokeMethod(VmMethodContext *vmc, const jvalue *params);

    static void invokeSuperMethod(VmMethodContext *vmc, const jvalue *params);

    static void invokeStaticMethod(VmMethodContext *vmc, const jvalue *params);

#ifdef VM_DEBUG

    static void debugInvokeMethod(VmMethodContext *vmc,
                                  const char *shorty,
                                  const jvalue retVal,
                                  const jvalue *params);

#endif
};



class ST_CH_NOP : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_From16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Wide_From16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Wide16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Object_From16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Object16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Result : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Result_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Result_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Move_Exception : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Return_Void : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Return : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Return_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Return_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const4 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const_High16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const_Wide16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const_Wide32 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const_Wide_High16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const_String : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const_String_Jumbo : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Const_Class : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Monitor_Enter : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Monitor_Exit : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Check_Cast : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Instance_Of : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Array_Length : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_New_Instance : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_New_Array : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Filled_New_Array : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Filled_New_Array_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Fill_Array_Data : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Throw : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Goto : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Goto16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Goto32 : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Packed_Switch : public CodeHandler {
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sparse_Switch : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_CMPL_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_CMPG_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_CMPL_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_CMPG_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_CMP_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_EQ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_NE : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_LT : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_LE : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_GT : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_GE : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_EQZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_NEZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_LTZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_GEZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_GTZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_IF_LEZ : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aget : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aget_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aget_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aget_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aget_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aget_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aget_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aput : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aput_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aput_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aput_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aput_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aput_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Aput_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Wide : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Object : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Boolean : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Virtual : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Super : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Direct : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Static : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Interface : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Virtual_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Super_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Direct_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Static_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Invoke_Interface_Range : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Neg_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Not_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Neg_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Not_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Neg_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Neg_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Int2Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Int2Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Int2Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Long2Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Long2Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Long2Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Float2Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Float2Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Float2Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Double2Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Double2Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Double2Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Int2Byte : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Int2Char : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Int2Short : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sub_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_And_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Or_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Xor_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shl_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shr_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Ushr_Int : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sub_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_And_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Or_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Xor_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shl_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shr_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Ushr_Long : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sub_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Float : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sub_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Double : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sub_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_And_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Or_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Xor_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shl_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shr_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Ushr_Int_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sub_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_And_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Or_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Xor_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shl_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shr_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Ushr_Long_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sub_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Float_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sub_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Double_2Addr : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_RSub_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_And_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Or_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Xor_Int_Lit16 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Add_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_RSub_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Mul_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Div_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Rem_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_And_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Or_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Xor_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shl_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Shr_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Ushr_Int_Lit8 : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};


class ST_CH_Iget_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget_Object_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iget_Wide_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Wide_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Wide_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Wide_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Iput_Object_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sget_Object_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};

class ST_CH_Sput_Object_Volatile : public CodeHandler {
public:
    void run(VmMethodContext *vmc) override;
};


#endif //VM_STANDARDINTERPRET_H
