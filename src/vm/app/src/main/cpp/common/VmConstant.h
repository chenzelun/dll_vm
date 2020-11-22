//
// Created by 陈泽伦 on 2020/11/5.
//

#ifndef VM_VMCONSTANT_H
#define VM_VMCONSTANT_H

#define RD_STR constexpr static const char *const

struct VM_CONFIG {
    RD_STR VM_DATA_FILE_NAME = "vm_data.bin";
    RD_STR DEST_APP_DEX_FILE_NAME = "classes.dex";
    RD_STR DEST_APP_APPLICATION_NAME = "application_name";
    RD_STR VM_KEY_FUNC_CODE_FILE_NAME = "code";

    // runtime path
    RD_STR RUNTIME_LIB_PATH = "/lib";
    RD_STR RUNTIME_ODEX_PATH = "/odex";
    RD_STR RUNTIME_DEX_PATH = "/dex";
    RD_STR RUNTIME_DATA_PATH = "/data";
};

#define DEFINE_NAME_SIGN(VAR_NAME, NAME, SIGN)                                  \
    RD_STR NAME_##VAR_NAME = NAME;                                              \
    RD_STR SIGN_##VAR_NAME = SIGN;                                              \

#define DEFINE_CLASS_NAME_SIGN(VAR_NAME, CLASS_NAME)                            \
    RD_STR C_NAME_##VAR_NAME = CLASS_NAME;                                      \
    RD_STR C_SIGN_##VAR_NAME = "L" CLASS_NAME ";";                              \


struct VM_REFLECT {
    DEFINE_NAME_SIGN(GetFilesDir,
                     "getFilesDir",
                     "()Ljava/io/File;");

    DEFINE_NAME_SIGN(GetCanonicalPath,
                     "getCanonicalPath",
                     "()Ljava/lang/String;");

    DEFINE_NAME_SIGN(GetAssets,
                     "getAssets",
                     "()Landroid/content/res/AssetManager;");

    DEFINE_NAME_SIGN(CurrentActivityThread,
                     "currentActivityThread",
                     "()Landroid/app/ActivityThread;");

    DEFINE_NAME_SIGN(GetApplication,
                     "getApplication",
                     "()Landroid/app/Application;");

    DEFINE_NAME_SIGN(GetApplicationContext,
                     "getApplicationContext",
                     "()Landroid/content/Context;");

    DEFINE_NAME_SIGN(GetClassLoader,
                     "getClassLoader",
                     "()Ljava/lang/ClassLoader;");

    DEFINE_NAME_SIGN(MakeApplication,
                     "makeApplication",
                     "(ZLandroid/app/Instrumentation;)Landroid/app/Application;");

    DEFINE_NAME_SIGN(Application_onCreate,
                     "onCreate",
                     "()V");

    DEFINE_NAME_SIGN(DexClassLoader_PathList,
                     "pathList",
                     "Ldalvik/system/DexPathList;");

    DEFINE_NAME_SIGN(NativeLibraryPathElements,
                     "nativeLibraryPathElements",
                     "[Ldalvik/system/DexPathList$NativeLibraryElement;");

    DEFINE_NAME_SIGN(File_init,
                     "<init>",
                     "(Ljava/lang/String;)V");

    DEFINE_NAME_SIGN(NativeLibraryElement_init,
                     "<init>",
                     "(Ljava/io/File;)V");

    DEFINE_NAME_SIGN(DexElements,
                     "dexElements",
                     "[Ldalvik/system/DexPathList$Element;");

    DEFINE_NAME_SIGN(ByteBuffer_wrap,
                     "wrap",
                     "([B)Ljava/nio/ByteBuffer;");

    DEFINE_NAME_SIGN(DexFile_init,
                     "<init>",
                     "(Ljava/nio/ByteBuffer;)V");

    DEFINE_NAME_SIGN(DexFile_mCookie,
                     "mCookie",
                     "Ljava/lang/Object;");

    DEFINE_NAME_SIGN(DexPathList_Element_init,
                     "<init>",
                     "(Ljava/io/File;ZLjava/io/File;Ldalvik/system/DexFile;)V");

    DEFINE_NAME_SIGN(ActicityThread_mBoundApplication,
                     "mBoundApplication",
                     "Landroid/app/ActivityThread$AppBindData;");

    DEFINE_NAME_SIGN(AppBindData_info,
                     "info",
                     "Landroid/app/LoadedApk;");

    DEFINE_NAME_SIGN(LoadedApk_mApplication,
                     "mApplication",
                     "Landroid/app/Application;");

    DEFINE_NAME_SIGN(ActivityThread_mInitialApplication,
                     "mInitialApplication",
                     "Landroid/app/Application;");

    DEFINE_NAME_SIGN(ActivityThread_mAllApplications,
                     "mAllApplications",
                     "Ljava/util/ArrayList;");

    DEFINE_NAME_SIGN(ArrayList_remove,
                     "remove",
                     "(Ljava/lang/Object;)Z");

    DEFINE_NAME_SIGN(LoadedApk_mApplicationInfo,
                     "mApplicationInfo",
                     "Landroid/content/pm/ApplicationInfo;");

    DEFINE_NAME_SIGN(ApplicationInfo_className,
                     "className",
                     "Ljava/lang/String;");

    DEFINE_NAME_SIGN(AppBindData_appInfo,
                     "appInfo",
                     "Landroid/content/pm/ApplicationInfo;");

    DEFINE_NAME_SIGN(ActivityThread_mProviderMap,
                     "mProviderMap",
                     "Landroid/util/ArrayMap;");

    DEFINE_NAME_SIGN(ArrayMap_values,
                     "values",
                     "()Ljava/util/Collection;");

    DEFINE_NAME_SIGN(Collection_iterator,
                     "iterator",
                     "()Ljava/util/Iterator;");

    DEFINE_NAME_SIGN(Iterator_hasNext,
                     "hasNext",
                     "()Z");

    DEFINE_NAME_SIGN(Iterator_next,
                     "next",
                     "()Ljava/lang/Object;");

    DEFINE_NAME_SIGN(ProviderClientRecord_mLocalProvider,
                     "mLocalProvider",
                     "Landroid/content/ContentProvider;");

    DEFINE_NAME_SIGN(ContentProvider_mContext,
                     "mContext",
                     "Landroid/content/Context;");





    DEFINE_CLASS_NAME_SIGN(ContextWrapper, "android/content/ContextWrapper");
    DEFINE_CLASS_NAME_SIGN(ActivityThread, "android/app/ActivityThread");
    DEFINE_CLASS_NAME_SIGN(Application, "android/app/Application");
    DEFINE_CLASS_NAME_SIGN(NativeLibraryElement,
                           "dalvik/system/DexPathList$NativeLibraryElement");
    DEFINE_CLASS_NAME_SIGN(File, "java/io/File");
    DEFINE_CLASS_NAME_SIGN(DexFile, "dalvik/system/DexFile");
    DEFINE_CLASS_NAME_SIGN(ByteBuffer, "java/nio/ByteBuffer");
    DEFINE_CLASS_NAME_SIGN(DexPathList_Element, "dalvik/system/DexPathList$Element");
    DEFINE_CLASS_NAME_SIGN(LoadedApk, "android/app/LoadedApk");
    DEFINE_CLASS_NAME_SIGN(ApplicationInfo, "android/content/pm/ApplicationInfo");
    DEFINE_CLASS_NAME_SIGN(ArrayMap, "android/util/ArrayMap");
    DEFINE_CLASS_NAME_SIGN(ProviderClientRecord,
                           "android/app/ActivityThread$ProviderClientRecord");
    DEFINE_CLASS_NAME_SIGN(ContentProvider, "android/content/ContentProvider");


};

#endif //VM_VMCONSTANT_H
