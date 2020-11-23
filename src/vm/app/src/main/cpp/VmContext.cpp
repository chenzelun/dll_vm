//
// Created by 陈泽伦 on 2020/11/5.
//

#include "VmContext.h"
#include "common/Util.h"
#include "common/VmConstant.h"

JNIEnv *VM_CONTEXT::env = nullptr;
VmDataFile *VM_CONTEXT::vmDataFile = nullptr;
VmKeyFuncCodeFile *VM_CONTEXT::vmKFCFile = nullptr;
Vm *VM_CONTEXT::vm = nullptr;

void VM_CONTEXT::initVmDataFileOfVC() {
    LOG_D("start, initVmDataFileOfVC");
    uint32_t bufSize = 0;
    const uint8_t *vmDataFileBuf =
            Util::getFileBufFromAssets(VM_CONFIG::VM_DATA_FILE_NAME, bufSize);
    assert(bufSize != 0);
    VM_CONTEXT::vmDataFile = new VmDataFile(vmDataFileBuf, bufSize);
    LOG_D("finish, initVmDataFileOfVC");
}

void VM_CONTEXT::updateNativeLibraryDirectories() {
    LOG_D("start, updateNativeLibraryDirectories");
    JNIEnv *pEnv = VM_CONTEXT::env;
    jobject oClassLoader = Util::getClassLoader();
    jclass cClassLoader = (*pEnv).GetObjectClass(oClassLoader);
    jclass cBaseDexClassLoader = (*pEnv).GetSuperclass(cClassLoader);
    jfieldID fPathList = (*pEnv).GetFieldID(
            cBaseDexClassLoader, VM_REFLECT::NAME_DexClassLoader_PathList,
            VM_REFLECT::SIGN_DexClassLoader_PathList);
    jobject oPathList = (*pEnv).GetObjectField(oClassLoader, fPathList);
    LOG_D("1");
    jclass cDexPathList = (*pEnv).GetObjectClass(oPathList);
    jfieldID fNativeLibraryPathElements = (*pEnv).GetFieldID(
            cDexPathList, VM_REFLECT::NAME_NativeLibraryPathElements,
            VM_REFLECT::SIGN_NativeLibraryPathElements);
    auto oNativeLibraryPathElements = reinterpret_cast<jobjectArray >((*pEnv).GetObjectField(
            oPathList, fNativeLibraryPathElements));
    LOG_D("2");
    jsize oldLenNativeLibraryPathElements = (*pEnv).GetArrayLength(oNativeLibraryPathElements);
    LOG_D("oldLenNativeLibraryDirectories: %d", oldLenNativeLibraryPathElements);
    jclass cNativeLibraryElement = (*pEnv).FindClass(VM_REFLECT::C_NAME_NativeLibraryElement);
    auto oNativeLibraryPathElementsNew = (*pEnv).NewObjectArray(
            oldLenNativeLibraryPathElements + 1, cNativeLibraryElement, nullptr);
    for (int i = 0; i < oldLenNativeLibraryPathElements; i++) {
        auto t = (*pEnv).GetObjectArrayElement(oNativeLibraryPathElements, i);
        (*pEnv).SetObjectArrayElement(oNativeLibraryPathElementsNew, i, t);
    }
    LOG_D("3");
    jclass cFile = (*pEnv).FindClass(VM_REFLECT::C_NAME_File);
    jmethodID mFile = (*pEnv).GetMethodID(
            cFile, VM_REFLECT::NAME_File_init, VM_REFLECT::SIGN_File_init);
    jstring filenameStr = (*pEnv).NewStringUTF(Util::getLibDir().data());
    jobject oFile = (*pEnv).NewObject(cFile, mFile, filenameStr);
    jmethodID mNativeLibraryElement = (*pEnv).GetMethodID(
            cNativeLibraryElement, VM_REFLECT::NAME_NativeLibraryElement_init,
            VM_REFLECT::SIGN_NativeLibraryElement_init);
    jobject oNativeLibraryElement = (*pEnv).NewObject(
            cNativeLibraryElement, mNativeLibraryElement, oFile);
    (*pEnv).SetObjectArrayElement(
            oNativeLibraryPathElementsNew, oldLenNativeLibraryPathElements, oNativeLibraryElement);
    (*pEnv).SetObjectField(oPathList, fNativeLibraryPathElements, oNativeLibraryPathElementsNew);
    LOG_D("4");
    (*pEnv).DeleteLocalRef(cClassLoader);
    (*pEnv).DeleteLocalRef(cBaseDexClassLoader);
    (*pEnv).DeleteLocalRef(cDexPathList);
    (*pEnv).DeleteLocalRef(cNativeLibraryElement);
    (*pEnv).DeleteLocalRef(cFile);
    LOG_D("finish, updateNativeLibraryDirectories");
}

void VM_CONTEXT::loadDexFromMemory() {
    LOG_D("finish, loadDexFromMemory");
    JNIEnv *pEnv = VM_CONTEXT::env;
    // update mCookie
    LOG_D("0");
    jobject oClassLoader = Util::getClassLoader();
    jclass cClassLoader = (*pEnv).GetObjectClass(oClassLoader);
    jclass cBaseDexClassLoader = (*pEnv).GetSuperclass(cClassLoader);
    jfieldID fPathList = (*pEnv).GetFieldID(
            cBaseDexClassLoader, VM_REFLECT::NAME_DexClassLoader_PathList,
            VM_REFLECT::SIGN_DexClassLoader_PathList);
    jobject oPathList = (*pEnv).GetObjectField(oClassLoader, fPathList);
    LOG_D("1");
    jclass cDexPathList = (*pEnv).GetObjectClass(oPathList);
    jfieldID fDexElements = (*pEnv).GetFieldID(
            cDexPathList, VM_REFLECT::NAME_DexElements, VM_REFLECT::SIGN_DexElements);
    auto oDexElements =
            reinterpret_cast<jobjectArray>((*pEnv).GetObjectField(oPathList, fDexElements));
    LOG_D("2");
    VDF_FileData dexFileData;
    VM_CONTEXT::vmDataFile->findFileByName(VM_CONFIG::DEST_APP_DEX_FILE_NAME, dexFileData);
    jclass cDexFile = (*pEnv).FindClass(VM_REFLECT::C_NAME_DexFile);
    jbyteArray byteArray = (*pEnv).NewByteArray(dexFileData.getDataSize());
    (pEnv)->SetByteArrayRegion(byteArray, 0, dexFileData.getDataSize(),
                               reinterpret_cast<const jbyte *>(dexFileData.getData()));
    jclass cByteBuffer = (*pEnv).FindClass(VM_REFLECT::C_NAME_ByteBuffer);
    jmethodID mWrap = (*pEnv).GetStaticMethodID(
            cByteBuffer, VM_REFLECT::NAME_ByteBuffer_wrap, VM_REFLECT::SIGN_ByteBuffer_wrap);
    jobject oBuf = (*pEnv).CallStaticObjectMethod(cByteBuffer, mWrap, byteArray);
    jmethodID mDexFileInit = (*pEnv).GetMethodID(
            cDexFile, VM_REFLECT::NAME_DexFile_init, VM_REFLECT::SIGN_DexFile_init);
    jobject oDexFile = (*pEnv).NewObject(cDexFile, mDexFileInit, oBuf);
    if ((*pEnv).ExceptionCheck()) {
        LOG_E("error");
        (*pEnv).ExceptionDescribe();
        return;
    }

    LOG_D("3.5");
    // Log start: dex in memory.
    jfieldID fMCookie = nullptr;
    auto android_api_num = android_get_device_api_level();
    switch (android_api_num) {
        case __ANDROID_API_O__:
        case __ANDROID_API_O_MR1__:
        case __ANDROID_API_P__:
            fMCookie = (*pEnv).GetFieldID(
                    cDexFile, VM_REFLECT::NAME_DexFile_mCookie, VM_REFLECT::SIGN_DexFile_mCookie);
            LOG_D("new cookie: %p", (*pEnv).GetObjectField(oDexFile, fMCookie));
            break;

        default:
            LOG_E("error android api: %d", android_api_num);
            assert(false);
    }
    // Log end.

    jclass cElement = (*pEnv).FindClass(VM_REFLECT::C_NAME_DexPathList_Element);
    jmethodID mElement = (*pEnv).GetMethodID(
            cElement, VM_REFLECT::NAME_DexPathList_Element_init,
            VM_REFLECT::SIGN_DexPathList_Element_init);
    jobject oElement = (*pEnv).NewObject(cElement, mElement, nullptr, JNI_FALSE, nullptr, oDexFile);
    LOG_D("5");
    jsize oldLenDexElements = (*pEnv).GetArrayLength(oDexElements);
    LOG_D("oldLenDexElements: %d", oldLenDexElements);

    jobjectArray oDexElementsNew = (*pEnv).NewObjectArray(
            oldLenDexElements + 1, cElement, nullptr);
    for (int i = 0; i < oldLenDexElements; i++) {
        auto t = (*pEnv).GetObjectArrayElement(oDexElements, i);
        (*pEnv).SetObjectArrayElement(oDexElementsNew, i, t);
    }
    (*pEnv).SetObjectArrayElement(oDexElementsNew, oldLenDexElements, oElement);
    LOG_D("6");
    (*pEnv).SetObjectField(oPathList, fDexElements, oDexElementsNew);
    LOG_D("7");
    (*pEnv).DeleteLocalRef(cClassLoader);
    (*pEnv).DeleteLocalRef(cBaseDexClassLoader);
    (*pEnv).DeleteLocalRef(cDexPathList);
    (*pEnv).DeleteLocalRef(cDexFile);
    (*pEnv).DeleteLocalRef(cByteBuffer);
    (*pEnv).DeleteLocalRef(cElement);
    LOG_D("finish, loadDexFromMemory");
}

void VM_CONTEXT::changeTopApplication() {
    LOG_D("start, changeTopApplication");
    JNIEnv *pEnv = VM_CONTEXT::env;
    // config dynamic loading pEnv

    LOG_D("1");
    // get main thread's object
    jclass cActivityThread = (*pEnv).FindClass(VM_REFLECT::C_NAME_ActivityThread);
    jmethodID mCurrentActivityThread = (*pEnv).GetStaticMethodID(
            cActivityThread, VM_REFLECT::NAME_CurrentActivityThread,
            VM_REFLECT::SIGN_CurrentActivityThread);
    jobject oCurrentActivityThread = (*pEnv).CallStaticObjectMethod(
            cActivityThread, mCurrentActivityThread);

    LOG_D("2");
    VDF_KeyValueData applicationName;
    VM_CONTEXT::vmDataFile->findValByKey(VM_CONFIG::DEST_APP_APPLICATION_NAME, applicationName);
    jstring sAppName = (*pEnv).NewStringUTF(applicationName.getVal());
    //有值的话调用该 Application
    jfieldID fMBoundApplication = (*pEnv).GetFieldID(
            cActivityThread, VM_REFLECT::NAME_ActicityThread_mBoundApplication,
            VM_REFLECT::SIGN_ActicityThread_mBoundApplication);
    jobject oMBoundApplication = (*pEnv).GetObjectField(
            oCurrentActivityThread, fMBoundApplication);

    LOG_D("3");
    jclass cAppBindData = (*pEnv).GetObjectClass(oMBoundApplication);
    jfieldID fInfo = (*pEnv).GetFieldID(
            cAppBindData, VM_REFLECT::NAME_AppBindData_info, VM_REFLECT::SIGN_AppBindData_info);
    jobject oLoadedApkInfo = (*pEnv).GetObjectField(oMBoundApplication, fInfo);

    LOG_D("4");
    jclass cLoadedApk = (*pEnv).FindClass(VM_REFLECT::C_NAME_LoadedApk);
    jfieldID fMApplication = (*pEnv).GetFieldID(
            cLoadedApk, VM_REFLECT::NAME_LoadedApk_mApplication,
            VM_REFLECT::SIGN_LoadedApk_mApplication);
    (*pEnv).SetObjectField(oLoadedApkInfo, fMApplication, nullptr);

    //set 'mApplication = null' in current thread.
    LOG_D("5");
    jfieldID fMInitialApplication = (*pEnv).GetFieldID(
            cActivityThread, VM_REFLECT::NAME_ActivityThread_mInitialApplication,
            VM_REFLECT::SIGN_ActivityThread_mInitialApplication);
    jobject oMInitialApplication = (*pEnv).GetObjectField(
            oCurrentActivityThread, fMInitialApplication);

    LOG_D("6");
    jfieldID fMAllApplications = (*pEnv).GetFieldID(
            cActivityThread, VM_REFLECT::NAME_ActivityThread_mAllApplications,
            VM_REFLECT::SIGN_ActivityThread_mAllApplications);
    jobject oMAllApplications = (*pEnv).GetObjectField(oCurrentActivityThread, fMAllApplications);
    assert(oMAllApplications != nullptr);

    LOG_D("7");
    jclass cArrayList = (*pEnv).GetObjectClass(oMAllApplications);
    jmethodID mRemove = (*pEnv).GetMethodID(
            cArrayList, VM_REFLECT::NAME_ArrayList_remove, VM_REFLECT::SIGN_ArrayList_remove);
    auto isRemove = (*pEnv).CallBooleanMethod(oMAllApplications, mRemove, oMInitialApplication);
    if (isRemove == JNI_TRUE) {
        LOG_D("delete oldApplication    ok...");
    } else {
        LOG_E("delete oldApplication    failed...");
    }

    LOG_D("8");
    jfieldID fMApplicationInfo = (*pEnv).GetFieldID(
            cLoadedApk, VM_REFLECT::NAME_LoadedApk_mApplicationInfo,
            VM_REFLECT::SIGN_LoadedApk_mApplicationInfo);
    jobject oAppInfo0 = (*pEnv).GetObjectField(oLoadedApkInfo, fMApplicationInfo);
    assert(oAppInfo0 != nullptr);

    LOG_D("9");
    jclass cApplicationInfo = (*pEnv).FindClass(VM_REFLECT::C_NAME_ApplicationInfo);
    jfieldID fClassName = (*pEnv).GetFieldID(
            cApplicationInfo, VM_REFLECT::NAME_ApplicationInfo_className,
            VM_REFLECT::SIGN_ApplicationInfo_className);
    (*pEnv).SetObjectField(oAppInfo0, fClassName, sAppName);

    LOG_D("10");
    jfieldID fAppInfo = (*pEnv).GetFieldID(
            cAppBindData, VM_REFLECT::NAME_AppBindData_appInfo,
            VM_REFLECT::SIGN_AppBindData_appInfo);
    jobject oAppInfo1 = (*pEnv).GetObjectField(oMBoundApplication, fAppInfo);
    assert(oAppInfo1 != nullptr);

    LOG_D("11");
    (*pEnv).SetObjectField(oAppInfo1, fClassName, sAppName);

    LOG_D("12");
    jmethodID mMakeApplication = (*pEnv).GetMethodID(
            cLoadedApk, VM_REFLECT::NAME_MakeApplication, VM_REFLECT::SIGN_MakeApplication);
    jobject oApp = (*pEnv).CallObjectMethod(
            oLoadedApkInfo, mMakeApplication, JNI_FALSE, nullptr);
    jobject oApplication = (*pEnv).NewGlobalRef(oApp);

    LOG_D("13");
    (*pEnv).SetObjectField(oCurrentActivityThread, fMInitialApplication, oApplication);

    LOG_D("14");
    // change ContentProvider
    jfieldID fMProviderMap = (*pEnv).GetFieldID(
            cActivityThread, VM_REFLECT::NAME_ActivityThread_mProviderMap,
            VM_REFLECT::SIGN_ActivityThread_mProviderMap);
    jobject oMProviderMap = (*pEnv).GetObjectField(oCurrentActivityThread, fMProviderMap);
    assert(oMProviderMap != nullptr);

    LOG_D("15");
    jclass cMap = (*pEnv).FindClass(VM_REFLECT::C_NAME_ArrayMap);
    jmethodID mValues = (*pEnv).GetMethodID(
            cMap, VM_REFLECT::NAME_ArrayMap_values, VM_REFLECT::SIGN_ArrayMap_values);
    jobject oValues = (*pEnv).CallObjectMethod(oMProviderMap, mValues);
    jclass cCollection = (*pEnv).GetObjectClass(oValues);
    jmethodID mIterator = (*pEnv).GetMethodID(
            cCollection, VM_REFLECT::NAME_Collection_iterator,
            VM_REFLECT::SIGN_Collection_iterator);
    jobject oIterator = (*pEnv).CallObjectMethod(oValues, mIterator);
    jclass cIterator = (*pEnv).GetObjectClass(oIterator);
    jmethodID mHasNext = (*pEnv).GetMethodID(
            cIterator, VM_REFLECT::NAME_Iterator_hasNext, VM_REFLECT::SIGN_Iterator_hasNext);
    jmethodID mNext = (*pEnv).GetMethodID(
            cIterator, VM_REFLECT::NAME_Iterator_next, VM_REFLECT::SIGN_Iterator_next);
    jclass cProviderClientRecord = (*pEnv).FindClass(VM_REFLECT::C_NAME_ProviderClientRecord);
    jfieldID fMLocalProvider = (*pEnv).GetFieldID(
            cProviderClientRecord, VM_REFLECT::NAME_ProviderClientRecord_mLocalProvider,
            VM_REFLECT::SIGN_ProviderClientRecord_mLocalProvider);
    jclass cContentProvider = (*pEnv).FindClass(VM_REFLECT::C_NAME_ContentProvider);
    jfieldID fMContext = (*pEnv).GetFieldID(
            cContentProvider, VM_REFLECT::NAME_ContentProvider_mContext,
            VM_REFLECT::SIGN_ContentProvider_mContext);
    jclass cApplication = (*pEnv).GetObjectClass(oApp);
    jmethodID mGetApplicationContext = (*pEnv).GetMethodID(
            cApplication, VM_REFLECT::NAME_GetApplicationContext,
            VM_REFLECT::SIGN_GetApplicationContext);
    jobject oContext1 = (*pEnv).CallObjectMethod(oApplication, mGetApplicationContext);
    assert(oContext1 != nullptr);

    LOG_D("16");
    while ((*pEnv).CallBooleanMethod(oIterator, mHasNext)) {
        jobject oProviderClientRecord = (*pEnv).CallObjectMethod(oIterator, mNext);
        jobject oMLocalProvider = (*pEnv).GetObjectField(oProviderClientRecord, fMLocalProvider);
        if (oMLocalProvider != nullptr) {
            (*pEnv).SetObjectField(oMLocalProvider, fMContext, oContext1);
        }
    }

    LOG_D("17");
    jclass c2Application = (*pEnv).GetObjectClass(oApplication);
    jmethodID mOnCreate = (*pEnv).GetMethodID(
            c2Application, VM_REFLECT::NAME_Application_onCreate,
            VM_REFLECT::SIGN_Application_onCreate);
    (*pEnv).CallVoidMethod(oApplication, mOnCreate);

    LOG_D("99.9999999999");
    (*pEnv).DeleteLocalRef(cActivityThread);
    (*pEnv).DeleteLocalRef(cApplicationInfo);
    (*pEnv).DeleteLocalRef(cAppBindData);
    (*pEnv).DeleteLocalRef(cApplication);
    (*pEnv).DeleteLocalRef(cArrayList);
    (*pEnv).DeleteLocalRef(cCollection);
    (*pEnv).DeleteLocalRef(cContentProvider);
    (*pEnv).DeleteLocalRef(cIterator);
    (*pEnv).DeleteLocalRef(cLoadedApk);
    (*pEnv).DeleteLocalRef(cProviderClientRecord);
    (*pEnv).DeleteLocalRef(cMap);
    (*pEnv).DeleteLocalRef(c2Application);
    LOG_D("finish, changeTopApplication");
}

void VM_CONTEXT::initVmKeyFuncCodeFileOfVC() {
    LOG_D("start, initVmKeyFuncCodeFileOfVC.");
    VDF_FileData kfc;
    VM_CONTEXT::vmDataFile->findFileByName(VM_CONFIG::VM_KEY_FUNC_CODE_FILE_NAME, kfc);
    VM_CONTEXT::vmKFCFile = new VmKeyFuncCodeFile(kfc.getData(), kfc.getDataSize());
    LOG_D("finish, initVmKeyFuncCodeFileOfVC.");
}

void VM_CONTEXT::initVm() {
    VM_CONTEXT::vm = new Vm();
}
