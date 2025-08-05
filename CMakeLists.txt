cmake_minimum_required(VERSION 3.31)
project(mouse_crosshair)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        mouse_crosshair
        main.cpp
        config_file_util.cpp
        crosshair.cpp
        config.cpp
        hotkey.cpp
        resource.rc
)

target_link_libraries(
        mouse_crosshair
        user32
        gdi32
        gdiplus
)
