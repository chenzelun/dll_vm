
//
// Created by 陈泽伦 on 2020/12/16
//

#ifndef VM_KEYFUNCJNI_H
#define VM_KEYFUNCJNI_H

#include <jni.h>


extern "C"
JNIEXPORT void JNICALL
Java_com_dalunlun_testapp_MainActivity_onCreate__Landroid_os_Bundle_2(
        JNIEnv *env, jobject instance, jobject param_0);


extern "C"
JNIEXPORT jlong JNICALL
Java_com_dalunlun_testapp_MainActivity_sumTo__J(
        JNIEnv *env, jobject instance, jlong param_0);


extern "C"
JNIEXPORT void JNICALL
Java_com_dalunlun_testapp_MainActivity_test(
        JNIEnv *env, jobject instance);


extern "C"
JNIEXPORT jlong JNICALL
Java_com_dalunlun_testapp_MainActivity_sum__JJ(
        JNIEnv *env, jobject instance, jlong param_0, jlong param_1);

#endif //VM_KEYFUNCJNI_H

