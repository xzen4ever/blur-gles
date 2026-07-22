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
| 🔄 **Ping-Pong FBO Architecture** | Luân phiên render qua lại giữa 2 Framebuffer Objects, xử lý mượt mà bán kính mờ siêu rộng (tới 1000px) |
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
├── app/
│   ├── CMakeLists.txt              # Cấu hình biên dịch CMake NDK cho `libblur_gles.so`
│   ├── build.gradle                # Cấu hình module app và NDK options
│   └── src/
│       └── main/
│           ├── AndroidManifest.xml # Cấu hình Android Manifest
│           ├── cpp/                # Mã nguồn Native C++ (OpenGL ES Engine)
│           │   ├── gles_renderer.cpp / .h  # EGL setup, FBO Ping-Pong, Shader Program & Render loop
│           │   ├── native_blur_engine.cpp  # JNI Bindings (Java ↔ C++)
│           │   ├── shader_utils.cpp / .h   # Utility biên dịch & liên kết GLSL Shader
│           │   └── third_party/            # Thư viện header-only (stb_image, stb_image_write)
│           ├── java/com/example/blurgles/
│           │   ├── MainActivity.java       # Controller chính, Photo Picker, MediaStore, UI Listener
│           │   └── NativeBlurEngine.java   # Khai báo các native JNI methods
│           └── res/                        # Layout XML, Drawables, Values
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
<summary><b>3. Window Presentation & Native Export</b></summary>

- **Hiển thị màn hình**: Texture kết quả cuối cùng từ FBO được render trực tiếp lên `ANativeWindow` hiển thị trên `SurfaceView`.
- **Xuất ảnh**: Khi nhấn **Save Image**, C++ render khung hình mờ vào FBO, đọc dữ liệu pixel bằng `glReadPixels`, mã hóa thành byte stream JPEG bằng `stbi_write_jpg_to_func`, và chuyển về Java để ghi vào bộ nhớ thông qua `MediaStore` API.

</details>

---

## Performance & Benchmarking

Nhờ việc tận dụng GPU và loại bỏ chi phí truyền nhận Bitmap qua JNI:
- **Độ trễ thấp**: Thời gian làm mờ cho ảnh dung lượng lớn chỉ mất vài millisecond.
- **Tiết kiệm RAM**: Dữ liệu ảnh không bị duplicate ở bộ nhớ Java Heap.
- **Chỉ số thời gian thực**:
  - `Native Proc`: Thời gian xử lý đợt làm mờ hiện tại.
  - `Fastest / Slowest`: Thống kê độ trễ GPU nhanh nhất / chậm nhất ghi nhận được trong phiên chạy.

---

## License

Dự án được phân phối dưới giấy phép **[MIT License](LICENSE)**.
