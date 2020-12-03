//
// Created by 陈泽伦 on 11/17/20.
//

#ifndef VM_VM_H
#define VM_VM_H

#include <jni.h>
#include <string>
#include "VmMethod.h"
#include "../Interpret/Interpret.h"

class VMException : public std::runtime_error {
public:
    VMException(const char *exp) : runtime_error(std::string("VMException: ") + exp) {}

    VMException(const std::string &exp) : runtime_error(std::string("VMException: ") + exp) {}
};

#define  PRIMITIVE_TYPE_SIZE 8

class Vm {
private:
    Interpret *interpret;

public:
    static void
    callMethod(jobject instance, jmethodID method, jvalue *pResult, ...);

    Vm();

    void setInterpret(Interpret *pInterpret);

    void run(VmMethodContext *vmc) const;

    jclass findPrimitiveClass(const char type) const;


private:
    const char primitiveType[PRIMITIVE_TYPE_SIZE] = {
//            'V',
            'B',
            'Z',
            'I',
            'S',
            'C',
            'F',
            'D',
            'J'
    };
    jclass primitiveClass[PRIMITIVE_TYPE_SIZE]{};
private:
    void initPrimitiveClass();

};


#endif //VM_VM_H
