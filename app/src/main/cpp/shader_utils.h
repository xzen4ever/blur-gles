#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include <GLES3/gl32.h>
#include <android/log.h>

#define LOG_TAG "BlurGLES_Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class ShaderUtils {
public:
    static GLuint compileShader(GLenum type, const char* source);
    static GLuint createProgram(const char* vertexSource, const char* fragmentSource);
};

#endif // SHADER_UTILS_H
