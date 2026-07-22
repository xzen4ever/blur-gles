#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#include "gles_renderer.h"
#include "shader_utils.h"
#include <android/imagedecoder.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <numeric>
#include <iomanip>

static const char* VERTEX_SHADER_SRC = R"(#version 320 es
layout(location = 0) in vec2 aPosition;
out vec2 vTexCoord;
void main() {
    vTexCoord = aPosition * 0.5 + 0.5;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)";

static const char* PASSTHROUGH_FRAGMENT_SHADER_SRC = R"(#version 320 es
precision mediump float;
in vec2 vTexCoord;
uniform sampler2D uTexture;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTexCoord);
}
)";

static const char* GAUSSIAN_BLUR_INPUT_FRAGMENT_SHADER_SRC = R"(#version 320 es
precision highp float;
in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec2 uDirection;
uniform float uRadius;
out vec4 fragColor;

void main() {
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    int samples = int(clamp(uRadius, 1.0, 24.0));
    float sigma = max(uRadius * 0.4, 1.0);
    vec2 flippedCoord = vec2(vTexCoord.x, 1.0 - vTexCoord.y);

    for (int i = -samples; i <= samples; ++i) {
        float offset = float(i);
        float weight = exp(-0.5 * (offset * offset) / (sigma * sigma));
        color += texture(uTexture, flippedCoord + uDirection * offset) * weight;
        totalWeight += weight;
    }
    fragColor = color / totalWeight;
}
)";

static const char* GAUSSIAN_BLUR_FRAGMENT_SHADER_SRC = R"(#version 320 es
precision highp float;
in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec2 uDirection;
uniform float uRadius;
out vec4 fragColor;

void main() {
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    int samples = int(clamp(uRadius, 1.0, 24.0));
    float sigma = max(uRadius * 0.4, 1.0);

    for (int i = -samples; i <= samples; ++i) {
        float offset = float(i);
        float weight = exp(-0.5 * (offset * offset) / (sigma * sigma));
        color += texture(uTexture, vTexCoord + uDirection * offset) * weight;
        totalWeight += weight;
    }
    fragColor = color / totalWeight;
}
)";

GLESRenderer::GLESRenderer() {}

GLESRenderer::~GLESRenderer() {
    destroyEGL();
}

void GLESRenderer::releaseCurrent() {
    if (mDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}

bool GLESRenderer::ensureCurrent() {
    if (mDisplay == EGL_NO_DISPLAY || mContext == EGL_NO_CONTEXT) {
        LOGE("ensureCurrent failed: EGL display or context is null");
        return false;
    }
    if (eglGetCurrentContext() != mContext) {
        if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
            EGLint err = eglGetError();
            LOGW("eglMakeCurrent direct switch failed (0x%x), releasing previous thread context...", err);
            eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
                LOGE("ensureCurrent failed to eglMakeCurrent: 0x%x", eglGetError());
                return false;
            }
        }
    }
    return true;
}

bool GLESRenderer::initEGL(ANativeWindow* window) {
    std::lock_guard<std::mutex> lock(mMutex);
    mWindow = window;
    if (!mWindow) return false;

    mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }

    if (!eglInitialize(mDisplay, nullptr, nullptr)) {
        LOGE("eglInitialize failed");
        return false;
    }

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(mDisplay, attribs, &config, 1, &numConfigs) || numConfigs <= 0) {
        LOGE("eglChooseConfig failed");
        return false;
    }

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    mContext = eglCreateContext(mDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (mContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext GLES 3.2 failed");
        return false;
    }

    mSurface = eglCreateWindowSurface(mDisplay, config, mWindow, nullptr);
    if (mSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return false;
    }

    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
        LOGE("eglMakeCurrent failed");
        return false;
    }

    mWindowWidth = ANativeWindow_getWidth(mWindow);
    mWindowHeight = ANativeWindow_getHeight(mWindow);

    initGL();
    releaseCurrent();
    LOGI("EGL Initialized successfully for GLES 3.2 (%dx%d)", mWindowWidth, mWindowHeight);
    return true;
}

void GLESRenderer::initGL() {
    mPassthroughProgram = ShaderUtils::createProgram(VERTEX_SHADER_SRC, PASSTHROUGH_FRAGMENT_SHADER_SRC);
    mBlurHInputProgram = ShaderUtils::createProgram(VERTEX_SHADER_SRC, GAUSSIAN_BLUR_INPUT_FRAGMENT_SHADER_SRC);
    mBlurHProgram = ShaderUtils::createProgram(VERTEX_SHADER_SRC, GAUSSIAN_BLUR_FRAGMENT_SHADER_SRC);
    mBlurVProgram = ShaderUtils::createProgram(VERTEX_SHADER_SRC, GAUSSIAN_BLUR_FRAGMENT_SHADER_SRC);

    // Quad geometry (-1..1)
    GLfloat quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTextures(1, &mInputTexture);
    glBindTexture(GL_TEXTURE_2D, mInputTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLESRenderer::setupFBOs(int width, int height) {
    if (mFBOWidth == width && mFBOHeight == height && mFBO0 != 0) return;

    releaseFBOs();
    mFBOWidth = width;
    mFBOHeight = height;

    auto createFBO = [width, height](GLuint& fbo, GLuint& tex) {
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &tex);

        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("FBO Creation failed status");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };

    createFBO(mFBO0, mFBOTexture0);
    createFBO(mFBO1, mFBOTexture1);
}

void GLESRenderer::releaseFBOs() {
    if (mFBO0) { glDeleteFramebuffers(1, &mFBO0); mFBO0 = 0; }
    if (mFBOTexture0) { glDeleteTextures(1, &mFBOTexture0); mFBOTexture0 = 0; }
    if (mFBO1) { glDeleteFramebuffers(1, &mFBO1); mFBO1 = 0; }
    if (mFBOTexture1) { glDeleteTextures(1, &mFBOTexture1); mFBOTexture1 = 0; }
    mFBOWidth = 0;
    mFBOHeight = 0;
}

void GLESRenderer::destroyEGL() {
    std::lock_guard<std::mutex> lock(mMutex);
    releaseFBOs();
    if (mVAO) { glDeleteVertexArrays(1, &mVAO); mVAO = 0; }
    if (mVBO) { glDeleteBuffers(1, &mVBO); mVBO = 0; }
    if (mInputTexture) { glDeleteTextures(1, &mInputTexture); mInputTexture = 0; }
    if (mPassthroughProgram) { glDeleteProgram(mPassthroughProgram); mPassthroughProgram = 0; }
    if (mBlurHInputProgram) { glDeleteProgram(mBlurHInputProgram); mBlurHInputProgram = 0; }
    if (mBlurHProgram) { glDeleteProgram(mBlurHProgram); mBlurHProgram = 0; }
    if (mBlurVProgram) { glDeleteProgram(mBlurVProgram); mBlurVProgram = 0; }

    if (mDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (mSurface != EGL_NO_SURFACE) { eglDestroySurface(mDisplay, mSurface); mSurface = EGL_NO_SURFACE; }
        if (mContext != EGL_NO_CONTEXT) { eglDestroyContext(mDisplay, mContext); mContext = EGL_NO_CONTEXT; }
        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
    }
    mWindow = nullptr;
}

void GLESRenderer::onSurfaceChanged(int width, int height) {
    std::lock_guard<std::mutex> lock(mMutex);
    mWindowWidth = width;
    mWindowHeight = height;
    if (mHasImage) {
        renderBlurPasses();
        releaseCurrent();
    }
}

double GLESRenderer::setImageData(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!ensureCurrent()) return 0.0;

    int w = 0, h = 0, comp = 0;
    uint8_t* pixels = nullptr;

    // Use AImageDecoder from Android NDK (API 30+)
    AImageDecoder* decoder = nullptr;
    int result = AImageDecoder_createFromBuffer(data, size, &decoder);
    if (result == ANDROID_IMAGE_DECODER_SUCCESS && decoder) {
        AImageDecoder_setAndroidBitmapFormat(decoder, ANDROID_BITMAP_FORMAT_RGBA_8888);
        const AImageDecoderHeaderInfo* info = AImageDecoder_getHeaderInfo(decoder);
        if (info) {
            w = AImageDecoderHeaderInfo_getWidth(info);
            h = AImageDecoderHeaderInfo_getHeight(info);
            size_t minStride = AImageDecoder_getMinimumStride(decoder);
            size_t bufSize = minStride * h;
            pixels = (uint8_t*) malloc(bufSize);
            if (pixels) {
                int decodeRes = AImageDecoder_decodeImage(decoder, pixels, minStride, bufSize);
                if (decodeRes != ANDROID_IMAGE_DECODER_SUCCESS) {
                    LOGE("AImageDecoder_decodeImage failed: %d", decodeRes);
                    free(pixels);
                    pixels = nullptr;
                } else if (minStride != (size_t)w * 4) {
                    uint8_t* tightlyPacked = (uint8_t*) malloc((size_t)w * h * 4);
                    for (int r = 0; r < h; ++r) {
                        memcpy(tightlyPacked + r * w * 4, pixels + r * minStride, w * 4);
                    }
                    free(pixels);
                    pixels = tightlyPacked;
                }
            }
        }
        AImageDecoder_delete(decoder);
    }

    if (!pixels) {
        // STB Fallback
        pixels = stbi_load_from_memory(data, (int)size, &w, &h, &comp, 4);
    }

    if (!pixels || w <= 0 || h <= 0) {
        LOGE("Failed to decode JPEG/PNG image data natively");
        if (pixels) free(pixels);
        mHasImage = false;
        return 0.0;
    }

    mImageWidth = w;
    mImageHeight = h;

    glBindTexture(GL_TEXTURE_2D, mInputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(pixels);
    mHasImage = true;
    LOGI("Natively decoded JPEG/PNG texture loaded: %dx%d", w, h);

    double duration = renderBlurPasses();
    releaseCurrent();
    return duration;
}

double GLESRenderer::setBlurRadius(int radiusPx) {
    std::lock_guard<std::mutex> lock(mMutex);
    mBlurRadiusPx = std::max(1, std::min(1000, radiusPx));
    if (mHasImage) {
        double duration = renderBlurPasses();
        releaseCurrent();
        return duration;
    }
    return 0.0;
}

double GLESRenderer::renderBlurPassesWithRes(int imgW, int imgH, bool swapDisplay) {
    if (imgW <= 0 || imgH <= 0) return 0.0;
    if (!ensureCurrent()) return 0.0;

    auto start = std::chrono::high_resolution_clock::now();

    // Scale resolution for downsampled ping-pong FBO if blur radius is high
    float scale = 1.0f;
    if (mBlurRadiusPx > 10) {
        scale = 1.0f + (mBlurRadiusPx - 10) / 15.0f;
    }
    int targetFBOWidth = std::max(1, (int)(imgW / scale));
    int targetFBOHeight = std::max(1, (int)(imgH / scale));

    setupFBOs(targetFBOWidth, targetFBOHeight);

    float effectiveRadius = std::max(1.0f, mBlurRadiusPx / scale);

    glBindVertexArray(mVAO);

    // Pass 1: Horizontal Blur & Y-Flip (InputTexture -> FBO 0)
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO0);
    glViewport(0, 0, targetFBOWidth, targetFBOHeight);
    glUseProgram(mBlurHInputProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mInputTexture);
    glUniform1i(glGetUniformLocation(mBlurHInputProgram, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(mBlurHInputProgram, "uDirection"), 1.0f / targetFBOWidth, 0.0f);
    glUniform1f(glGetUniformLocation(mBlurHInputProgram, "uRadius"), effectiveRadius);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Pass 2: Vertical Blur (FBO 0 Texture -> FBO 1)
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO1);
    glViewport(0, 0, targetFBOWidth, targetFBOHeight);
    glUseProgram(mBlurVProgram);
    glBindTexture(GL_TEXTURE_2D, mFBOTexture0);
    glUniform1i(glGetUniformLocation(mBlurVProgram, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(mBlurVProgram, "uDirection"), 0.0f, 1.0f / targetFBOHeight);
    glUniform1f(glGetUniformLocation(mBlurVProgram, "uRadius"), effectiveRadius);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Pass 3: Render to Preview Window Surface with aspect ratio fitting
    if (swapDisplay && mSurface != EGL_NO_SURFACE && mWindowWidth > 0 && mWindowHeight > 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, mWindowWidth, mWindowHeight);
        glClearColor(0.0706f, 0.0706f, 0.0706f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float imageAspect = (float)imgW / (float)imgH;
        float windowAspect = (float)mWindowWidth / (float)mWindowHeight;

        int vpX = 0, vpY = 0, vpW = mWindowWidth, vpH = mWindowHeight;

        if (imageAspect > windowAspect) {
            vpW = mWindowWidth;
            vpH = (int)(mWindowWidth / imageAspect);
            vpX = 0;
            vpY = (mWindowHeight - vpH) / 2;
        } else {
            vpH = mWindowHeight;
            vpW = (int)(mWindowHeight * imageAspect);
            vpX = (mWindowWidth - vpW) / 2;
            vpY = 0;
        }

        glViewport(vpX, vpY, vpW, vpH);
        glUseProgram(mPassthroughProgram);
        glBindTexture(GL_TEXTURE_2D, mFBOTexture1);
        glUniform1i(glGetUniformLocation(mPassthroughProgram, "uTexture"), 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        eglSwapBuffers(mDisplay, mSurface);
        glFinish();
    } else {
        glFlush();
    }

    auto end = std::chrono::high_resolution_clock::now();
    mLastProcessingTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    return mLastProcessingTimeMs;
}

double GLESRenderer::renderBlurPasses(bool swapDisplay) {
    if (!mHasImage || mImageWidth <= 0 || mImageHeight <= 0) return 0.0;
    return renderBlurPassesWithRes(mImageWidth, mImageHeight, swapDisplay);
}

std::string GLESRenderer::runBenchmarkSuiteWithSamples(const std::vector<std::vector<uint8_t>>& samplePayloads, int iterationsPerRadius) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!ensureCurrent()) {
        return "{\"error\": \"EGL context unavailable\"}";
    }

    int originalRadius = mBlurRadiusPx;
    int originalW = mImageWidth;
    int originalH = mImageHeight;
    bool originalHasImage = mHasImage;

    std::vector<int> targetRadii = {1, 5, 10, 25, 50, 100, 250, 500, 1000};
    int totalIterations = 100;

    std::ostringstream json;
    json << "{\n";
    json << "  \"iterationsPerRadius\": " << totalIterations << ",\n";
    json << "  \"resolutions\": [\n";

    if (!samplePayloads.empty()) {
        for (size_t sIdx = 0; sIdx < samplePayloads.size(); ++sIdx) {
            const auto& payload = samplePayloads[sIdx];
            if (payload.empty()) continue;

            int w = 0, h = 0, comp = 0;
            uint8_t* pixels = stbi_load_from_memory(payload.data(), (int)payload.size(), &w, &h, &comp, 4);
            if (!pixels || w <= 0 || h <= 0) {
                LOGE("Failed to decode benchmark sample asset index %d", (int)sIdx);
                if (pixels) free(pixels);
                continue;
            }

            glBindTexture(GL_TEXTURE_2D, mInputTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
            free(pixels);

            mImageWidth = w;
            mImageHeight = h;
            mHasImage = true;

            std::string label;
            if (w >= 3000) label = "4K UHD (" + std::to_string(w) + "x" + std::to_string(h) + ")";
            else if (w >= 1800) label = "Full HD (" + std::to_string(w) + "x" + std::to_string(h) + ")";
            else label = "HD (" + std::to_string(w) + "x" + std::to_string(h) + ")";

            json << "    {\n";
            json << "      \"label\": \"" << label << "\",\n";
            json << "      \"width\": " << w << ",\n";
            json << "      \"height\": " << h << ",\n";
            json << "      \"results\": [\n";

            for (size_t rIdx = 0; rIdx < targetRadii.size(); ++rIdx) {
                int r = targetRadii[rIdx];
                mBlurRadiusPx = r;

                // Warm-up pass (10 frames)
                for (int k = 0; k < 10; ++k) {
                    renderBlurPassesWithRes(w, h, false);
                }

                std::vector<double> timings;
                timings.reserve(totalIterations);

                for (int i = 0; i < totalIterations; ++i) {
                    glFinish();
                    auto t0 = std::chrono::high_resolution_clock::now();
                    renderBlurPassesWithRes(w, h, false);
                    glFinish();
                    auto t1 = std::chrono::high_resolution_clock::now();
                    double elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
                    timings.push_back(elapsedMs);
                }

                std::sort(timings.begin(), timings.end());
                double minMs = timings.front();
                double maxMs = timings.back();
                double sum = std::accumulate(timings.begin(), timings.end(), 0.0);
                double meanMs = sum / timings.size();
                size_t p95Index = std::min(timings.size() - 1, (size_t)(timings.size() * 0.95));
                double p95Ms = timings[p95Index];
                double fps = (meanMs > 0.0) ? (1000.0 / meanMs) : 0.0;

                json << std::fixed << std::setprecision(3);
                json << "        {\n";
                json << "          \"radiusPx\": " << r << ",\n";
                json << "          \"meanMs\": " << meanMs << ",\n";
                json << "          \"p95Ms\": " << p95Ms << ",\n";
                json << "          \"minMs\": " << minMs << ",\n";
                json << "          \"maxMs\": " << maxMs << ",\n";
                json << "          \"fps\": " << fps << "\n";
                json << "        }" << (rIdx + 1 < targetRadii.size() ? "," : "") << "\n";
            }

            json << "      ]\n";
            json << "    }" << (sIdx + 1 < samplePayloads.size() ? "," : "") << "\n";
        }
    } else {
        struct TargetResolution {
            std::string label;
            int width;
            int height;
        };
        std::vector<TargetResolution> targetResList = {
            {"HD (1280x720)", 1280, 720},
            {"Full HD (1920x1080)", 1920, 1080},
            {"4K UHD (3840x2160)", 3840, 2160}
        };
        for (size_t resIdx = 0; resIdx < targetResList.size(); ++resIdx) {
            const auto& res = targetResList[resIdx];
            json << "    {\n";
            json << "      \"label\": \"" << res.label << "\",\n";
            json << "      \"width\": " << res.width << ",\n";
            json << "      \"height\": " << res.height << ",\n";
            json << "      \"results\": [\n";

            for (size_t rIdx = 0; rIdx < targetRadii.size(); ++rIdx) {
                int r = targetRadii[rIdx];
                mBlurRadiusPx = r;

                for (int k = 0; k < 10; ++k) {
                    renderBlurPassesWithRes(res.width, res.height, false);
                }

                std::vector<double> timings;
                timings.reserve(totalIterations);

                for (int i = 0; i < totalIterations; ++i) {
                    glFinish();
                    auto t0 = std::chrono::high_resolution_clock::now();
                    renderBlurPassesWithRes(res.width, res.height, false);
                    glFinish();
                    auto t1 = std::chrono::high_resolution_clock::now();
                    double elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
                    timings.push_back(elapsedMs);
                }

                std::sort(timings.begin(), timings.end());
                double minMs = timings.front();
                double maxMs = timings.back();
                double sum = std::accumulate(timings.begin(), timings.end(), 0.0);
                double meanMs = sum / timings.size();
                size_t p95Index = std::min(timings.size() - 1, (size_t)(timings.size() * 0.95));
                double p95Ms = timings[p95Index];
                double fps = (meanMs > 0.0) ? (1000.0 / meanMs) : 0.0;

                json << std::fixed << std::setprecision(3);
                json << "        {\n";
                json << "          \"radiusPx\": " << r << ",\n";
                json << "          \"meanMs\": " << meanMs << ",\n";
                json << "          \"p95Ms\": " << p95Ms << ",\n";
                json << "          \"minMs\": " << minMs << ",\n";
                json << "          \"maxMs\": " << maxMs << ",\n";
                json << "          \"fps\": " << fps << "\n";
                json << "        }" << (rIdx + 1 < targetRadii.size() ? "," : "") << "\n";
            }

            json << "      ]\n";
            json << "    }" << (resIdx + 1 < targetResList.size() ? "," : "") << "\n";
        }
    }

    // Restore original state
    mBlurRadiusPx = originalRadius;
    mImageWidth = originalW;
    mImageHeight = originalH;
    mHasImage = originalHasImage;

    if (mHasImage) {
        renderBlurPasses(true);
    }
    releaseCurrent();

    json << "  ]\n";
    json << "}";

    return json.str();
}

std::string GLESRenderer::runBenchmarkSuite(int iterationsPerRadius) {
    return runBenchmarkSuiteWithSamples({}, iterationsPerRadius);
}

void GLESRenderer::renderFrame() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mHasImage) {
        renderBlurPasses();
        releaseCurrent();
    }
}

static void writeFunc(void *context, void *data, int size) {
    std::vector<uint8_t>* vec = (std::vector<uint8_t>*)context;
    uint8_t* p = (uint8_t*)data;
    vec->insert(vec->end(), p, p + size);
}

std::vector<uint8_t> GLESRenderer::exportImage(int format) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<uint8_t> result;
    if (!mHasImage || mImageWidth <= 0 || mImageHeight <= 0) return result;
    if (!ensureCurrent()) return result;

    // Scale resolution for downsampled ping-pong FBO matching preview exactly
    float scale = 1.0f;
    if (mBlurRadiusPx > 10) {
        scale = 1.0f + (mBlurRadiusPx - 10) / 15.0f;
    }
    int targetFBOWidth = std::max(1, (int)(mImageWidth / scale));
    int targetFBOHeight = std::max(1, (int)(mImageHeight / scale));
    float effectiveRadius = std::max(1.0f, mBlurRadiusPx / scale);

    // Create intermediate FBOs for blur passes at target FBOWidth/FBOHeight
    GLuint expFBO0 = 0, expTex0 = 0;
    GLuint expFBO1 = 0, expTex1 = 0;

    // Create final full-resolution output FBO (mImageWidth x mImageHeight)
    GLuint expOutFBO = 0, expOutTex = 0;

    auto createFBO = [](GLuint& fbo, GLuint& tex, int w, int h) {
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &tex);

        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };

    createFBO(expFBO0, expTex0, targetFBOWidth, targetFBOHeight);
    createFBO(expFBO1, expTex1, targetFBOWidth, targetFBOHeight);
    createFBO(expOutFBO, expOutTex, mImageWidth, mImageHeight);

    glBindVertexArray(mVAO);

    // Pass 1: Horizontal Blur & Y-Flip (InputTexture -> expFBO0)
    glBindFramebuffer(GL_FRAMEBUFFER, expFBO0);
    glViewport(0, 0, targetFBOWidth, targetFBOHeight);
    glUseProgram(mBlurHInputProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mInputTexture);
    glUniform1i(glGetUniformLocation(mBlurHInputProgram, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(mBlurHInputProgram, "uDirection"), 1.0f / targetFBOWidth, 0.0f);
    glUniform1f(glGetUniformLocation(mBlurHInputProgram, "uRadius"), effectiveRadius);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Pass 2: Vertical Blur (expTex0 -> expFBO1)
    glBindFramebuffer(GL_FRAMEBUFFER, expFBO1);
    glViewport(0, 0, targetFBOWidth, targetFBOHeight);
    glUseProgram(mBlurVProgram);
    glBindTexture(GL_TEXTURE_2D, expTex0);
    glUniform1i(glGetUniformLocation(mBlurVProgram, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(mBlurVProgram, "uDirection"), 0.0f, 1.0f / targetFBOHeight);
    glUniform1f(glGetUniformLocation(mBlurVProgram, "uRadius"), effectiveRadius);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Pass 3: Upscale blurred image to full resolution (expTex1 -> expOutFBO)
    glBindFramebuffer(GL_FRAMEBUFFER, expOutFBO);
    glViewport(0, 0, mImageWidth, mImageHeight);
    glUseProgram(mPassthroughProgram);
    glBindTexture(GL_TEXTURE_2D, expTex1);
    glUniform1i(glGetUniformLocation(mPassthroughProgram, "uTexture"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Read back pixel data from expOutFBO
    std::vector<uint8_t> rawPixels((size_t)mImageWidth * mImageHeight * 4);
    glReadPixels(0, 0, mImageWidth, mImageHeight, GL_RGBA, GL_UNSIGNED_BYTE, rawPixels.data());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &expFBO0);
    glDeleteTextures(1, &expTex0);
    glDeleteFramebuffers(1, &expFBO1);
    glDeleteTextures(1, &expTex1);
    glDeleteFramebuffers(1, &expOutFBO);
    glDeleteTextures(1, &expOutTex);

    // glReadPixels reads bottom-to-top, so flip rows vertically before STB write
    std::vector<uint8_t> flippedPixels((size_t)mImageWidth * mImageHeight * 4);
    size_t stride = (size_t)mImageWidth * 4;
    for (int r = 0; r < mImageHeight; ++r) {
        memcpy(flippedPixels.data() + (mImageHeight - 1 - r) * stride,
               rawPixels.data() + r * stride, stride);
    }

    // Encode to PNG or JPEG using stb_image_write
    if (format == 1) { // PNG
        stbi_write_png_to_func(writeFunc, &result, mImageWidth, mImageHeight, 4, flippedPixels.data(), (int)stride);
    } else { // JPEG
        stbi_write_jpg_to_func(writeFunc, &result, mImageWidth, mImageHeight, 4, flippedPixels.data(), 90);
    }

    LOGI("Exported image: %d bytes (format=%d, size=%dx%d, radius=%d)", (int)result.size(), format, mImageWidth, mImageHeight, mBlurRadiusPx);
    releaseCurrent();
    return result;
}

