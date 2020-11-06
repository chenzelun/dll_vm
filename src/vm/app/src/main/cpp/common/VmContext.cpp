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

void VmContext::loadDexFromMemory() {
    LOG_D("finish, loadDexFromMemory");
    JNIEnv *env = VM_CONTEXT.env;
    // update mCookie
    LOG_D("0");
    jobject oClassLoader = Util::getClassLoader();
    jclass cClassLoader = (*env).GetObjectClass(oClassLoader);
    jclass cBaseDexClassLoader = (*env).GetSuperclass(cClassLoader);
    jfieldID fPathList = (*env).GetFieldID(
            cBaseDexClassLoader, VM_REFLECT::NAME_DexClassLoader_PathList,
            VM_REFLECT::SIGN_DexClassLoader_PathList);
    jobject oPathList = (*env).GetObjectField(oClassLoader, fPathList);
    LOG_D("1");
    jclass cDexPathList = (*env).GetObjectClass(oPathList);
    jfieldID fDexElements = (*env).GetFieldID(
            cDexPathList, VM_REFLECT::NAME_DexElements, VM_REFLECT::SIGN_DexElements);
    auto oDexElements =
            reinterpret_cast<jobjectArray>((*env).GetObjectField(oPathList, fDexElements));
    LOG_D("2");
    VmFileData dexFileData;
    VM_CONTEXT.vmDataFile->findFileByName(VM_CONFIG::DEST_APP_DEX_FILE_NAME, dexFileData);
    jclass cDexFile = (*env).FindClass(VM_REFLECT::C_NAME_DexFile);
    jbyteArray byteArray = (*env).NewByteArray(dexFileData.getDataSize());
    (env)->SetByteArrayRegion(byteArray, 0, dexFileData.getDataSize(),
                              reinterpret_cast<const jbyte *>(dexFileData.getData()));
    jclass cByteBuffer = (*env).FindClass(VM_REFLECT::C_NAME_ByteBuffer);
    jmethodID mWrap = (*env).GetStaticMethodID(
            cByteBuffer, VM_REFLECT::NAME_ByteBuffer_wrap, VM_REFLECT::SIGN_ByteBuffer_wrap);
    jobject oBuf = (*env).CallStaticObjectMethod(cByteBuffer, mWrap, byteArray);
    jmethodID mDexFileInit = (*env).GetMethodID(
            cDexFile, VM_REFLECT::NAME_DexFile_init, VM_REFLECT::SIGN_DexFile_init);
    jobject oDexFile = (*env).NewObject(cDexFile, mDexFileInit, oBuf);
    if ((*env).ExceptionCheck()) {
        LOG_E("error");
        (*env).ExceptionDescribe();
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
            fMCookie = (*env).GetFieldID(
                    cDexFile, VM_REFLECT::NAME_DexFile_mCookie, VM_REFLECT::SIGN_DexFile_mCookie);
            LOG_D("new cookie: %p", (*env).GetObjectField(oDexFile, fMCookie));
            break;

        default:
            LOG_E("error android api: %d", android_api_num);
            assert(false);
    }
    // Log end.

    jclass cElement = (*env).FindClass(VM_REFLECT::C_NAME_DexPathList_Element);
    jmethodID mElement = (*env).GetMethodID(
            cElement, VM_REFLECT::NAME_DexPathList_Element_init,
            VM_REFLECT::SIGN_DexPathList_Element_init);
    jobject oElement = (*env).NewObject(cElement, mElement, nullptr, JNI_FALSE, nullptr, oDexFile);
    LOG_D("5");
    jsize oldLenDexElements = (*env).GetArrayLength(oDexElements);
    LOG_D("oldLenDexElements: %d", oldLenDexElements);

    jobjectArray oDexElementsNew = (*env).NewObjectArray(
            oldLenDexElements + 1, cElement, nullptr);
    for (int i = 0; i < oldLenDexElements; i++) {
        auto t = (*env).GetObjectArrayElement(oDexElements, i);
        (*env).SetObjectArrayElement(oDexElementsNew, i, t);
    }
    (*env).SetObjectArrayElement(oDexElementsNew, oldLenDexElements, oElement);
    LOG_D("6");
    (*env).SetObjectField(oPathList, fDexElements, oDexElementsNew);
    LOG_D("7");
    (*env).DeleteLocalRef(cClassLoader);
    (*env).DeleteLocalRef(cBaseDexClassLoader);
    (*env).DeleteLocalRef(cDexPathList);
    (*env).DeleteLocalRef(cDexFile);
    (*env).DeleteLocalRef(cByteBuffer);
    (*env).DeleteLocalRef(cElement);
    LOG_D("finish, loadDexFromMemory");
}

void VmContext::changeTopApplication() {
    LOG_D("start, changeTopApplication");
    JNIEnv *env = VM_CONTEXT.env;
    // config dynamic loading env

    LOG_D("1");
    // get main thread's object
    jclass cActivityThread = (*env).FindClass(VM_REFLECT::C_NAME_ActivityThread);
    jmethodID mCurrentActivityThread = (*env).GetStaticMethodID(
            cActivityThread, VM_REFLECT::NAME_CurrentActivityThread,
            VM_REFLECT::SIGN_CurrentActivityThread);
    jobject oCurrentActivityThread = (*env).CallStaticObjectMethod(
            cActivityThread, mCurrentActivityThread);

    LOG_D("2");
    VmKeyValueData applicationName;
    VM_CONTEXT.vmDataFile->findValByKey(VM_CONFIG::DEST_APP_APPLICATION_NAME, applicationName);
    jstring sAppName = (*env).NewStringUTF(applicationName.getVal());
    //有值的话调用该 Application
    jfieldID fMBoundApplication = (*env).GetFieldID(
            cActivityThread, VM_REFLECT::NAME_ActicityThread_mBoundApplication,
            VM_REFLECT::SIGN_ActicityThread_mBoundApplication);
    jobject oMBoundApplication = (*env).GetObjectField(
            oCurrentActivityThread, fMBoundApplication);

    LOG_D("3");
    jclass cAppBindData = (*env).GetObjectClass(oMBoundApplication);
    jfieldID fInfo = (*env).GetFieldID(
            cAppBindData, VM_REFLECT::NAME_AppBindData_info, VM_REFLECT::SIGN_AppBindData_info);
    jobject oLoadedApkInfo = (*env).GetObjectField(oMBoundApplication, fInfo);

    LOG_D("4");
    jclass cLoadedApk = (*env).FindClass(VM_REFLECT::C_NAME_LoadedApk);
    jfieldID fMApplication = (*env).GetFieldID(
            cLoadedApk, VM_REFLECT::NAME_LoadedApk_mApplication,
            VM_REFLECT::SIGN_LoadedApk_mApplication);
    (*env).SetObjectField(oLoadedApkInfo, fMApplication, nullptr);

    //set 'mApplication = null' in current thread.
    LOG_D("5");
    jfieldID fMInitialApplication = (*env).GetFieldID(
            cActivityThread, VM_REFLECT::NAME_ActivityThread_mInitialApplication,
            VM_REFLECT::SIGN_ActivityThread_mInitialApplication);
    jobject oMInitialApplication = (*env).GetObjectField(
            oCurrentActivityThread, fMInitialApplication);

    LOG_D("6");
    jfieldID fMAllApplications = (*env).GetFieldID(
            cActivityThread, VM_REFLECT::NAME_ActivityThread_mAllApplications,
            VM_REFLECT::SIGN_ActivityThread_mAllApplications);
    jobject oMAllApplications = (*env).GetObjectField(oCurrentActivityThread, fMAllApplications);
    assert(oMAllApplications != nullptr);

    LOG_D("7");
    jclass cArrayList = (*env).GetObjectClass(oMAllApplications);
    jmethodID mRemove = (*env).GetMethodID(
            cArrayList, VM_REFLECT::NAME_ArrayList_remove, VM_REFLECT::SIGN_ArrayList_remove);
    auto isRemove = (*env).CallBooleanMethod(oMAllApplications, mRemove, oMInitialApplication);
    if (isRemove == JNI_TRUE) {
        LOG_D("delete oldApplication    ok...");
    } else {
        LOG_E("delete oldApplication    failed...");
    }

    LOG_D("8");
    jfieldID fMApplicationInfo = (*env).GetFieldID(
            cLoadedApk, VM_REFLECT::NAME_LoadedApk_mApplicationInfo,
            VM_REFLECT::SIGN_LoadedApk_mApplicationInfo);
    jobject oAppInfo0 = (*env).GetObjectField(oLoadedApkInfo, fMApplicationInfo);
    assert(oAppInfo0 != nullptr);

    LOG_D("9");
    jclass cApplicationInfo = (*env).FindClass(VM_REFLECT::C_NAME_ApplicationInfo);
    jfieldID fClassName = (*env).GetFieldID(
            cApplicationInfo, VM_REFLECT::NAME_ApplicationInfo_className,
            VM_REFLECT::SIGN_ApplicationInfo_className);
    (*env).SetObjectField(oAppInfo0, fClassName, sAppName);

    LOG_D("10");
    jfieldID fAppInfo = (*env).GetFieldID(
            cAppBindData, VM_REFLECT::NAME_AppBindData_appInfo,
            VM_REFLECT::SIGN_AppBindData_appInfo);
    jobject oAppInfo1 = (*env).GetObjectField(oMBoundApplication, fAppInfo);
    assert(oAppInfo1 != nullptr);

    LOG_D("11");
    (*env).SetObjectField(oAppInfo1, fClassName, sAppName);

    LOG_D("12");
    jmethodID mMakeApplication = (*env).GetMethodID(
            cLoadedApk, VM_REFLECT::NAME_MakeApplication, VM_REFLECT::SIGN_MakeApplication);
    jobject oApp = (*env).CallObjectMethod(
            oLoadedApkInfo, mMakeApplication, JNI_FALSE, nullptr);
    jobject oApplication = (*env).NewGlobalRef(oApp);

    LOG_D("13");
    (*env).SetObjectField(oCurrentActivityThread, fMInitialApplication, oApplication);

    LOG_D("14");
    // change ContentProvider
    jfieldID fMProviderMap = (*env).GetFieldID(
            cActivityThread, VM_REFLECT::NAME_ActivityThread_mProviderMap,
            VM_REFLECT::SIGN_ActivityThread_mProviderMap);
    jobject oMProviderMap = (*env).GetObjectField(oCurrentActivityThread, fMProviderMap);
    assert(oMProviderMap != nullptr);

    LOG_D("15");
    jclass cMap = (*env).FindClass(VM_REFLECT::C_NAME_ArrayMap);
    jmethodID mValues = (*env).GetMethodID(
            cMap, VM_REFLECT::NAME_ArrayMap_values, VM_REFLECT::SIGN_ArrayMap_values);
    jobject oValues = (*env).CallObjectMethod(oMProviderMap, mValues);
    jclass cCollection = (*env).GetObjectClass(oValues);
    jmethodID mIterator = (*env).GetMethodID(
            cCollection, VM_REFLECT::NAME_Collection_iterator,
            VM_REFLECT::SIGN_Collection_iterator);
    jobject oIterator = (*env).CallObjectMethod(oValues, mIterator);
    jclass cIterator = (*env).GetObjectClass(oIterator);
    jmethodID mHasNext = (*env).GetMethodID(
            cIterator, VM_REFLECT::NAME_Iterator_hasNext, VM_REFLECT::SIGN_Iterator_hasNext);
    jmethodID mNext = (*env).GetMethodID(
            cIterator, VM_REFLECT::NAME_Iterator_next, VM_REFLECT::SIGN_Iterator_next);
    jclass cProviderClientRecord = (*env).FindClass(VM_REFLECT::C_NAME_ProviderClientRecord);
    jfieldID fMLocalProvider = (*env).GetFieldID(
            cProviderClientRecord, VM_REFLECT::NAME_ProviderClientRecord_mLocalProvider,
            VM_REFLECT::SIGN_ProviderClientRecord_mLocalProvider);
    jclass cContentProvider = (*env).FindClass(VM_REFLECT::C_NAME_ContentProvider);
    jfieldID fMContext = (*env).GetFieldID(
            cContentProvider, VM_REFLECT::NAME_ContentProvider_mContext,
            VM_REFLECT::SIGN_ContentProvider_mContext);
    jclass cApplication = (*env).GetObjectClass(oApp);
    jmethodID mGetApplicationContext = (*env).GetMethodID(
            cApplication, VM_REFLECT::NAME_GetApplicationContext,
            VM_REFLECT::SIGN_GetApplicationContext);
    jobject oContext1 = (*env).CallObjectMethod(oApplication, mGetApplicationContext);
    assert(oContext1 != nullptr);

    LOG_D("16");
    while ((*env).CallBooleanMethod(oIterator, mHasNext)) {
        jobject oProviderClientRecord = (*env).CallObjectMethod(oIterator, mNext);
        jobject oMLocalProvider = (*env).GetObjectField(oProviderClientRecord, fMLocalProvider);
        if (oMLocalProvider != nullptr) {
            (*env).SetObjectField(oMLocalProvider, fMContext, oContext1);
        }
    }

    LOG_D("17");
    jclass c2Application = (*env).GetObjectClass(oApplication);
    jmethodID mOnCreate = (*env).GetMethodID(
            c2Application, VM_REFLECT::NAME_Application_onCreate,
            VM_REFLECT::SIGN_Application_onCreate);
    (*env).CallVoidMethod(oApplication, mOnCreate);

    LOG_D("99.9999999999");
    (*env).DeleteLocalRef(cActivityThread);
    (*env).DeleteLocalRef(cApplicationInfo);
    (*env).DeleteLocalRef(cAppBindData);
    (*env).DeleteLocalRef(cApplication);
    (*env).DeleteLocalRef(cArrayList);
    (*env).DeleteLocalRef(cCollection);
    (*env).DeleteLocalRef(cContentProvider);
    (*env).DeleteLocalRef(cIterator);
    (*env).DeleteLocalRef(cLoadedApk);
    (*env).DeleteLocalRef(cProviderClientRecord);
    (*env).DeleteLocalRef(cMap);
    (*env).DeleteLocalRef(c2Application);
    LOG_D("finish, changeTopApplication");
}
