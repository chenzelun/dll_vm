
//
// Created by 陈泽伦 on 2020/12/16
//

#include "KeyFuncJni.h"
#include "common/Util.h"
#include "vm/Vm.h"


extern "C"
JNIEXPORT void JNICALL
Java_com_dalunlun_testapp_MainActivity_onCreate__Landroid_os_Bundle_2(
        JNIEnv *env, jobject instance, jobject param_0) {
    
    LOG_D("jni function start.");
    jclass clazz_obj = (*env).GetObjectClass(instance);
    jmethodID method_id = (*env).GetMethodID(clazz_obj, "onCreate", "(Landroid/os/Bundle;)V");
    jvalue retValue;
    Vm::callMethod(instance, method_id, &retValue, param_0);
    LOG_D("jni function finish.");
}



extern "C"
JNIEXPORT jlong JNICALL
Java_com_dalunlun_testapp_MainActivity_sumTo__J(
        JNIEnv *env, jobject instance, jlong param_0) {
    
    LOG_D("jni function start.");
    jclass clazz_obj = (*env).GetObjectClass(instance);
    jmethodID method_id = (*env).GetMethodID(clazz_obj, "sumTo", "(J)J");
    jvalue retValue;
    Vm::callMethod(instance, method_id, &retValue, param_0);
    LOG_D("jni function finish.");
    return retValue.j;
}



extern "C"
JNIEXPORT void JNICALL
Java_com_dalunlun_testapp_MainActivity_test(
        JNIEnv *env, jobject instance) {
    
    LOG_D("jni function start.");
    jclass clazz_obj = (*env).GetObjectClass(instance);
    jmethodID method_id = (*env).GetMethodID(clazz_obj, "test", "()V");
    jvalue retValue;
    Vm::callMethod(instance, method_id, &retValue);
    LOG_D("jni function finish.");
}



extern "C"
JNIEXPORT jlong JNICALL
Java_com_dalunlun_testapp_MainActivity_sum__JJ(
        JNIEnv *env, jobject instance, jlong param_0, jlong param_1) {
    
    LOG_D("jni function start.");
    jclass clazz_obj = (*env).GetObjectClass(instance);
    jmethodID method_id = (*env).GetMethodID(clazz_obj, "sum", "(JJ)J");
    jvalue retValue;
    Vm::callMethod(instance, method_id, &retValue, param_0, param_1);
    LOG_D("jni function finish.");
    return retValue.j;
}


