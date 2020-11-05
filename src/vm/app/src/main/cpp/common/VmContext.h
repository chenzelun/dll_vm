//
// Created by 陈泽伦 on 2020/11/5.
//

#ifndef VM_VMCONTEXT_H
#define VM_VMCONTEXT_H

#include "VmDataFile.h"
#include <jni.h>

class VmContext {
public:
    JNIEnv *env;
    VmDataFile *vmDataFile;


public:
    static void initVmDataFileOfVmContext();


    static void updateNativeLibraryDirectories();

} VM_CONTEXT;


#endif //VM_VMCONTEXT_H
