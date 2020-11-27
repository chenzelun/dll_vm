
//
// Created by 陈泽伦 on 2020/11/28
//

#ifndef VM_KEYFUNCJNI_H
#define VM_KEYFUNCJNI_H

#include <jni.h>


extern "C"
JNIEXPORT void JNICALL
Java_Lcom_dalunlun_testapp_MainActivity_2_onCreate__Landroid_os_Bundle_2(
        JNIEnv *env, jobject instance, jobject param_0);


extern "C"
JNIEXPORT void JNICALL
Java_Lcom_dalunlun_testapp_MainActivity_2_test__Ljava_lang_String_2Ljava_lang_String_2(
        JNIEnv *env, jobject instance, jobject param_0, jobject param_1);

#endif //VM_KEYFUNCJNI_H

