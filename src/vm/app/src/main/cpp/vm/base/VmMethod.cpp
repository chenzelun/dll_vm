//
// Created by 陈泽伦 on 11/23/20.
//

#include "VmMethod.h"
#include "../../common/VmConstant.h"
#include "../../common/AndroidSystem.h"
#include "../../VmContext.h"

DexFile::DexFile(const u1 *base) {
    this->base = base;
    this->pHeader = (const DexHeader *) base;
    this->pStringIds = (const DexStringId *) (base + this->pHeader->stringIdsOff);
    this->pTypeIds = (const DexTypeId *) (base + this->pHeader->typeIdsOff);
    this->pProtoIds = (const DexProtoId *) (base + this->pHeader->protoIdsOff);
    this->pFieldIds = (const DexFieldId *) (base + this->pHeader->fieldIdsOff);
    this->pMethodIds = (const DexMethodId *) (base + this->pHeader->methodIdsOff);
    this->pClassDefs = (const DexClassDef *) (base + this->pHeader->classDefsOff);

    // dex's  offset - file start
    // cdex's offset - data
    char cdex_magic[4] = {'c', 'd', 'e', 'x'};
    if (memcmp(this->pHeader->magic, cdex_magic, 4) == 0) {
        this->base += this->pHeader->dataOff;
    }
}

jstring VmMethod::resolveString(u4 idx) const {
    const char *data = this->dexFile->dexStringById(idx);
    LOG_D_VM("+++ resolving string=%s, referrer is %s", data, this->clazzDescriptor);
    return VM_CONTEXT::env->NewStringUTF(data);
}

jclass VmMethod::resolveClass(u4 idx) const {
    std::string clazzName = this->dexFile->dexStringByTypeIdx(idx);
    jclass retClass;
    if (clazzName[0] != '\0' && clazzName[1] == '\0') {
        retClass = VM_CONTEXT::vm->findPrimitiveClass(clazzName[0]);
    } else {
        clazzName = clazzName.substr(1, clazzName.size() - 2);
        retClass = (*VM_CONTEXT::env).FindClass(clazzName.data());
    }
    LOG_D_VM("--- resolving class %s (idx=%u referrer=%s)",
             clazzName.data(), idx, this->clazzDescriptor);
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
    LOG_D_VM("descriptor: %s", javaDescChars);
    std::string retVal(javaDescChars);
    (*env).ReleaseStringUTFChars(utfString, javaDescChars);
//    (*env).DeleteLocalRef(utfString);
//    (*env).DeleteLocalRef(cClass);
    return retVal;
}

jarray VmMethod::allocArray(s4 len, u4 idx) const {
    std::string clazzName = this->dexFile->dexStringById(idx);
    LOG_D_VM("--- resolving class %s (idx=%u referrer=%s)", clazzName.data(), idx,
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

/**
 * resolve static field or obj's field.
 * static field: obj == nullptr
 * obj's field:  obj != nullptr
 * @param idx: FieldId
 * @param obj: obj
 * @param retVal: the value of field
 * @return true or false. If false, must catch exception.
 */
bool VmMethod::resolveField(u4 idx, jobject obj, RegValue *retVal) const {
    LOG_D_VM("--- resolving field %u (referrer=%s)",
             idx, this->clazzDescriptor);
    JNIEnv *env = VM_CONTEXT::env;
    const DexFieldId *pFieldId = this->dexFile->dexGetFieldId(idx);
    jclass resClazz = this->resolveClass(pFieldId->classIdx);
    if (resClazz == nullptr) {
        LOG_E("can't found class: %s", VmMethod::getClassDescriptorByJClass(resClazz).data());
        return false;
    }
    const char *fName = this->dexFile->dexStringById(pFieldId->nameIdx);
    const char *fSign = this->dexFile->dexStringByTypeIdx(pFieldId->typeIdx);
    jfieldID resField = nullptr;
    if (obj == nullptr) {
        resField = (*env).GetStaticFieldID(resClazz, fName, fSign);
    } else {
        resField = (*env).GetFieldID(resClazz, fName, fSign);
    }
    if (resField == nullptr) {
        LOG_E("can't found field: %s in class: %s",
              fName, VmMethod::getClassDescriptorByJClass(resClazz).data());
        return false;
    }
    switch (fSign[0]) {
        case 'I':
            retVal->i = obj == nullptr ? (*env).GetStaticIntField(resClazz, resField)
                                       : (*env).GetIntField(obj, resField);
            break;
        case 'F':
            retVal->f = obj == nullptr ? (*env).GetStaticFloatField(resClazz, resField)
                                       : (*env).GetFloatField(obj, resField);
            break;
        case 'Z':
            retVal->z = obj == nullptr ? (*env).GetStaticBooleanField(resClazz, resField)
                                       : (*env).GetBooleanField(obj, resField);
            break;
        case 'B':
            retVal->b = obj == nullptr ? (*env).GetStaticByteField(resClazz, resField)
                                       : (*env).GetByteField(obj, resField);
            break;
        case 'C':
            retVal->c = obj == nullptr ? (*env).GetStaticCharField(resClazz, resField)
                                       : (*env).GetCharField(obj, resField);
            break;
        case 'S':
            retVal->s = obj == nullptr ? (*env).GetStaticShortField(resClazz, resField)
                                       : (*env).GetShortField(obj, resField);
            break;
        case 'J':
            retVal->j = obj == nullptr ? (*env).GetStaticLongField(resClazz, resField)
                                       : (*env).GetLongField(obj, resField);
            break;
        case 'D':
            retVal->d = obj == nullptr ? (*env).GetStaticDoubleField(resClazz, resField)
                                       : (*env).GetDoubleField(obj, resField);
            break;
        case '[':
        case 'L':
            retVal->l = obj == nullptr ? (*env).GetStaticObjectField(resClazz, resField)
                                       : (*env).GetObjectField(obj, resField);
            break;

        default:
            LOG_E("error type of field...");
//            (*env).DeleteLocalRef(resClazz);
            throw VMException("error type of field... cc");
    }
//    (*env).DeleteLocalRef(resClazz);
    LOG_D_VM("--- resolving field %s (referrer=%s)",
             fName, this->clazzDescriptor);
    return true;
}

const char *VmMethod::resolveFieldName(u4 idx) const {
    const DexFieldId *pFieldId = this->dexFile->dexGetFieldId(idx);
    LOG_D_VM("--- resolving field %s (referrer=%s)",
             this->dexFile->dexStringById(pFieldId->nameIdx), this->clazzDescriptor);
    return this->dexFile->dexStringById(pFieldId->nameIdx);
}

bool VmMethod::resolveSetField(u4 idx, jobject obj, const RegValue *val) const {
    LOG_D_VM("--- resolving field %u (referrer=%s)",
             idx, this->clazzDescriptor);
    JNIEnv *env = VM_CONTEXT::env;
    const DexFieldId *pFieldId = this->dexFile->dexGetFieldId(idx);
    jclass resClazz = this->resolveClass(pFieldId->classIdx);
    if (resClazz == nullptr) {
        LOG_E("can't found class: %s", VmMethod::getClassDescriptorByJClass(resClazz).data());
        return false;
    }
    const char *fName = this->dexFile->dexStringById(pFieldId->nameIdx);
    const char *fSign = this->dexFile->dexStringByTypeIdx(pFieldId->typeIdx);
    jfieldID resField;
    if (obj == nullptr) {
        resField = (*env).GetStaticFieldID(resClazz, fName, fSign);
    } else {
        resField = (*env).GetFieldID(resClazz, fName, fSign);
    }
    if (resField == nullptr) {
        LOG_E("can't found field: %s in class: %s",
              fName, VmMethod::getClassDescriptorByJClass(resClazz).data());
        return false;
    }
    switch (fSign[0]) {
        case 'I':
            obj == nullptr ? (*env).SetStaticIntField(resClazz, resField, val->i)
                           : (*env).SetIntField(obj, resField, val->i);
            break;
        case 'F':
            obj == nullptr ? (*env).SetStaticFloatField(resClazz, resField, val->f)
                           : (*env).SetFloatField(obj, resField, val->f);
            break;
        case 'Z':
            obj == nullptr ? (*env).SetStaticBooleanField(resClazz, resField, val->z)
                           : (*env).SetBooleanField(obj, resField, val->z);
            break;
        case 'B':
            obj == nullptr ? (*env).SetStaticByteField(resClazz, resField, val->b)
                           : (*env).SetByteField(obj, resField, val->b);
            break;
        case 'C':
            obj == nullptr ? (*env).SetStaticCharField(resClazz, resField, val->c)
                           : (*env).SetCharField(obj, resField, val->c);
            break;
        case 'S':
            obj == nullptr ? (*env).SetStaticShortField(resClazz, resField, val->s)
                           : (*env).SetShortField(obj, resField, val->s);
            break;
        case 'J':
            obj == nullptr ? (*env).SetStaticLongField(resClazz, resField, val->j)
                           : (*env).SetLongField(obj, resField, val->j);
            break;
        case 'D':
            obj == nullptr ? (*env).SetStaticDoubleField(resClazz, resField, val->d)
                           : (*env).SetDoubleField(obj, resField, val->d);
            break;
        case '[':
            if (fSign[1] == 'L') {
                jobject resFieldJava = (*env).ToReflectedField(
                        resClazz, resField, (jboolean) (obj == nullptr));
                jclass cField = (*env).GetObjectClass(resFieldJava);
                jmethodID mFieldSet = (*env).GetMethodID(
                        cField, VM_REFLECT::NAME_Field_set, VM_REFLECT::SIGN_Field_set);
                (*env).CallVoidMethod(resFieldJava, mFieldSet, obj, val->l);
                break;
            }
            // no break

        case 'L':
            obj == nullptr ? (*env).SetStaticObjectField(resClazz, resField, val->l)
                           : (*env).SetObjectField(obj, resField, val->l);
            break;

        default:
            LOG_E("error type of field...");
//            (*env).DeleteLocalRef(resClazz);
            throw VMException("error type of field... cc");
    }
//    (*env).DeleteLocalRef(resClazz);
    LOG_D_VM("--- resolving field %s (referrer=%s)",
             fName, this->clazzDescriptor);
    return true;
}

jmethodID VmMethod::resolveMethod(u4 idx, bool isStatic) const {
    const DexMethodId *dexMethodId = this->dexFile->dexGetMethodId(idx);
    const char *resName = this->dexFile->dexStringById(dexMethodId->nameIdx);
    LOG_D_VM("--- resolving method=%s (idx=%u class=%s)", resName, idx,
             this->dexFile->dexStringByTypeIdx(dexMethodId->classIdx));
    std::string sign = this->resolveMethodSign(dexMethodId->protoIdx);
    jclass resClass = this->resolveClass(dexMethodId->classIdx);
    if (resClass == nullptr) {
        return nullptr;
    }
    jmethodID ret;
    if (isStatic) {
        ret = (*VM_CONTEXT::env).GetStaticMethodID(resClass, resName, sign.data());
    } else {
        ret = (*VM_CONTEXT::env).GetMethodID(resClass, resName, sign.data());
    }
    return ret;
}

std::string VmMethod::resolveMethodSign(u4 idx) const {
    const DexProtoId *dexProtoId = this->dexFile->dexGetProtoId(idx);
    std::string ret = "(";
    if (dexProtoId->parametersOff > 0) {
        const DexTypeList *dexTypeList =
                this->dexFile->dexGetProtoParameters(dexProtoId->parametersOff);
        for (int i = 0; i < dexTypeList->size; ++i) {
            ret += this->dexFile->dexStringByTypeIdx(dexTypeList->list[i].typeIdx);
        }
    }
    ret += ")";
    ret += this->dexFile->dexStringByTypeIdx(dexProtoId->returnTypeIdx);
    LOG_D_VM("--- resolving proto(%u) sign %s", idx, ret.data());
    return ret;
}

VmMethod *VmMethod::reset(jmethodID jniMethod, bool isUpdateCode) {
    void *artMethod = jniMethod;
    this->method_id = ((ArtMethod_26_28 *) artMethod)->dex_method_index;
    auto *artClass = (ArtClass *) (uint64_t) ((ArtMethod_26_28 *) artMethod)->declaring_class;
    void *artDexCache = (void *) (uint64_t) artClass->dex_cache;
    auto *artDexFile = (ArtDexFile_28 *) ((ArtDexCache_26_28 *) artDexCache)->dex_file;
    this->dexFile = new DexFile(artDexFile->begin);
    const DexMethodId *pDexMethodId =
            this->dexFile->dexGetMethodId(((ArtMethod_26_28 *) artMethod)->dex_method_index);
    this->protoId = this->dexFile->dexGetProtoId(pDexMethodId->protoIdx);
    this->accessFlags = ((ArtMethod_26_28 *) artMethod)->access_flags;
    this->clazzDescriptor = this->dexFile->dexStringByTypeIdx(pDexMethodId->classIdx);
    this->name = this->dexFile->dexStringById(pDexMethodId->nameIdx);

    if (isUpdateCode) {
        // update code.
        this->code = (CodeItemData *) VM_CONTEXT::vmKFCFile->getCode(this->method_id);
        this->triesAndHandlersBuf = (u1 *) (this->code->insns + this->code->insnsSize);
        this->triesAndHandlersBuf += this->code->insnsSize & 0x01u ? 0x02 : 0x00;
    } else {
        this->code = nullptr;
        this->triesAndHandlersBuf = nullptr;
        LOG_W("isUpdateCode: false.");
    }
    return this;
}


void VmMethodContext::printVmMethodContext() const {
    LOG_D("current method: %s#%s", this->method->clazzDescriptor, this->method->name);
    for (int i = 0; i < this->method->code->registersSize; ++i) {
        LOG_D("reg[%d]: 0x%016lx", i, this->reg[i].u8);
    }

    LOG_D("src1: 0x%04x, src2: 0x%04x, dst: 0x%04x",
          this->tmp->src1, this->tmp->src2, this->tmp->dst);
    LOG_D("retVal: %ld", this->retVal->j);
    LOG_D("exception: %p", this->curException);
    LOG_D("val_1: 0x%016lx, val_2: 0x%016lx", this->tmp->val_1.u8, this->tmp->val_2.u8);
    LOG_D("pc: %u, cur insns: 0x%02x", this->pc_cur(), this->fetch_op());
    LOG_D("state: %u", this->state);
}

#if defined(VM_DEBUG_FULL)
void VmMethodContext::printMethodInsns() const {
    LOG_D_VM("current method: %s#%s", this->method->clazzDescriptor, this->method->name);
    for (int i = 0; i < this->method->code->insnsSize; ++i) {
        LOG_D_VM("insns[%2d]: 0x%04x", i, this->method->code->insns[i]);
    }
}
#endif

uint32_t VmMethodContext::regCacheKey = 0;
uint32_t VmMethodContext::methodCacheKey = 0;

void VmMethodContext::resetWithoutParams(jmethodID methodId, jvalue *pResult) {
    if (VmMethodContext::regCacheKey == 0 || VmMethodContext::methodCacheKey == 0) {
        assert(VmMethodContext::regCacheKey == 0);
        assert(VmMethodContext::methodCacheKey == 0);
        VmMethodContext::regCacheKey = VM_CONTEXT::vm->newCacheType(sizeof(RegValue));
        VmMethodContext::methodCacheKey = VM_CONTEXT::vm->newCacheType(sizeof(VmMethod));
    }
    assert(VmMethodContext::regCacheKey != 0);
    assert(VmMethodContext::methodCacheKey != 0);

    this->method = ((VmMethod *) VM_CONTEXT::vm->mallocCache(
            VmMethodContext::methodCacheKey, 1))->reset(methodId, true);
    assert(this->method->code != nullptr);
    this->retVal = pResult;
    this->reg = (RegValue *) VM_CONTEXT::vm->mallocCache(
            VmMethodContext::regCacheKey, this->method->code->registersSize);
    this->pc = 0;
    this->tmp = VM_CONTEXT::vm->getTempDataBuf();
    this->state = VmMethodContextState::Running;
}

void VmMethodContext::pushParams(jobject caller, va_list param) const {
    // 参数入寄存器
    // [0] is return value.
    const char *desc = this->method->dexFile->dexStringById(method->protoId->shortyIdx) + 1;
    int verifyCount = 0;
    int startReg = this->method->code->registersSize - method->code->insSize;

    // push this
    if (!DexFile::isStaticMethod(this->method->accessFlags)) {
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

    if (verifyCount != this->method->code->insSize) {
        LOG_E("Got vfy count=%d insSize=%d for %s",
              verifyCount,
              this->method->code->insSize,
              this->method->name);
        throw VMException("error param, and can't push them to vm reg.");
    }
}

void VmMethodContext::reset(
        jobject caller, jmethodID methodId, jvalue *pResult, va_list param) {
    this->resetWithoutParams(methodId, pResult);
    this->pushParams(caller, param);
}

void VmMethodContext::release() const {
    VM_CONTEXT::vm->freeCache(VmMethodContext::methodCacheKey, 1);
    VM_CONTEXT::vm->freeCache(VmMethodContext::regCacheKey, this->method->code->registersSize);
}
