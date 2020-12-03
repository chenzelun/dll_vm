//
// Created by 陈泽伦 on 12/3/20.
//

#ifndef VM_JAVAEXCEPTION_H
#define VM_JAVAEXCEPTION_H


#include "VmMethod.h"

class JAVAException {
public:
    static void throwJavaException(VmMethodContext *vmc);

    static bool handleJavaException(VmMethodContext *vmc);

    static bool checkForNull(jobject obj);

    static void throwNullPointerException(const char *msg);

    static void throwClassCastException(jclass actual, jclass desired);

    static void throwNegativeArraySizeException(s4 size);

    static void throwRuntimeException(const char *msg);

    static void throwInternalError(const char *msg);

    static void throwArrayIndexOutOfBoundsException(u4 length, u4 index);

    static void throwArithmeticException(const char *msg);

private:
    static void throwNew(const char *exceptionClassName, const char *msg);
};


#endif //VM_JAVAEXCEPTION_H
