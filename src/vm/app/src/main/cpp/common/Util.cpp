//
// Created by 陈泽伦 on 2020/11/2.
//

#include "Util.h"
#include "../VmContext.h"
#include "VmConstant.h"
#include "../include/AndHook.h"

#include <string>
#include <vector>
#include <sys/stat.h>
#include <android/asset_manager_jni.h>
#include <unistd.h>

std::string &Util::getBaseFilesDir() {
    static std::string baseDir;
    if (baseDir.empty()) {
        JNIEnv *env = VM_CONTEXT::env;
        jobject oContext = getAppContext();
        jclass cContext = (*env).GetObjectClass(oContext);
        jmethodID mGetFilesDir = (*env).GetMethodID(
                cContext, VM_REFLECT::NAME_GetFilesDir, VM_REFLECT::SIGN_GetFilesDir);
        jobject oFile = (*env).CallObjectMethod(oContext, mGetFilesDir);
        assert(oFile != nullptr);
        jclass cFile = (*env).GetObjectClass(oFile);
        jmethodID mGetCanonicalPath = (*env).GetMethodID(
                cFile, VM_REFLECT::NAME_GetCanonicalPath, VM_REFLECT::SIGN_GetCanonicalPath);
        auto oPath = reinterpret_cast<jstring >((*env).CallObjectMethod(oFile, mGetCanonicalPath));
        assert(oPath != nullptr);
        baseDir = (*env).GetStringUTFChars(oPath, JNI_FALSE);

        LOG_D("getBaseFilesDir success first....");
        LOG_D("baseDir: %s", baseDir.data());
//        (*env).DeleteLocalRef(cContext);
//        (*env).DeleteLocalRef(cFile);
    }
    return baseDir;
}

std::string &Util::getDataDir() {
    static std::string data;
    if (data.empty()) {
        data = getBaseFilesDir() + VM_CONFIG::RUNTIME_DATA_PATH;
        LOG_D("getDataDir success first....");
        LOG_D("dataDir: %s", data.data());
    }
    return data;
}

std::string &Util::getLibDir() {
    static std::string lib;
    if (lib.empty()) {
        lib = getBaseFilesDir() + VM_CONFIG::RUNTIME_LIB_PATH;
        LOG_D("getLibDir success first....");
        LOG_D("libDir: %s", lib.data());
    }
    return lib;
}

std::string &Util::getOdexDir() {
    static std::string odex;
    if (odex.empty()) {
        odex = getBaseFilesDir() + VM_CONFIG::RUNTIME_ODEX_PATH;
        LOG_D("getOdexDir success first....");
        LOG_D("odexDir: %s", odex.data());
    }
    return odex;
}

std::string &Util::getDexDir() {
    static std::string dex;
    if (dex.empty()) {
        dex = getBaseFilesDir() + VM_CONFIG::RUNTIME_DEX_PATH;
        LOG_D("getOdexDir success first....");
        LOG_D("odexDir: %s", dex.data());
    }
    return dex;
}

void Util::buildFileSystem() {
    LOG_I("start,  buildFileSystem");
    std::vector<std::string> path_vec = {
            getBaseFilesDir(),
            getDataDir(),
            getOdexDir(),
            getDexDir(),
            getLibDir(),
    };

    for (auto &path : path_vec) {
        if (0 != access(path.data(), R_OK) &&
            0 != mkdir(path.data(), S_IRWXU | S_IRWXG | S_IRWXO)) {// NOLINT(hicpp-signed-bitwise)
            // if this folder not exist, create a new one.
            LOG_E("create dir failed: %s", path.data());
            throw std::runtime_error("create dir failed");
        }
    }
    LOG_I("finish, buildFileSystem");
}

const uint8_t *Util::getFileBufFromAssets(const std::string &fileName, uint32_t &buf_size) {
    LOG_D("start, getFileFromAssets, fileName: %s", fileName.data());
    JNIEnv *env = VM_CONTEXT::env;
    jclass cContextWrapper = (*env).FindClass(VM_REFLECT::C_NAME_ContextWrapper);
    jmethodID mGetAssets = (*env).GetMethodID(
            cContextWrapper, VM_REFLECT::NAME_GetAssets, VM_REFLECT::SIGN_GetAssets);
    jobject oAssetsManager = (*env).CallObjectMethod(getAppContext(), mGetAssets);
    assert(oAssetsManager != nullptr);
    AAssetManager *assetManager = AAssetManager_fromJava(env, oAssetsManager);
    assert(assetManager != nullptr);
    LOG_D("Get AssetManager success...");
//    (*env).DeleteLocalRef(cContextWrapper);

    AAsset *asset = AAssetManager_open(assetManager, fileName.data(), AASSET_MODE_BUFFER);
    buf_size = AAsset_getLength(asset);
    const auto *fileBuf = reinterpret_cast<const uint8_t *>(AAsset_getBuffer(asset));
    LOG_D("srcPath: %s, fileSize: %u", fileName.data(), buf_size);
    LOG_D("finish, getFileFromAssets");
    return fileBuf;
}

jobject Util::getAppContext() {
    static jobject appContext = nullptr;
    if (appContext == nullptr) {
        JNIEnv *env = VM_CONTEXT::env;
        jclass cActivityThread = (*env).FindClass(VM_REFLECT::C_NAME_ActivityThread);
        jmethodID mCurrentActivityThread = (*env).GetStaticMethodID(
                cActivityThread, VM_REFLECT::NAME_CurrentActivityThread,
                VM_REFLECT::SIGN_CurrentActivityThread);
        jobject oCurrentActivityThread = (*env).CallStaticObjectMethod(
                cActivityThread, mCurrentActivityThread);
        assert(oCurrentActivityThread != nullptr);
        jmethodID mGetApplication = (*env).GetMethodID(
                cActivityThread, VM_REFLECT::NAME_GetApplication,
                VM_REFLECT::SIGN_GetApplication);
        jobject oCurrentApplication = (*env).CallObjectMethod(oCurrentActivityThread,
                                                              mGetApplication);
        jclass cApplication = (*env).FindClass(VM_REFLECT::C_NAME_Application);
        jmethodID mGetApplicationContext = (*env).GetMethodID(
                cApplication, VM_REFLECT::NAME_GetApplicationContext,
                VM_REFLECT::SIGN_GetApplicationContext);
        jobject oContext = (*env).CallObjectMethod(oCurrentApplication, mGetApplicationContext);
        appContext = (*env).NewGlobalRef(oContext);
        LOG_D("getAppContext success first....");
//        (*env).DeleteLocalRef(cActivityThread);
//        (*env).DeleteLocalRef(cApplication);
    }
    return appContext;
}

constexpr const char *Util::getCUP_ABI() {
#if defined(__arm__)
#if defined(__ARM_ARCH_7A__)
#if defined(__ARM_NEON__)
#if defined(__ARM_PCS_VFP)
    return "armeabi-v7a/NEON (hard-float)";
#else
    return "armeabi-v7a/NEON";
#endif
#else
#if defined(__ARM_PCS_VFP)
    return "armeabi-v7a (hard-float)";
#else
    return "armeabi-v7a";
#endif
#endif
#else
    return "armeabi";
#endif
#elif defined(__i386__)
    return "x86";
#elif defined(__x86_64__)
    return "x86_64";
#elif defined(__mips64)  /* mips64el-* toolchain defines __mips__ too */
    return "mips64";
#elif defined(__mips__)
    return "mips";
#elif defined(__aarch64__)
    return "arm64-v8a";
#else
    return "unknown";
#endif
}

jobject Util::getClassLoader() {
    static jobject oClassLoader = nullptr;
    if (oClassLoader == nullptr) {
        JNIEnv *env = VM_CONTEXT::env;
        jobject oContext = getAppContext();
        jclass cContext = (*env).GetObjectClass(oContext);
        jmethodID mGetClassLoader = (*env).GetMethodID(
                cContext, VM_REFLECT::NAME_GetClassLoader, VM_REFLECT::SIGN_GetClassLoader);
        oClassLoader = (*env).CallObjectMethod(oContext, mGetClassLoader);
        LOG_D("getClassLoader first...");
    }
    return oClassLoader;
}

bool
Util::HookNativeInline(const char *soPath, const char *signature, void *my_func, void **ori_func) {
    LOG_D("start HookNativeInline...");
    auto *lib = AKGetImageByName(soPath);
    if (lib == nullptr) {
        LOG_E("can't find so by the path: %s", soPath);
        return false;
    }
    LOG_D("lib base address: %p", AKGetBaseAddress(lib));

    auto *symbol = AKFindSymbol(lib, signature);
    if (symbol == nullptr) {
        LOG_E("can't find the symbol: %s", signature);
        AKCloseImage(lib);
        return false;
    }
    AKHookFunction(symbol, my_func, ori_func);
    AKCloseImage(lib);
    return true;
}

bool
Util::HookNativeInlineAnonymous(const char *soPath, uint64_t addr, void *my_func, void **ori_func) {
    LOG_D("start HookNativeInline...");
    auto *lib = AKGetImageByName(soPath);
    if (lib == nullptr) {
        LOG_E("can't find so by the path: %s", soPath);
        return false;
    }
    LOG_D("lib base address: %p", AKGetBaseAddress(lib));

    auto *symbol = AKFindAnonymity(lib, addr | 0x1);
    if (symbol == nullptr) {
        LOG_E("can't find the symbol at addr: 0x%016lx", addr);
        AKCloseImage(lib);
        return false;
    }
    AKHookFunction(symbol, my_func, ori_func);
    AKCloseImage(lib);
    return true;
}

bool Util::HookJava(JNIEnv *env, const char *clazzPath, const char *methodName,
                    const char *methodSignature, const void *my_func, jmethodID *ori_func) {
    LOG_D("start HookJava...");
    // init AndHook Java
    static bool initAndHookJava = true;
    if (initAndHookJava) {
        initAndHookJava = false;
        JavaVM *vm = nullptr;
        (*env).GetJavaVM(&vm);
        AKInitializeOnce(env, vm);
        LOG_D("init java hook vm...");
    }
    jclass clazz = (*env).FindClass(clazzPath);
    if (clazz == nullptr) {
        LOG_E("can't find the class by the path: %s", clazzPath);
        return false;
    }
    AKJavaHookMethod(env, clazz, methodName, methodSignature, my_func, ori_func);
//    (*env).DeleteLocalRef(clazz);
    return true;
}
