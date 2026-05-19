cmake_minimum_required(VERSION 3.20)
project(vision_bot_cpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)

add_executable(vision_bot
    src/main.cpp
    src/core/capture.cpp
    src/core/input.cpp
    src/core/humanizer.cpp
    src/core/watcher.cpp
    src/core/debug_view.cpp
    src/core/runner.cpp
    src/mechanics/slider.cpp
    src/profiles/profiles.cpp
)

target_include_directories(vision_bot PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(vision_bot PRIVATE ${OpenCV_LIBS} d3d11 dxgi user32 gdi32)

if(MSVC)
    target_compile_options(vision_bot PRIVATE /W4 /utf-8)
    target_compile_definitions(vision_bot PRIVATE
        UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
