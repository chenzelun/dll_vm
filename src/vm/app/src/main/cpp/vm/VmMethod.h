//
// Created by 陈泽伦 on 11/23/20.
//

#ifndef VM_VMMETHOD_H
#define VM_VMMETHOD_H


#include <jni.h>
#include "../common/AndroidSource.h"

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

    static bool isStaticMethod(const u4 accessFlags) {
        return (accessFlags & ACC_STATIC) != 0;
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

public:
    VmMethod(jmethodID jniMethod);

    void updateCode();
};

union RegValue{
    jboolean    z;
    jbyte       b;
    jchar       c;
    jshort      s;
    jint        i;
    jlong       j;
    jfloat      f;
    jdouble     d;
    jobject     l;
    uint64_t    u64;
    uint32_t    u32;
};

class VmMethodContext {
public:
    const VmMethod *method;
    jobject caller;
    const jvalue *result;
    RegValue *reg;
    uint32_t reg_len;

    uint16_t pc;
    uint16_t src1, src2, dst;
    jthrowable curException = nullptr;

public:
    VmMethodContext(jobject caller, const VmMethod *method, const jvalue *pResult, va_list param);

    // reader
    inline uint16_t fetch_op() const {
        return (this->method->code->insns[pc]) & 0xff;
    }

    inline uint16_t fetch(uint16_t off) const {
        return (this->method->code->insns[pc + off]) & 0xff;
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
};


#endif //VM_VMMETHOD_H
