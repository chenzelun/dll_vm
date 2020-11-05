//
// Created by 陈泽伦 on 2020/11/5.
//

#include "VmContext.h"
#include "Util.h"
#include "VmConstant.h"

void VmContext::initVmDataFileOfVmContext() {
    LOG_D("start, initVmDataFileOfVmContext");
    uint32_t bufSize = 0;
    const uint8_t *vmDataFileBuf =
            Util::getFileBufFromAssets(VM_CONFIG::VM_DATA_FILE_NAME, bufSize);
    assert(bufSize != 0);
    VM_CONTEXT.vmDataFile = new VmDataFile(vmDataFileBuf, bufSize);
    LOG_D("finish, initVmDataFileOfVmContext");
}

void VmContext::updateNativeLibraryDirectories() {
    LOG_D("start, updateNativeLibraryDirectories");
    JNIEnv *env = VM_CONTEXT.env;
    jobject oClassLoader = Util::getClassLoader();
    jclass cClassLoader = (*env).GetObjectClass(oClassLoader);
    jclass cBaseDexClassLoader = (*env).GetSuperclass(cClassLoader);
    jfieldID fPathList = (*env).GetFieldID(
            cBaseDexClassLoader, VM_REFLECT::NAME_DexClassLoader_PathList,
            VM_REFLECT::SIGN_DexClassLoader_PathList);
    jobject oPathList = (*env).GetObjectField(oClassLoader, fPathList);
    LOG_D("1");
    jclass cDexPathList = (*env).GetObjectClass(oPathList);
    jfieldID fNativeLibraryPathElements = (*env).GetFieldID(
            cDexPathList, VM_REFLECT::NAME_NativeLibraryPathElements,
            VM_REFLECT::SIGN_NativeLibraryPathElements);
    auto oNativeLibraryPathElements = reinterpret_cast<jobjectArray >((*env).GetObjectField(
            oPathList, fNativeLibraryPathElements));
    LOG_D("2");
    jsize oldLenNativeLibraryPathElements = (*env).GetArrayLength(oNativeLibraryPathElements);
    LOG_D("oldLenNativeLibraryDirectories: %d", oldLenNativeLibraryPathElements);
    jclass cNativeLibraryElement = (*env).FindClass(VM_REFLECT::C_NAME_NativeLibraryElement);
    auto oNativeLibraryPathElementsNew = (*env).NewObjectArray(
            oldLenNativeLibraryPathElements + 1, cNativeLibraryElement, nullptr);
    for (int i = 0; i < oldLenNativeLibraryPathElements; i++) {
        auto t = (*env).GetObjectArrayElement(oNativeLibraryPathElements, i);
        (*env).SetObjectArrayElement(oNativeLibraryPathElementsNew, i, t);
    }
    LOG_D("3");
    jclass cFile = (*env).FindClass(VM_REFLECT::C_NAME_File);
    jmethodID mFile = (*env).GetMethodID(
            cFile, VM_REFLECT::NAME_File_init, VM_REFLECT::SIGN_File_init);
    jstring filenameStr = (*env).NewStringUTF(Util::getLibDir().data());
    jobject oFile = (*env).NewObject(cFile, mFile, filenameStr);
    jmethodID mNativeLibraryElement = (*env).GetMethodID(
            cNativeLibraryElement, VM_REFLECT::NAME_NativeLibraryElement_init,
            VM_REFLECT::SIGN_NativeLibraryElement_init);
    jobject oNativeLibraryElement = (*env).NewObject(
            cNativeLibraryElement, mNativeLibraryElement, oFile);
    (*env).SetObjectArrayElement(
            oNativeLibraryPathElementsNew, oldLenNativeLibraryPathElements, oNativeLibraryElement);
    (*env).SetObjectField(oPathList, fNativeLibraryPathElements, oNativeLibraryPathElementsNew);
    LOG_D("4");
    (*env).DeleteLocalRef(cClassLoader);
    (*env).DeleteLocalRef(cBaseDexClassLoader);
    (*env).DeleteLocalRef(cDexPathList);
    (*env).DeleteLocalRef(cNativeLibraryElement);
    (*env).DeleteLocalRef(cFile);
    LOG_D("finish, updateNativeLibraryDirectories");
}
