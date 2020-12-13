//
// Created by 陈泽伦 on 11/23/20.
//

#ifndef VM_VMMETHOD_H
#define VM_VMMETHOD_H


#include <jni.h>
#include "../../common/Util.h"
#include "../../common/AndroidSystem.h"
#include "VmCommon.h"

class DexFile {
public:
    const DexHeader *pHeader;
    const DexStringId *pStringIds;
    const DexTypeId *pTypeIds;
    const DexFieldId *pFieldIds;
    const DexMethodId *pMethodIds;
    const DexProtoId *pProtoIds;
    const DexClassDef *pClassDefs;

    /* points to start of DEX file data */
    const u1 *base;

public:
    DexFile(const u1 *base);

    /* return the StringId with the specified index */
    inline const DexStringId *dexGetStringId(u4 idx) {
        assert(idx < this->pHeader->stringIdsSize);
        return &this->pStringIds[idx];
    }

    /* return the const char* string data referred to by the given string_id */
    inline const char *dexGetStringData(const DexStringId *pStringId) {
        const u1 *ptr = this->base + pStringId->stringDataOff;

        // Skip the uleb128 length.
        while (*(ptr++) > 0x7f) /* empty */ ;

        return (const char *) ptr;
    }

    /* return the UTF-8 encoded string with the specified string_id index */
    inline const char *dexStringById(u4 idx) {
        const DexStringId *pStringId = this->dexGetStringId(idx);
        return this->dexGetStringData(pStringId);
    }

    /*
     * Get the descriptor string associated with a given type index.
     * The caller should not free() the returned string.
     */
    inline const char *dexStringByTypeIdx(u4 idx) {
        const DexTypeId *typeId = this->dexGetTypeId(idx);
        return this->dexStringById(typeId->descriptorIdx);
    }

    /* return the TypeId with the specified index */
    inline const DexTypeId *dexGetTypeId(u4 idx) {
        assert(idx < this->pHeader->typeIdsSize);
        return &this->pTypeIds[idx];
    }

    /* return the ProtoId with the specified index */
    inline const DexProtoId *dexGetProtoId(u4 idx) {
        assert(idx < this->pHeader->protoIdsSize);
        return &this->pProtoIds[idx];
    }

    /* return the MethodId with the specified index */
    inline const DexMethodId *dexGetMethodId(u4 idx) {
        assert(idx < this->pHeader->methodIdsSize);
        return &this->pMethodIds[idx];
    }

    /* return the FieldId with the specified index */
    inline const DexFieldId *dexGetFieldId(u4 idx) {
        assert(idx < this->pHeader->fieldIdsSize);
        return &this->pFieldIds[idx];
    }

    static bool isStaticMethod(const u4 accessFlags) {
        return (accessFlags & ACC_STATIC) != 0;
    }

    inline const char *dexGetMethodShorty(u4 idx) {
        assert(idx < this->pHeader->methodIdsSize);
        return this->dexStringById(
                this->dexGetProtoId(
                        this->dexGetMethodId(idx)->protoIdx)->shortyIdx);
    }

    /*
     * Get the parameter list from a ProtoId. The returns NULL if the ProtoId
     * does not have a parameter list.
     */
    inline const DexTypeList *dexGetProtoParameters(u4 off) {
        assert(off);
        return (const DexTypeList *) (this->base + off);
    }
};

class VmMethod {
public:
    uint32_t method_id;
    DexFile *dexFile;
    const char *name;
    const char *clazzDescriptor;
    const DexProtoId *protoId;
    u4 accessFlags;
    CodeItemData *code;
    u1 *triesAndHandlersBuf;

public:
    VmMethod(jmethodID jniMethod, bool isUpdateCode = true);

    jstring resolveString(u4 idx) const;

    jclass resolveClass(u4 idx) const;

    jmethodID resolveMethod(u4 idx, bool isStatic) const;

    std::string resolveMethodSign(u4 idx) const;

    bool resolveField(u4 idx, jobject obj, RegValue *retVal) const;

    const char *resolveFieldName(u4 idx) const;

    bool resolveSetField(u4 idx, jobject obj, const RegValue *val) const;

    jarray allocArray(const s4 len, u4 idx) const;

    static std::string getClassDescriptorByJClass(jclass clazz);
};

enum VmMethodContextState {
    Running,
    Return,
    MethodToCall_Start,
    Method,
    SuperMethod,
    StaticMethod,
    Range,
    MethodRange,
    SuperMethodRange,
    StaticMethodRange,
    MethodToCall_End,
    JniMethodToCall,
};

class VmMethodContext {
public:
    const VmMethod *method;
    RegValue *reg;
    VmTempData *tmp;

    jvalue *retVal;
    jthrowable curException = nullptr;

private:
    VmMethodContextState state;
    uint16_t pc;

public:
    inline void setState(VmMethodContextState contextState){
        this->state = contextState;
    }

    inline void run() {
        this->state = VmMethodContextState::Running;
    }

    inline void callMethodByJni() {
        this->state = VmMethodContextState::JniMethodToCall;
    }

    inline bool isCallStaticMethod()const {
        return this->state == VmMethodContextState::StaticMethod ||
               this->state == VmMethodContextState::StaticMethodRange;
    }

    inline bool isCallSuperMethod() const {
        return this->state == VmMethodContextState::SuperMethod ||
               this->state == VmMethodContextState::SuperMethodRange;
    }

    inline bool isCallMethodRange() const {
        return VmMethodContextState::Range < this->state &&
               this->state < VmMethodContextState::MethodToCall_End;
    }

    void reset(jobject caller, jmethodID methodId, jvalue *pResult, va_list param);

    void resetWithoutParams(jmethodID methodId, jvalue *pResult);

    void pushParams(jobject caller, va_list param) const;

    void release() const;

    inline bool isCallFromVm() {
        return this->isMethodToCall();
    }

    inline bool isMethodToCall() {
        return VmMethodContextState::MethodToCall_Start < this->state
               && this->state < VmMethodContextState::MethodToCall_End;
    }

#if defined(VM_DEBUG)

    void printVmMethodContext() const;

    void printMethodInsns() const;

#endif

    inline void finish() {
        assert(this->state != VmMethodContextState::Return);
        this->state = VmMethodContextState::Return;
    }

    inline bool isFinish() const {
        return this->state == VmMethodContextState::Return;
    }

    // reader
    inline uint16_t fetch_op() const {
        return (this->method->code->insns[pc]) & 0xff;
    }

    inline uint16_t fetch(uint16_t off) const {
        return (this->method->code->insns[pc + off]);
    }

    inline uint16_t inst_A() const {
        return ((this->method->code->insns[pc]) >> 8) & 0x0f;
    }

    inline uint16_t inst_B() const {
        return ((this->method->code->insns[pc]) >> 12) & 0x0f;
    }

    inline uint16_t inst_AA() const {
        return ((this->method->code->insns[pc]) >> 8);
    }

    inline void pc_off(uint32_t off) {
        this->pc += off;
    }

    inline u4 pc_cur() const {
        return this->pc;
    }

    inline void set_pc(uint32_t off) {
        assert(off < this->method->code->insnsSize);
        this->pc = off;
    }

    inline void goto_off(int32_t off){
        this->pc = this->pc + off;
        assert(0 <= this->pc && this->pc <= this->method->code->insnsSize);
    }

    inline const u2 *arrayData(s4 off) {
        s4 data_off = this->pc + off;
        assert(0 <= data_off && data_off <= this->method->code->insnsSize);
        return this->method->code->insns + data_off;
    }

    inline u4 getRegister(uint32_t off)const {
        return this->reg[off].u4;
    }

    inline void setRegister(uint32_t off, u4 val) {
        this->reg[off].u4 = val;
    }

    inline jint getRegisterInt(uint32_t off)const {
        return this->reg[off].i;
    }

    inline void setRegisterInt(uint32_t off, jint val) {
        this->reg[off].i = val;
    }

    inline u8 getRegisterWide(uint32_t off)const {
        return this->reg[off].u8;
    }

    inline void setRegisterWide(uint32_t off, u8 val) {
        this->reg[off].u8 = val;
    }

    inline jobject getRegisterAsObject(uint32_t off) const {
        return this->reg[off].l;
    }

    inline void setRegisterAsObject(uint32_t off, jobject val) {
        this->reg[off].l = val;
    }

    inline jfloat getRegisterFloat(uint32_t off) const {
        return this->reg[off].f;
    }

    inline void setRegisterFloat(uint32_t off, jfloat val) {
        this->reg[off].f = val;
    }

    inline jfloat getRegisterDouble(uint32_t off) const {
        return this->reg[off].f;
    }

    inline void setRegisterDouble(uint32_t off, jdouble val) {
        this->reg[off].d = val;
    }

    inline jlong getRegisterLong(uint32_t off) const {
        return this->reg[off].j;
    }

    inline void setRegisterLong(uint32_t off, jlong val) {
        this->reg[off].j = val;
    }
};

#endif //VM_VMMETHOD_H
