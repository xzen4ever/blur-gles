package com.example.blurgles;

import android.view.Surface;

public class NativeBlurEngine {
    static {
        System.loadLibrary("blur_gles");
    }

    public static native void nativeInit();
    public static native void nativeSurfaceCreated(Surface surface);
    public static native void nativeSurfaceChanged(int width, int height);
    public static native void nativeSurfaceDestroyed();
    public static native double nativeSetImageData(byte[] data);
    public static native double nativeSetBlurRadius(int radiusPx);
    public static native byte[] nativeExportImage(int format);
    public static native String nativeRunBenchmark(int iterationsPerRadius);
    public static native String nativeRunBenchmarkWithSamples(byte[][] sampleBytesArray, int iterationsPerRadius);
    public static native void nativeRelease();
}
