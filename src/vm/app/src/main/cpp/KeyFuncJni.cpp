
//
// Created by 陈泽伦 on 2020/11/28
//

#include "KeyFuncJni.h"
#include "common/Util.h"
#include "vm/Vm.h"


extern "C"
JNIEXPORT void JNICALL
Java_Lcom_dalunlun_testapp_MainActivity_2_onCreate__Landroid_os_Bundle_2(
        JNIEnv *env, jobject instance, jobject param_0) {
    
    LOG_D("jni function start: %s", __func__);
    jclass clazz_obj = (*env).GetObjectClass(instance);
    jmethodID method_id = (*env).GetMethodID(clazz_obj, "onCreate", "(Landroid/os/Bundle;)V");
    jvalue retValue;
    Vm::callMethod(instance, method_id, &retValue, param_0);
    LOG_D("jni function finish: %s", __func__);
}



extern "C"
JNIEXPORT void JNICALL
Java_Lcom_dalunlun_testapp_MainActivity_2_test__Ljava_lang_String_2Ljava_lang_String_2(
        JNIEnv *env, jobject instance, jobject param_0, jobject param_1) {
    
    LOG_D("jni function start: %s", __func__);
    jclass clazz_obj = (*env).GetObjectClass(instance);
    jmethodID method_id = (*env).GetMethodID(clazz_obj, "test", "(Ljava/lang/String;Ljava/lang/String;)V");
    jvalue retValue;
    Vm::callMethod(instance, method_id, &retValue, param_0, param_1);
    LOG_D("jni function finish: %s", __func__);
}


