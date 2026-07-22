<div align="center">

# BlurGLES 🚀

### High-Performance Hardware-Accelerated Image Blur Engine for Android

**Tăng tốc làm mờ ảnh bằng phần cứng với OpenGL ES 3.0+ · Ping-Pong FBO · Native C++ I/O · 100% On-Device**

[![Android](https://img.shields.io/badge/Platform-Android-green.svg?style=flat-square&logo=android)](#architecture--tech-stack)
[![Min SDK](https://img.shields.io/badge/Min%20SDK-API%2024%20(7.0)-blue.svg?style=flat-square)](#architecture--tech-stack)
[![OpenGL ES](https://img.shields.io/badge/OpenGL%20ES-3.2-orange.svg?style=flat-square&logo=opengl)](#architecture--tech-stack)
[![NDK](https://img.shields.io/badge/Android%20NDK-r25+-brightgreen.svg?style=flat-square)](#architecture--tech-stack)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17%20%7C%20Java-007ACC.svg?style=flat-square&logo=cplusplus)](#architecture--tech-stack)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](#license)

</div>

---

## Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Architecture & Tech Stack](#architecture--tech-stack)
- [Project Structure](#project-structure)
- [Quick Start](#quick-start)
- [How It Works](#how-it-works)
- [Performance & Benchmarking](#performance--benchmarking)
- [License](#license)

---

## Overview

**BlurGLES** là một ứng dụng Android chuyên dụng minh họa kỹ thuật làm mờ ảnh (Gaussian Blur) với hiệu năng cực cao bằng việc kết hợp sức mạnh của **OpenGL ES 3.0+**, **EGL**, và **Android NDK (C++)**.

Bằng việc chuyển toàn bộ công đoạn giải mã ảnh, render đa bước qua chuỗi **FBO (Framebuffer Objects) Ping-Pong**, và mã hóa ảnh trực tiếp ở tầng C++ Native, BlurGLES loại bỏ hoàn toàn hiện tượng nghẽn cổ chai dữ liệu khi truyền giữa Java Bitmap và GPU, đạt tốc độ xử lý tức thì với độ trễ cực thấp.

---

## Key Features

| Feature | Description |
|---|---|
| ⚡ **OpenGL ES 3.0+ Acceleration** | Tăng tốc GPU làm mờ ảnh bằng GLSL shaders chuyên dụng, hỗ trợ Separable Gaussian Blur (tách 2 chiều ngang/dọc) |
| 🔄 **Ping-Pong FBO Architecture** | Luân phiên render qua lại giữa 2 Framebuffer Objects, xử lý mượt mà bán kính mờ siêu rộng |
| 📊 **Hardware Benchmark Suite** | Bộ đo hiệu năng GPU tự động trên các độ phân giải tiêu chuẩn (720p HD, 1080p FHD, 4K 2160p UHD) với thống kê Mean, P95, Min/Max và FPS |
| 🖼️ **Native Image I/O** | Giải mã JPEG/PNG trực tiếp từ byte stream và mã hóa xuất ảnh ở tầng C++ native bằng `stb_image` & `stb_image_write` |
| ⏱️ **Real-time Micro-Benchmarking** | Đo chính xác thời gian render GPU bằng `std::chrono::high_resolution_clock` ở tầng Native C++ theo thời gian thực |
| 🎛️ **Dynamic Radius Tuning** | Thanh cuộn (SeekBar) tương tác cho phép thay đổi bán kính mờ mượt mà ngay trên giao diện người dùng |
| 📱 **Modern Android UI** | Thiết kế Edge-to-Edge, tự động chuyển đổi Light/Dark status bar, tích hợp Photo Picker (`PickVisualMedia`) và MediaStore API |

---

## Architecture & Tech Stack

### Tech Stack Table

| Component | Technology / Library |
|---|---|
| **Platform** | Android API Level 24+ (Android 7.0+) |
| **Native Kernel** | C++17 (Android NDK r25+) |
| **Graphics API** | OpenGL ES 3.2, EGL 1.4, `ANativeWindow` |
| **Native Libraries** | `stb_image.h`, `stb_image_write.h`, `jnigraphics`, `android/log`, `EGL`, `GLESv3` |
| **Build Tools** | CMake 3.22.1+, Android Gradle Plugin 8.x |
| **Android UI** | Material Components, SurfaceView, ViewBinding, Photo Picker API |

---

## Project Structure

```text
blur-gles/
├── .sample/                        # Script Python tạo mẫu ảnh benchmark (720p, 1080p, 4K)
│   └── generate_samples.py
├── app/
│   ├── CMakeLists.txt              # Cấu hình biên dịch CMake NDK cho `libblur_gles.so`
│   ├── build.gradle                # Cấu hình module app và NDK options
│   └── src/
│       └── main/
│           ├── AndroidManifest.xml # Cấu hình Android Manifest
│           ├── assets/samples/     # Mẫu ảnh benchmark thử nghiệm tích hợp sẵn (720p, 1080p, 4K)
│           ├── cpp/                # Mã nguồn Native C++ (OpenGL ES Engine & Benchmark Suite)
│           │   ├── gles_renderer.cpp / .h  # EGL setup, FBO Ping-Pong, Offscreen Benchmark Suite & Render loop
│           │   ├── native_blur_engine.cpp  # JNI Bindings (Java ↔ C++) cho Render & Benchmark
│           │   ├── shader_utils.cpp / .h   # Utility biên dịch & liên kết GLSL Shader
│           │   └── third_party/            # Thư viện header-only (stb_image, stb_image_write)
│           ├── java/com/example/blurgles/
│           │   ├── MainActivity.java       # Controller chính, Photo Picker, UI Listener & Benchmark UI Report Handler
│           │   └── NativeBlurEngine.java   # Khai báo các native JNI methods (Render & Benchmark)
│           └── res/                        # Layout XML, Drawables, Benchmark Card & Values
├── build.gradle                    # Top-level Gradle script
├── settings.gradle                 # Gradle settings
└── README.md                       # Tài liệu dự án
```

---

## Quick Start

### Prerequisites
- **Android Studio**: Android Studio Hedgehog / Jellyfish / Ladybug trở lên.
- **Android NDK**: NDK version 25 trở lên (Cài đặt qua *SDK Manager → SDK Tools → NDK*).
- **CMake**: Version 3.22.1+.

### Build & Installation

1. **Clone repository**:
   ```bash
   git clone https://github.com/xzen4ever/blur-gles.git
   cd blur-gles
   ```

2. **Mở dự án trong Android Studio**:
   - Mở Android Studio, chọn **Open** và trỏ tới thư mục `blur-gles`.

3. **Đồng bộ & Biên dịch**:
   - Gradle sẽ tự động tải các gói phụ thuộc và cấu hình NDK CMake.

4. **Chạy ứng dụng**:
   - Kết nối thiết bị Android hoặc khởi tạo Emulator (Android 7.0+).
   - Bấm **Run (Shift + F10)** để nạp APK.

---

## How It Works

<details>
<summary><b>1. Native Image Ingestion (Giải mã ảnh ở Native)</b></summary>

Khi người dùng chọn ảnh bằng Photo Picker (`PickVisualMedia`), file buffer dưới dạng byte array được truyền trực tiếp vào C++ qua JNI `nativeSetImageData`. Hàm `stbi_load_from_memory` giải mã dữ liệu JPEG/PNG thành raw RGBA pixels trong C++ memory và nạp trực tiếp vào OpenGL ES Texture (`mInputTexture`) thông qua `glTexImage2D`.

</details>

<details>
<summary><b>2. Separable Gaussian Blur & FBO Ping-Pong Pipeline</b></summary>

Thuật toán làm mờ 2D Gaussian được tối ưu hóa thành 2 pass 1D độc lập:
- **Pass 1 (Horizontal)**: Đọc từ `mInputTexture` và render kết quả mờ chiều ngang vào `FBO0`.
- **Pass 2 (Vertical)**: Đọc từ `FBO0` và render kết quả mờ chiều dọc vào `FBO1`.
- Với bán kính mờ lớn hơn, hệ thống thực hiện vòng lặp **Ping-Pong** luân phiên giữa `FBO0` và `FBO1` cho đến khi đạt được chất lượng hình ảnh mong muốn.

</details>

<details>
<summary><b>3. Offscreen EGL Hardware Benchmarking</b></summary>

- Khi bấm **Run Benchmark**, ứng dụng sử dụng EGL Context độc lập để thực thi kiểm thử offscreen không bị giới hạn bởi tốc độ làm tươi màn hình (VSync).
- Quy trình chạy tự động trên 3 chuẩn độ phân giải (720p HD, 1080p FHD, 4K 2160p UHD) với 10 vòng khởi động (Warm-up) + 100 vòng đo đạc cho từng bán kính mờ (1px, 2px, 4px, 8px, 16px, 32px).
- Báo cáo chi tiết dạng JSON chứa độ trễ trung bình (Mean), P95, Min/Max ms và FPS được định dạng trực tiếp lên thẻ UI.

</details>

<details>
<summary><b>4. Window Presentation & Native Export</b></summary>

- **Hiển thị màn hình**: Texture kết quả cuối cùng từ FBO được render trực tiếp lên `ANativeWindow` hiển thị trên `SurfaceView`.
- **Xuất ảnh**: Khi nhấn **Save Image**, C++ render khung hình mờ vào FBO, đọc dữ liệu pixel bằng `glReadPixels`, mã hóa thành byte stream JPEG bằng `stbi_write_jpg_to_func`, và chuyển về Java để ghi vào bộ nhớ thông qua `MediaStore` API.

</details>

---

## Performance & Benchmarking

BlurGLES tích hợp sẵn **Bộ Benchmark Tự Động GPU Hardware** hỗ trợ kiểm thử hiệu năng chi tiết trực tiếp trên phần cứng thiết bị.

### Quy Trình Kiểm Thử (Benchmark Pipeline)
1. **Offscreen EGL Context**: Khởi tạo EGL Surface ẩn giúp đo chính xác thời gian xử lý GPU độc lập với chu kỳ hiển thị màn hình (VSync).
2. **Đa Độ Phân Giải (Multi-Resolution Suite)**:
   - **720p HD** (1280 x 720)
   - **1080p Full HD** (1920 x 1080)
   - **4K 2160p Ultra HD** (3840 x 2160)
3. **Thử Nghiệm Đa Bán Kính (Blur Radii)**: Chạy qua các mức bán kính 1px, 2px, 4px, 8px, 16px, 32px.
4. **Chuẩn Đo Độ Trễ Precision Profiling**:
   - **Warm-up**: 10 vòng chạy khởi động GPU để loại bỏ chi phí JIT/GPU frequency scaling.
   - **Sample Iterations**: 100 vòng chạy thực nghiệm cho từng mức bán kính.
   - **Metric Output**: Tính toán trung bình (**Mean ms**), độ trễ bách phân vị 95 (**P95 ms**), độ trễ nhỏ nhất/lớn nhất (**Min/Max ms**), và tốc độ khung hình tương đương (**FPS**).

### Giao Diện Báo Cáo Benchmark
Khi người dùng bấm nút **Run Benchmark** trên ứng dụng, báo cáo sẽ được trả về dạng JSON từ C++ Native và định dạng hiển thị ngay trên giao diện:

```text
=== GPU HARDWARE BENCHMARK REPORT ===
Config: 10 Warm-up + 100 Runs per Radius

[720p HD (1280x720)]
Radius  |  Mean(ms) |   P95(ms) |  Min/Max  |    FPS
----------------------------------------------------
   1 px |      0.45 |      0.52 |  0.4/0.7  | 2222.2
   2 px |      0.62 |      0.71 |  0.5/0.9  | 1612.9
   4 px |      0.95 |      1.10 |  0.8/1.4  | 1052.6
   8 px |      1.58 |      1.82 |  1.4/2.1  |  632.9
  16 px |      2.85 |      3.20 |  2.6/3.8  |  350.9
  32 px |      5.40 |      5.95 |  5.1/6.8  |  185.2

[1080p Full HD (1920x1080)]
Radius  |  Mean(ms) |   P95(ms) |  Min/Max  |    FPS
----------------------------------------------------
  ...
```

---

## License

Dự án được phân phối dưới giấy phép **[MIT License](LICENSE)**.

