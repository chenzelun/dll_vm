//
// Created by 陈泽伦 on 2020/11/5.
//

#ifndef VM_VMCONTEXT_H
#define VM_VMCONTEXT_H

#include "data/VmDataFile.h"
#include "data/VmKeyFuncCodeFile.h"
#include "vm/Vm.h"
#include <jni.h>

class VM_CONTEXT {
public:
    static JNIEnv *env;
    static VmDataFile *vmDataFile;
    static VmKeyFuncCodeFile* vmKFCFile;
    static Vm *vm;

public:

    static void initVmDataFileOfVC();

    static void initVmKeyFuncCodeFileOfVC();

    static void initVm();

    static void updateNativeLibraryDirectories();

    static void loadDexFromMemory();

    static void changeTopApplication();

};


#endif //VM_VMCONTEXT_H
