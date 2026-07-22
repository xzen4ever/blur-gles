package com.example.blurgles;

import android.content.ContentValues;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore;
import android.view.View;
import android.view.SurfaceHolder;
import android.widget.SeekBar;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.PickVisualMediaRequest;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import com.example.blurgles.databinding.ActivityMainBinding;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private ActivityMainBinding binding;

    private double slowestMs = -1.0;
    private double fastestMs = -1.0;
    private boolean hasLoadedImage = false;

    private ActivityResultLauncher<PickVisualMediaRequest> imagePickerLauncher;
    private final ExecutorService executorService = Executors.newSingleThreadExecutor();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        EdgeToEdge.enable(this);
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        boolean isNightMode = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
        WindowInsetsControllerCompat insetsController = WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView());
        insetsController.setAppearanceLightStatusBars(!isNightMode);
        insetsController.setAppearanceLightNavigationBars(!isNightMode);

        ViewCompat.setOnApplyWindowInsetsListener(binding.getRoot(), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        NativeBlurEngine.nativeInit();

        binding.surfaceView.getHolder().addCallback(this);

        imagePickerLauncher = registerForActivityResult(
                new ActivityResultContracts.PickVisualMedia(),
                uri -> {
                    if (uri != null) {
                        loadImageFromUri(uri);
                    }
                }
        );

        binding.btnLoadImage.setOnClickListener(v -> imagePickerLauncher.launch(
                new PickVisualMediaRequest.Builder()
                        .setMediaType(ActivityResultContracts.PickVisualMedia.ImageOnly.INSTANCE)
                        .build()
        ));

        binding.btnSaveImage.setOnClickListener(v -> saveBlurredImage());

        binding.btnRunBenchmark.setOnClickListener(v -> runBenchmark());

        binding.seekbarRadius.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int radius = Math.max(1, progress);
                binding.tvBlurRadius.setText("Blur Radius: " + radius + " px");
                double durationMs = NativeBlurEngine.nativeSetBlurRadius(radius);
                updateProcessingTime(durationMs);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
    }

    private void loadImageFromUri(Uri uri) {
        try (InputStream inputStream = getContentResolver().openInputStream(uri);
             ByteArrayOutputStream byteBuffer = new ByteArrayOutputStream()) {

            if (inputStream == null) return;
            byte[] buffer = new byte[16384];
            int len;
            while ((len = inputStream.read(buffer)) != -1) {
                byteBuffer.write(buffer, 0, len);
            }
            byte[] bytes = byteBuffer.toByteArray();

            double durationMs = NativeBlurEngine.nativeSetImageData(bytes);
            hasLoadedImage = durationMs >= 0.0;
            updateProcessingTime(durationMs);
            Toast.makeText(this, "Image loaded natively", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(this, "Failed to load image", Toast.LENGTH_SHORT).show();
        }
    }

    private void saveBlurredImage() {
        try {
            byte[] exportedBytes = NativeBlurEngine.nativeExportImage(0); // 0 = JPEG, 1 = PNG
            if (exportedBytes == null || exportedBytes.length == 0) {
                Toast.makeText(this, "No blurred image to save", Toast.LENGTH_SHORT).show();
                return;
            }

            ContentValues values = new ContentValues();
            values.put(MediaStore.Images.Media.DISPLAY_NAME, "blurred_" + System.currentTimeMillis() + ".jpg");
            values.put(MediaStore.Images.Media.MIME_TYPE, "image/jpeg");
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                values.put(MediaStore.Images.Media.RELATIVE_PATH, "Pictures/BlurGLES");
                values.put(MediaStore.Images.Media.IS_PENDING, 1);
            }

            Uri uri = getContentResolver().insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
            if (uri != null) {
                try (OutputStream outputStream = getContentResolver().openOutputStream(uri)) {
                    if (outputStream != null) {
                        outputStream.write(exportedBytes);
                        outputStream.flush();
                    }
                }
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    values.clear();
                    values.put(MediaStore.Images.Media.IS_PENDING, 0);
                    getContentResolver().update(uri, values, null, null);
                }
                Toast.makeText(this, "Blurred image saved successfully", Toast.LENGTH_LONG).show();
            } else {
                Toast.makeText(this, "Failed to create MediaStore entry", Toast.LENGTH_SHORT).show();
            }
        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(this, "Failed to save image: " + e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }

    private void updateProcessingTime(double durationMs) {
        if (durationMs > 0.0) {
            if (slowestMs < 0.0 || durationMs > slowestMs) {
                slowestMs = durationMs;
            }
            if (fastestMs < 0.0 || durationMs < fastestMs) {
                fastestMs = durationMs;
            }
            binding.tvProcCurrent.setText(String.format(Locale.US, "%.2f ms", durationMs));
            binding.tvProcFast.setText(String.format(Locale.US, "%.2f ms", fastestMs));
            binding.tvProcSlow.setText(String.format(Locale.US, "%.2f ms", slowestMs));
            binding.tvProcessingTime.setText(String.format(Locale.US,
                    "Native proc: %.2f ms    slow: %.2f ms     fast: %.2f ms",
                    durationMs, slowestMs, fastestMs));
        }
    }

    private byte[] loadAssetBytes(String path) {
        try (InputStream is = getAssets().open(path);
             ByteArrayOutputStream baos = new ByteArrayOutputStream()) {
            byte[] buf = new byte[16384];
            int len;
            while ((len = is.read(buf)) != -1) {
                baos.write(buf, 0, len);
            }
            return baos.toByteArray();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    private void runBenchmark() {
        binding.cardBenchmarkReport.setVisibility(View.VISIBLE);
        binding.tvBenchmarkResults.setText("Running GPU Hardware Benchmark Suite...\nLoading Asset Samples (HD, Full HD, 4K) x 9 Radii x 100 Runs (10 Warm-up)\nPlease wait...");
        binding.btnRunBenchmark.setEnabled(false);

        executorService.execute(() -> {
            byte[] hdBytes = loadAssetBytes("samples/sample_hd_720p.png");
            byte[] fhdBytes = loadAssetBytes("samples/sample_fhd_1080p.png");
            byte[] uhdBytes = loadAssetBytes("samples/sample_4k_2160p.png");

            byte[][] samplePayloads = null;
            if (hdBytes != null && fhdBytes != null && uhdBytes != null) {
                samplePayloads = new byte[][]{ hdBytes, fhdBytes, uhdBytes };
            }

            String jsonResult;
            if (samplePayloads != null) {
                jsonResult = NativeBlurEngine.nativeRunBenchmarkWithSamples(samplePayloads, 100);
            } else {
                jsonResult = NativeBlurEngine.nativeRunBenchmark(100);
            }

            runOnUiThread(() -> {
                binding.btnRunBenchmark.setEnabled(true);
                formatBenchmarkReport(jsonResult);
            });
        });
    }

    private void formatBenchmarkReport(String jsonResult) {
        try {
            JSONObject obj = new JSONObject(jsonResult);
            if (obj.has("error")) {
                binding.tvBenchmarkResults.setText("Benchmark Error: " + obj.getString("error"));
                return;
            }

            int iters = obj.optInt("iterationsPerRadius", 100);
            JSONArray resolutions = obj.getJSONArray("resolutions");

            StringBuilder sb = new StringBuilder();
            sb.append("=== GPU HARDWARE BENCHMARK REPORT ===\n");
            sb.append("Config: 10 Warm-up + ").append(iters).append(" Runs per Radius\n\n");

            for (int rIdx = 0; rIdx < resolutions.length(); rIdx++) {
                JSONObject resObj = resolutions.getJSONObject(rIdx);
                String label = resObj.getString("label");
                JSONArray results = resObj.getJSONArray("results");

                sb.append("----------------------------------------------------\n");
                sb.append("[").append(label).append("]\n");
                sb.append("Radius  |  Mean(ms) |   P95(ms) |  Min/Max  |    FPS\n");
                sb.append("----------------------------------------------------\n");

                for (int i = 0; i < results.length(); i++) {
                    JSONObject item = results.getJSONObject(i);
                    int r = item.getInt("radiusPx");
                    double mean = item.getDouble("meanMs");
                    double p95 = item.getDouble("p95Ms");
                    double min = item.getDouble("minMs");
                    double max = item.getDouble("maxMs");
                    double fps = item.getDouble("fps");

                    sb.append(String.format(Locale.US,
                            "%4d px | %9.2f | %9.2f | %4.1f/%-4.1f | %6.1f\n",
                            r, mean, p95, min, max, fps));
                }
                sb.append("\n");
            }
            binding.tvBenchmarkResults.setText(sb.toString().trim());
        } catch (Exception e) {
            e.printStackTrace();
            binding.tvBenchmarkResults.setText("Raw Benchmark Output:\n" + jsonResult);
        }
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        NativeBlurEngine.nativeSurfaceCreated(holder.getSurface());
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        NativeBlurEngine.nativeSurfaceChanged(width, height);
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        NativeBlurEngine.nativeSurfaceDestroyed();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        executorService.shutdown();
        NativeBlurEngine.nativeRelease();
    }
}
