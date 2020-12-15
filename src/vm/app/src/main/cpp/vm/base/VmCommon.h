//
// Created by 陈泽伦 on 12/7/20.
//

#ifndef VM_VMCOMMON_H
#define VM_VMCOMMON_H

#include <cstdint>
#include <string>
#include <jni.h>


//#define VM_DEBUG_FULL

#if defined(VM_DEBUG_FULL)
#define LOG_D_VM(...) LOG_D(__VA_ARGS__)
#else
#define LOG_D_VM(...)
#endif


union RegValue {
    jboolean z;
    jbyte b;
    jchar c;
    jshort s;
    jint i;
    jlong j;
    jfloat f;
    jdouble d;
    jobject l;
    uint64_t u8;
    uint32_t u4;
    uint16_t u2;
    uint8_t u1;
    int64_t s8;
    int32_t s4;
    int16_t s2;
    int8_t s1;
    jarray la;
    jclass lc;
    jobjectArray lla;
    jintArray lia;
    jcharArray lca;
    jbooleanArray lza;
    jbyteArray lba;
    jfloatArray lfa;
    jdoubleArray lda;
    jshortArray lsa;
    jlongArray lja;
    jthrowable lt;
    jmethodID lm;
};



struct VmTempData {
    uint16_t src1 = 0, src2 = 0, dst = 0;
    RegValue val_1{}, val_2{};
};


class VMException : public std::runtime_error {
public:
    VMException(const char *exp) : runtime_error(std::string("VMException: ") + exp) {}

    VMException(const std::string &exp) : runtime_error(std::string("VMException: ") + exp) {}
};



#endif //VM_VMCOMMON_H
