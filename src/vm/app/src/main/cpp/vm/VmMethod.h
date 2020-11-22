//
// Created by 陈泽伦 on 11/16/20.
//

#ifndef VM_VMMETHOD_H
#define VM_VMMETHOD_H


#include <jni.h>
#include <vector>
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

class VmRegister {
private:
    uint64_t *reg;
    uint32_t len;

public:
    VmRegister(uint32_t len);

    ~VmRegister() {
        free(reg);
    }

    template<typename T>
    void write(int idx, T data);

    template<typename T>
    T read(int idx);

};


class VmMethodContext {
public:
    const VmMethod *method;
    jobject caller;
    const jvalue *result;
    VmRegister reg;

public:
    VmMethodContext(jobject caller, const VmMethod *method, const jvalue *pResult, va_list param);
};


#endif //VM_VMMETHOD_H
