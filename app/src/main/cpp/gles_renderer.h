#ifndef GLES_RENDERER_H
#define GLES_RENDERER_H

#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <android/native_window.h>
#include <vector>
#include <mutex>
#include <chrono>

#include <string>

struct BenchmarkResult {
    int radiusPx = 0;
    double meanMs = 0.0;
    double medianMs = 0.0;
    double p95Ms = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;
};

class GLESRenderer {
public:
    GLESRenderer();
    ~GLESRenderer();

    bool initEGL(ANativeWindow* window);
    void destroyEGL();

    void onSurfaceChanged(int width, int height);
    
    // Decodes JPEG/PNG and uploads to GL texture natively
    double setImageData(const uint8_t* data, size_t size);
    
    // Updates blur radius (1 - 1000 px) and returns processing duration in ms
    double setBlurRadius(int radiusPx);

    // Runs automated GPU benchmark suite across resolutions & radii (1 to 1000 px) using provided sample images
    std::string runBenchmarkSuite(int iterationsPerRadius = 100);
    std::string runBenchmarkSuiteWithSamples(const std::vector<std::vector<uint8_t>>& samplePayloads, int iterationsPerRadius = 100);

    // Renders and reads back blurred image bytes (0 = JPEG, 1 = PNG)
    std::vector<uint8_t> exportImage(int format);

    void renderFrame();

private:
    void initGL();
    void setupFBOs(int width, int height);
    void releaseFBOs();
    double renderBlurPasses(bool swapDisplay = true);
    double renderBlurPassesWithRes(int imgW, int imgH, bool swapDisplay = true);
    bool ensureCurrent();
    void releaseCurrent();

    ANativeWindow* mWindow = nullptr;
    EGLDisplay mDisplay = EGL_NO_DISPLAY;
    EGLSurface mSurface = EGL_NO_SURFACE;
    EGLContext mContext = EGL_NO_CONTEXT;

    int mWindowWidth = 0;
    int mWindowHeight = 0;
    int mImageWidth = 0;
    int mImageHeight = 0;
    int mBlurRadiusPx = 10;

    GLuint mInputTexture = 0;

    // Shader Programs
    GLuint mPassthroughProgram = 0;
    GLuint mBlurHInputProgram = 0;
    GLuint mBlurHProgram = 0;
    GLuint mBlurVProgram = 0;

    // FBO Ping-Pong for Blur Passes
    GLuint mFBO0 = 0, mFBOTexture0 = 0;
    GLuint mFBO1 = 0, mFBOTexture1 = 0;
    int mFBOWidth = 0;
    int mFBOHeight = 0;

    // Quad geometry VAO/VBO
    GLuint mVAO = 0;
    GLuint mVBO = 0;

    std::mutex mMutex;
    bool mHasImage = false;
    double mLastProcessingTimeMs = 0.0;
};

#endif // GLES_RENDERER_H
