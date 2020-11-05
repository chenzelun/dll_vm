//
// Created by 陈泽伦 on 2020/11/5.
//

#ifndef VM_VMCONSTANT_H
#define VM_VMCONSTANT_H

#define RD_STR constexpr static const char *const

struct VM_CONFIG {
    RD_STR VM_DATA_FILE_NAME = "vm_data.bin";

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



    DEFINE_CLASS_NAME_SIGN(ContextWrapper, "android/content/ContextWrapper");
    DEFINE_CLASS_NAME_SIGN(ActivityThread, "android/app/ActivityThread");
    DEFINE_CLASS_NAME_SIGN(Application, "android/app/Application");
    DEFINE_CLASS_NAME_SIGN(NativeLibraryElement,
                           "dalvik/system/DexPathList$NativeLibraryElement");
    DEFINE_CLASS_NAME_SIGN(File, "java/io/File");

};

#endif //VM_VMCONSTANT_H
