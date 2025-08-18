cmake_minimum_required(VERSION 3.31)
project(mouse_crosshair)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(
        mouse_crosshair  WIN32
        main.cpp
        src/config_file_util.cpp
        src/crosshair.cpp
        src/config.cpp
        src/hotkey.cpp
        res/resource.rc
)

target_link_libraries(
        mouse_crosshair
        user32
        gdi32
        gdiplus
)
