//
// Created by 陈泽伦 on 12/3/20.
//

#include "JavaException.h"
#include "../VmContext.h"
#include "../common/VmConstant.h"
#include "../common/Util.h"

void JavaException::throwJavaException(VmMethodContext *vmc) {
#if defined(VM_DEBUG)
    (*VM_CONTEXT::env).ExceptionDescribe();
#endif
    vmc->curException = (*VM_CONTEXT::env).ExceptionOccurred();
    assert(vmc->curException != nullptr);
    (*VM_CONTEXT::env).ExceptionClear();
}

bool JavaException::handleJavaException(VmMethodContext *vmc) {
    /*
     * We need to unroll to the catch block or the nearest "break"
     * frame.
     *
     * A break frame could indicate that we have reached an intermediate
     * native call, or have gone off the topFrame of the stack and the thread
     * needs to exit.  Either way, we return from here, leaving the
     * exception raised.
     *
     * If we do find a catch block, we want to transfer execution to
     * that point.
     *
     * Note this can cause an exception while resolving classes in
     * the "catch" blocks.
     */

    JNIEnv *env = VM_CONTEXT::env;
    const DexTry *pTry = (DexTry *) vmc->method->triesAndHandlersBuf;
    const u1 *catchHandlerList = (u1 *) (pTry + vmc->method->code->triesSize);
    const u4 pc_off = vmc->pc_cur();
    const DexTry *pBestTry = nullptr;
    u4 catchOff = -1;
    for (int i = 0; i < vmc->method->code->triesSize; ++i, pTry++) {
        if (pTry->startAddr <= pc_off && pc_off < pTry->startAddr + pTry->insnCount &&
            (pBestTry == nullptr || (pBestTry->startAddr < pTry->startAddr))) {
            const u1 *pCatchHandler = catchHandlerList + pTry->handlerOff;
            int size = readSignedLeb128(&pCatchHandler);
            // find catch
            bool catchFlag = false;
            for (int j = 0; j < abs(size); j++) {
                u4 type_idx = static_cast<u4>(readUnsignedLeb128(&pCatchHandler));
                u4 address = readUnsignedLeb128(&pCatchHandler);
                std::string catchClassName = vmc->method->dexFile->dexStringByTypeIdx(type_idx);
                if (catchClassName.size() < 3) {
                    continue;
                }
                catchClassName = catchClassName.substr(1, catchClassName.size() - 2);
                jclass catchExceptionClazz = (*env).FindClass(catchClassName.data());
                assert(catchExceptionClazz != nullptr);
                if ((*env).IsInstanceOf(vmc->curException, catchExceptionClazz)) {
                    // may catch the exception
                    pBestTry = pTry;
                    catchOff = address;
                    catchFlag = true;
                    break;
                }
            }
            if (!catchFlag && size <= 0) {
                catchOff = readUnsignedLeb128(&pCatchHandler);
            }
        }
    }

    if (catchOff == -1) {       // not found.
        LOG_D("can't handle the exception and threw it to caller.");
        return false;
    } else {
        LOG_D("handle the exception.");
        vmc->set_pc(catchOff);
        // remove the exception.
        vmc->curException = nullptr;
        return true;
    }
}

bool JavaException::checkForNull(VmMethodContext *vmc, jobject obj) {
    LOG_D("obj: %p", obj);
    if (obj == nullptr) {
        JavaException::throwNullPointerException(vmc, nullptr);
        return false;
    }
    return true;
}

void JavaException::throwNullPointerException(VmMethodContext *vmc, const char *msg) {
    JavaException::throwNew(vmc, VM_REFLECT::C_NAME_NullPointerException, msg);
}

void
JavaException::throwNew(VmMethodContext *vmc, const char *exceptionClassName, const char *msg) {
    JNIEnv *env = VM_CONTEXT::env;
    jclass clazz = (*env).FindClass(exceptionClassName);
    assert(clazz != nullptr);
    (*env).ThrowNew(clazz, msg);
    JavaException::throwJavaException(vmc);
}

void
JavaException::throwClassCastException(VmMethodContext *vmc, jclass actual, jclass desired) {
#if defined(VM_DEBUG)
    std::string msg = VmMethod::getClassDescriptorByJClass(actual);
    msg += " cannot be cast to ";
    msg += VmMethod::getClassDescriptorByJClass(desired);
#else
    std::string msg = "cannot cast class.";
#endif
    JavaException::throwNew(vmc, VM_REFLECT::C_NAME_ClassCastException, msg.data());
}

void JavaException::throwNegativeArraySizeException(VmMethodContext *vmc, s4 size) {
    char msgBuf[BUFSIZ];
    sprintf(msgBuf, "%d", size);
    JavaException::throwNew(vmc, VM_REFLECT::C_NAME_NegativeArraySizeException, msgBuf);
}

void JavaException::throwRuntimeException(VmMethodContext *vmc, const char *msg) {
    JavaException::throwNew(vmc, VM_REFLECT::C_NAME_RuntimeException, msg);
}

void JavaException::throwInternalError(VmMethodContext *vmc, const char *msg) {
    JavaException::throwNew(vmc, VM_REFLECT::C_NAME_InternalError, msg);
}

void JavaException::throwArrayIndexOutOfBoundsException(VmMethodContext *vmc, u4 length, u4 index) {
    char msgBuf[BUFSIZ];
    sprintf(msgBuf, "length=%d; index=%d", length, index);
    JavaException::throwNew(vmc, VM_REFLECT::C_NAME_ArrayIndexOutOfBoundsException, msgBuf);
}

void JavaException::throwArithmeticException(VmMethodContext *vmc, const char *msg) {
    JavaException::throwNew(vmc, VM_REFLECT::C_NAME_ArithmeticException, msg);
}