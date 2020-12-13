//
// Created by 陈泽伦 on 12/3/20.
//

#ifndef VM_JAVAEXCEPTION_H
#define VM_JAVAEXCEPTION_H


#include "base/VmMethod.h"

class JavaException {
public:
    static void throwJavaException(VmMethodContext *vmc);

    static bool handleJavaException(VmMethodContext *vmc);

    static bool checkForNull(VmMethodContext *vmc, jobject obj);

    static void throwNullPointerException(VmMethodContext *vmc, const char *msg);

    static void throwClassCastException(VmMethodContext *vmc, jclass actual, jclass desired);

    static void throwNegativeArraySizeException(VmMethodContext *vmc, s4 size);

    static void throwRuntimeException(VmMethodContext *vmc, const char *msg);

    static void throwInternalError(VmMethodContext *vmc, const char *msg);

    static void throwArrayIndexOutOfBoundsException(VmMethodContext *vmc, u4 length, u4 index);

    static void throwArithmeticException(VmMethodContext *vmc, const char *msg);

private:
    static void throwNew(VmMethodContext *vmc, const char *exceptionClassName, const char *msg);
};


#endif //VM_JAVAEXCEPTION_H
