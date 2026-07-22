package com.example.blurgles;

import android.content.ContentValues;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore;
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

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private ActivityMainBinding binding;

    private double slowestMs = -1.0;
    private double fastestMs = -1.0;

    private ActivityResultLauncher<PickVisualMediaRequest> imagePickerLauncher;

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
        NativeBlurEngine.nativeRelease();
    }
}
