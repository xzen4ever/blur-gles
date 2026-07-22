# BlurGLES - High-Performance Android OpenGL ES Blur Engine 🚀

**BlurGLES** là một ứng dụng Android hiệu năng cao minh họa kỹ thuật xử lý làm mờ ảnh (Gaussian Blur) tăng tốc phần cứng thông qua **OpenGL ES 3.0+** và **Android NDK (C++)**. Dự án kết hợp quy trình xử lý mờ hai chiều (multi-pass separable blur) với kỹ thuật **Framebuffer Object (FBO) Ping-Pong** và giải mã/mã hóa ảnh trực tiếp ở tầng Native C++ nhằm đạt hiệu suất tối đa và độ trễ cực thấp.

---

## 🌟 Tính năng nổi bật (Key Features)

- ⚡ **Tăng tốc phần cứng OpenGL ES 3.0+**: Xử lý làm mờ hình ảnh bằng GPU thông qua các GLSL shader chuyên dụng, hỗ trợ tách chuỗi mờ theo hai chiều ngang và dọc (Separable Gaussian Blur).
- 🔄 **Kiến trúc Ping-Pong Framebuffer**: Tối ưu hóa việc render luân phiên giữa các FBO giúp xử lý bán kính mờ lớn (lên tới 1000px) một cách mượt mà và tiết kiệm tài nguyên.
- 🖼️ **Native Image Decoding & Encoding**: Nạp và xuất dữ liệu hình ảnh (JPEG/PNG) trực tiếp ở tầng C++ bằng thư viện `stb_image` & `stb_image_write`, loại bỏ hoàn toàn hiện tượng nghẽn cổ chai truyền dữ liệu giữa Java Bitmap và Native.
- ⏱️ **Đo đạc hiệu năng thời gian thực**: Đo chính xác thời gian xử lý GPU/Native tính bằng millisecond (sử dụng `std::chrono::high_resolution_clock` ở tầng C++) và hiển thị trực quan các chỉ số: thời gian xử lý hiện tại, nhanh nhất và chậm nhất.
- 🎛️ **Điều chỉnh bán kính động**: Cho phép thay đổi bán kính mờ (Blur Radius từ 1 đến 1000 px) linh hoạt ngay trên giao diện người dùng theo thời gian thực.
- 📱 **Giao diện Modern Android**: 
  - Tích hợp **Android Photo Picker** (`PickVisualMedia`) an toàn, không yêu cầu quyền truy cập bộ nhớ không cần thiết.
  - Hỗ trợ **Edge-to-Edge UI** và tự động thích ứng với chế độ Sáng/Tối (Light/Dark mode) của hệ thống.
  - Xuất và lưu ảnh đã làm mờ trực tiếp vào thư viện thiết bị qua Android **MediaStore** API.

---

## 🏗️ Kiến trúc & Công nghệ (Architecture & Tech Stack)

### Stack Công nghệ
- **Ngôn ngữ**: C++17 (NDK), Java (Android Framework)
- **Graphics & Display API**: OpenGL ES 3.0 / 3.2, EGL, `ANativeWindow`
- **Native Libraries**: `stb_image.h`, `stb_image_write.h`, `jnigraphics`, `log`, `EGL`, `GLESv3`
- **Build System**: Gradle 8.x, CMake 3.22.1+
- **Min SDK / Target SDK**: Android 7.0 (API Level 24+) / Target API Level 34+

### Cấu trúc dự án (Project Structure)
```text
blur-gles/
├── app/
│   ├── CMakeLists.txt              # Cấu hình biên dịch CMake cho thư viện NDK `libblur_gles.so`
│   ├── build.gradle                # Cấu hình Gradle module app và NDK options
│   └── src/
│       └── main/
│           ├── AndroidManifest.xml # Cấu hình ứng dụng Android
│           ├── cpp/                # Mã nguồn C++ xử lý đồ họa OpenGL ES
│           │   ├── gles_renderer.cpp / .h  # Khởi tạo EGL, FBO Ping-Pong, GLSL Shader, render pipeline
│           │   ├── native_blur_engine.cpp  # JNI Interface kết nối Java và C++
│           │   ├── shader_utils.cpp / .h   # Biên dịch và liên kết Shader program
│           │   └── third_party/            # Thư viện header-only (stb_image, stb_image_write)
│           ├── java/com/example/blurgles/
│           │   ├── MainActivity.java       # Quản lý giao diện, SurfaceHolder, PhotoPicker, MediaStore
│           │   └── NativeBlurEngine.java   # JNI bridge khai báo các hàm native
│           └── res/                        # Giao diện XML (Layouts, Values, Drawables)
├── build.gradle                    # Top-level build script
├── settings.gradle                 # Gradle settings
└── README.md                       # Tài liệu hướng dẫn dự án
```

---

## ⚙️ Yêu cầu & Cài đặt (Setup & Prerequisites)

### Yêu cầu môi trường
- **Android Studio**: Android Studio Hedgehog / Jellyfish / Ladybug trở lên.
- **Android NDK**: NDK version 25+ (được cài đặt qua SDK Manager trong Android Studio).
- **CMake**: Version 3.22.1 trở lên.
- **Thiết bị thử nghiệm**: Android API Level 24 (Android 7.0) trở lên có hỗ trợ OpenGL ES 3.0+.

### Các bước biên dịch và chạy dự án
1. **Clone repository**:
   ```bash
   git clone https://github.com/username/blur-gles.git
   cd blur-gles
   ```
2. **Mở dự án trong Android Studio**:
   - Chọn `Open an existing project` và trỏ tới thư mục `blur-gles`.
3. **Đồng bộ Gradle & NDK**:
   - Android Studio sẽ tự động nạp dependencies và cấu hình CMake cho NDK.
4. **Build & Run**:
   - Kết nối thiết bị Android thật hoặc khởi chạy Android Emulator.
   - Nhấn **Run (Shift + F10)** để biên dịch và cài đặt APK.

---

## 🛠️ Nguyên lý hoạt động của Blur Engine (How It Works)

1. **Native Image Loading**: Khi chọn ảnh từ Photo Picker, mảng byte nén (JPEG/PNG) được chuyển trực tiếp vào C++ qua hàm JNI `nativeSetImageData`. Thư viện `stb_image` giải mã dữ liệu ảnh thành raw pixels và nạp thẳng vào OpenGL ES Texture (`mInputTexture`).
2. **Ping-Pong Rendering Pass**:
   - Thuật toán Separable Gaussian Blur chia công đoạn làm mờ thành 2 bước (ngang `Horizontal` và dọc `Vertical`).
   - Hai Framebuffer Objects (`FBO0` và `FBO1`) được dùng luân phiên làm nguồn đọc và đích render (Ping-Pong technique).
   - Số lượt pass được tính toán tự động dựa theo bán kính `mBlurRadiusPx` chọn trên thanh cuộn.
3. **Surface Display**: Kết quả xử lý từ FBO cuối cùng được vẽ ra màn hình thông qua `ANativeWindow` gắn liền với `SurfaceView`.
4. **Export Image**: Khi người dùng nhấn nút Save, engine render khung hình mờ vào FBO, đọc lại dữ liệu pixel và mã hóa ra mảng byte JPEG bằng `stb_image_write`. Tầng Java sẽ lưu file này vào bộ nhớ thiết bị thông qua `MediaStore` API.

---

## 📜 Giấy phép (License)

Dự án được phân phối theo giấy phép **MIT License**.
