#include <jni.h>
#include <string>
#include <android/log.h>

#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG,__FUNCTION__,__VA_ARGS__) // 定义LOGD类型
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR,__FUNCTION__,__VA_ARGS__) // 定义LOGE类型


extern "C" JNIEXPORT jstring JNICALL
Java_com_dalunlun_testapp_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject obj /* this */) {
    std::string hello = "Hello from C++";
    LOG_D("dalunlun: %p", obj);

    jvalue val[2];
    val[0].l = (*env).NewStringUTF("hello");
    val[1].l = (*env).NewStringUTF("world");
    jclass jclass1 = (*env).GetObjectClass(obj);
    jmethodID jmethodId = (*env).GetMethodID(jclass1,
            "test", "(Ljava/lang/String;Ljava/lang/String;)V");
    (*env).CallVoidMethodA(obj, jmethodId, val);

    return env->NewStringUTF(hello.c_str());
}