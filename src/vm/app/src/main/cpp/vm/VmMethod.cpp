//
// Created by 陈泽伦 on 11/23/20.
//

#include "VmMethod.h"
#include "Vm.h"
#include "../common/Util.h"

DexFile::DexFile(const u1 *base) {
    this->base = base;
    this->pHeader = (const DexHeader *) base;
    this->pStringIds = (const DexStringId *) (base + this->pHeader->stringIdsOff);
    this->pTypeIds = (const DexTypeId *) (base + this->pHeader->typeIdsOff);
    this->pProtoIds = (const DexProtoId *) (base + this->pHeader->protoIdsOff);
    this->pFieldIds = (const DexFieldId *) (base + this->pHeader->fieldIdsOff);
    this->pMethodIds = (const DexMethodId *) (base + this->pHeader->methodIdsOff);
    this->pClassDefs = (const DexClassDef *) (base + this->pHeader->classDefsOff);
}

VmMethod::VmMethod(jmethodID jniMethod) {
    void *artMethod = jniMethod;
    auto *artClass = (ArtClass *) (uint64_t) ((ArtMethod_26_28 *) artMethod)->declaring_class;
    void *artDexCache = (void *) (uint64_t) artClass->dex_cache;
    auto *artDexFile = (ArtDexFile *) ((ArtDexCache_26_28 *) artDexCache)->dex_file;
    this->dexFile = new DexFile(artDexFile->begin);
    const DexMethodId *pDexMethodId =
            this->dexFile->dexGetMethodId(((ArtMethod_26_28 *) artMethod)->dex_method_index);
    this->method_id = ((ArtMethod_26_28 *) artMethod)->dex_method_index;
    this->protoId = this->dexFile->dexGetProtoId(pDexMethodId->protoIdx);
    this->accessFlags = ((ArtMethod_26_28 *) artMethod)->access_flags;
    this->clazzDescriptor = this->dexFile->dexStringByTypeIdx(pDexMethodId->classIdx);
    this->name = this->dexFile->dexStringById(pDexMethodId->nameIdx);
    this->code = nullptr;
}

void VmMethod::updateCode() {
    this->code = (CodeItemData *) VM_CONTEXT::vmKFCFile->getCode(this->method_id);
    this->code->triesAndHandlersBuf = (u1 *) (this->code->insns + this->code->insnsSize);
    this->code->triesAndHandlersBuf += this->code->insnsSize & (uint32_t) 0x01 ? 0x02 : 0x00;
}

VmMethodContext::VmMethodContext(jobject caller, const VmMethod *method,
                                 const jvalue *pResult, va_list param) {
    this->method = method;
    this->caller = caller;
    this->result = pResult;
    this->reg_len = method->code->registersSize;
    this->reg = (RegValue *) malloc(sizeof(RegValue) * this->reg_len);
    this->pc = 0;

    // 参数入寄存器
    // [0] is return value.
    const char *desc = method->dexFile->dexStringById(method->protoId->shortyIdx) + 1;
    int verifyCount = 0;
    int startReg = method->code->registersSize - method->code->insSize;

    // push this
    if (!DexFile::isStaticMethod(method->accessFlags)) {
        this->reg[startReg].l = caller;
        startReg++;
        verifyCount++;
    }

    // push param
    while (*desc != '\0') {
        switch (*desc++) {
            case 'D':
            case 'J': {
                this->reg[startReg].j = va_arg(param, jlong);
                startReg += 2;
                verifyCount += 2;
                break;
            }

            case 'L': {     /* 'shorty' descr uses L for all refs, incl array */
                this->reg[startReg].l = va_arg(param, jobject);
                startReg++;
                verifyCount++;
                break;
            }
            default: {
                /* Z B C S I F -- all passed as 32-bit integers */
                this->reg[startReg].i = va_arg(param, jint);
                startReg++;
                verifyCount++;
                break;
            }
        }
    }

    if (verifyCount != method->code->insSize) {
        LOG_E("Got vfy count=%d insSize=%d for %s", verifyCount, method->code->insSize,
              method->name);
        throw VMException("error param, and can't push them to vm reg.");
    }

}
