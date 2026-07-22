cmake_minimum_required(VERSION 3.22.1)
project("blur_gles")

add_library(
        blur_gles
        SHARED
        src/main/cpp/native_blur_engine.cpp
        src/main/cpp/gles_renderer.cpp
        src/main/cpp/shader_utils.cpp
)

target_include_directories(blur_gles PRIVATE src/main/cpp src/main/cpp/third_party)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles3-lib GLESv3)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(
        blur_gles
        ${log-lib}
        ${android-lib}
        ${egl-lib}
        ${gles3-lib}
        ${jnigraphics-lib}
)
