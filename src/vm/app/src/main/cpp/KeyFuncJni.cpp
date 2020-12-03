
//
// Created by 陈泽伦 on 2020/12/03
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
JNIEXPORT void JNICALL
Java_com_dalunlun_testapp_MainActivity_test__Ljava_lang_String_2Ljava_lang_String_2(
        JNIEnv *env, jobject instance, jobject param_0, jobject param_1) {
    
    LOG_D("jni function start.");
    jclass clazz_obj = (*env).GetObjectClass(instance);
    jmethodID method_id = (*env).GetMethodID(clazz_obj, "test", "(Ljava/lang/String;Ljava/lang/String;)V");
    jvalue retValue;
    Vm::callMethod(instance, method_id, &retValue, param_0, param_1);
    LOG_D("jni function finish.");
}


