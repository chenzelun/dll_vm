#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_dalunlun_testapp_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}