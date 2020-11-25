//
// Created by 陈泽伦 on 11/23/20.
//

#include "VmMethod.h"
#include "../common/VmConstant.h"
#include "../VmContext.h"

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

jstring VmMethod::resolveString(u4 idx) const {
    const char *data = this->dexFile->dexStringById(idx);
    LOG_D("+++ resolving string=%s, referrer is %s", data, this->clazzDescriptor);
    return VM_CONTEXT::env->NewStringUTF(data);
}

jclass VmMethod::resolveClass(u4 idx) const {
    const char *clazzName = this->dexFile->dexStringByTypeIdx(idx);
    jclass retClass;
    if (clazzName[0] != '\0' && clazzName[1] == '\0') {
        retClass = VM_CONTEXT::vm->findPrimitiveClass(clazzName[0]);
    } else {
        retClass = (*VM_CONTEXT::env).FindClass(clazzName);
    }
//    assert(retClass != nullptr);  // throw exception if retClass==nullptr
    LOG_D("--- resolving class %s (idx=%u referrer=%s)", clazzName, idx, this->clazzDescriptor);
    return retClass;
}


std::string VmMethod::getClassDescriptorByJClass(jclass clazz) {
    JNIEnv *env = VM_CONTEXT::env;
    jclass cClass = (*env).FindClass(VM_REFLECT::C_NAME_Class);
    jmethodID mGetCanonicalName = (*env).GetMethodID(
            cClass, VM_REFLECT::NAME_Class_getName,
            VM_REFLECT::SIGN_Class_getName);
    auto utfString = (jstring) (*env).CallObjectMethod(clazz, mGetCanonicalName);
    const char *javaDescChars = (*env).GetStringUTFChars(utfString, JNI_FALSE);
    LOG_D("descriptor: %s", javaDescChars);
    std::string retVal(javaDescChars);
    (*env).ReleaseStringUTFChars(utfString, javaDescChars);
//    (*env).DeleteLocalRef(utfString);
//    (*env).DeleteLocalRef(cClass);
    return retVal;
}

jarray VmMethod::allocArray(s4 len, u4 idx) const {
    std::string clazzName = this->dexFile->dexStringById(idx);
    LOG_D("--- resolving class %s (idx=%u referrer=%s)", clazzName.data(), idx,
          this->clazzDescriptor);
    assert(clazzName[0] == '[');
    jclass elementClazz;
    jarray retValue;
    switch (clazzName[1]) {
        case 'I':
            return (*VM_CONTEXT::env).NewIntArray(len);
        case 'C':
            return (*VM_CONTEXT::env).NewCharArray(len);
        case 'B':
            return (*VM_CONTEXT::env).NewByteArray(len);
        case 'Z':
            return (*VM_CONTEXT::env).NewBooleanArray(len);
        case 'F':
            return (*VM_CONTEXT::env).NewFloatArray(len);
        case 'D':
            return (*VM_CONTEXT::env).NewDoubleArray(len);
        case 'S':
            return (*VM_CONTEXT::env).NewShortArray(len);
        case 'J':
            return (*VM_CONTEXT::env).NewLongArray(len);
        case '[':
            elementClazz = (*VM_CONTEXT::env).FindClass(
                    clazzName.substr(1, clazzName.size() - 1).data());
            if (elementClazz == nullptr) { return nullptr; }
            retValue = (*VM_CONTEXT::env).NewObjectArray(len, elementClazz, nullptr);
//            (*VM_CONTEXT::env).DeleteLocalRef(elementClazz);
            return retValue;
        case 'L':
            elementClazz = (*VM_CONTEXT::env).FindClass(
                    clazzName.substr(2, clazzName.size() - 3).data());
            if (elementClazz == nullptr) { return nullptr; }
            retValue = (*VM_CONTEXT::env).NewObjectArray(len, elementClazz, nullptr);
//            (*VM_CONTEXT::env).DeleteLocalRef(elementClazz);
            return retValue;
        default:
            LOG_E("Unknown primitive type '%s'", clazzName.data() + 1);
            return nullptr;
    }
}

VmMethodContext::VmMethodContext(jobject caller, const VmMethod *method,
                                 jvalue *pResult, va_list param) {
    this->method = method;
    this->caller = caller;
    this->retVal = pResult;
    this->reg = (RegValue *) malloc(sizeof(RegValue) * (method->code->registersSize));
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
