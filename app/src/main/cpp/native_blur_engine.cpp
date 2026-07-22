#include <jni.h>
#include <android/native_window_jni.h>
#include "gles_renderer.h"

static GLESRenderer* gRenderer = nullptr;

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeInit(JNIEnv *env, jclass clazz) {
    if (!gRenderer) {
        gRenderer = new GLESRenderer();
    }
}

JNIEXPORT void JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeSurfaceCreated(JNIEnv *env, jclass clazz, jobject surface) {
    if (!gRenderer) {
        gRenderer = new GLESRenderer();
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    if (window) {
        gRenderer->initEGL(window);
    }
}

JNIEXPORT void JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeSurfaceChanged(JNIEnv *env, jclass clazz, jint width, jint height) {
    if (gRenderer) {
        gRenderer->onSurfaceChanged(width, height);
    }
}

JNIEXPORT void JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeSurfaceDestroyed(JNIEnv *env, jclass clazz) {
    if (gRenderer) {
        gRenderer->destroyEGL();
    }
}

JNIEXPORT jdouble JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeSetImageData(JNIEnv *env, jclass clazz, jbyteArray data) {
    if (!gRenderer || !data) return 0.0;
    jsize len = env->GetArrayLength(data);
    jbyte *bytes = env->GetByteArrayElements(data, nullptr);

    double duration = gRenderer->setImageData((const uint8_t*)bytes, (size_t)len);

    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
    return (jdouble)duration;
}

JNIEXPORT jdouble JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeSetBlurRadius(JNIEnv *env, jclass clazz, jint radiusPx) {
    if (!gRenderer) return 0.0;
    return (jdouble)gRenderer->setBlurRadius((int)radiusPx);
}

JNIEXPORT jbyteArray JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeExportImage(JNIEnv *env, jclass clazz, jint format) {
    if (!gRenderer) return nullptr;
    std::vector<uint8_t> bytes = gRenderer->exportImage((int)format);
    if (bytes.empty()) return nullptr;

    jbyteArray resultArray = env->NewByteArray(bytes.size());
    if (resultArray) {
        env->SetByteArrayRegion(resultArray, 0, bytes.size(), (const jbyte*)bytes.data());
    }
    return resultArray;
}

JNIEXPORT jstring JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeRunBenchmark(JNIEnv *env, jclass clazz, jint iterationsPerRadius) {
    if (!gRenderer) return env->NewStringUTF("{\"error\": \"Renderer not initialized\"}");
    std::string jsonResult = gRenderer->runBenchmarkSuite((int)iterationsPerRadius);
    return env->NewStringUTF(jsonResult.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeRunBenchmarkWithSamples(JNIEnv *env, jclass clazz, jobjectArray sampleBytesArray, jint iterationsPerRadius) {
    if (!gRenderer) return env->NewStringUTF("{\"error\": \"Renderer not initialized\"}");

    std::vector<std::vector<uint8_t>> samples;
    if (sampleBytesArray != nullptr) {
        jsize count = env->GetArrayLength(sampleBytesArray);
        for (jsize i = 0; i < count; ++i) {
            jbyteArray byteArray = (jbyteArray)env->GetObjectArrayElement(sampleBytesArray, i);
            if (byteArray != nullptr) {
                jsize len = env->GetArrayLength(byteArray);
                jbyte* bytes = env->GetByteArrayElements(byteArray, nullptr);
                samples.emplace_back((uint8_t*)bytes, (uint8_t*)bytes + len);
                env->ReleaseByteArrayElements(byteArray, bytes, JNI_ABORT);
                env->DeleteLocalRef(byteArray);
            }
        }
    }

    std::string jsonResult = gRenderer->runBenchmarkSuiteWithSamples(samples, (int)iterationsPerRadius);
    return env->NewStringUTF(jsonResult.c_str());
}

JNIEXPORT void JNICALL
Java_com_example_blurgles_NativeBlurEngine_nativeRelease(JNIEnv *env, jclass clazz) {
    if (gRenderer) {
        delete gRenderer;
        gRenderer = nullptr;
    }
}

} // extern "C"
