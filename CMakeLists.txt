cmake_minimum_required(VERSION 4.1.0)
project(mouse_crosshair VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_GENERATOR "Ninja")
set(CMAKE_MAKE_PROGRAM "ninja")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT MSVC)
  message(FATAL_ERROR "This project strictly requires the MSVC compiler.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ARCH_SUFFIX "x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(ARCH_SUFFIX "x86")
else()
  set(ARCH_SUFFIX "unknown")
endif()

# EXECUTABLE_OUTPUT_PATH
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

# 静态编译仅针对 Release (/MT)，Debug 保持默认动态链接 (/MDd)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

add_executable(
  mouse_crosshair WIN32 main.cpp src/config_file_util.cpp src/crosshair.cpp
  src/config.cpp src/hotkey.cpp res/resource.rc)

set_target_properties(mouse_crosshair PROPERTIES
  OUTPUT_NAME "mouse_crosshair_${ARCH_SUFFIX}"
)

target_compile_options(mouse_crosshair PRIVATE
  /utf-8
  $<$<CONFIG:Debug>:/Od /Zi>
  $<$<CONFIG:Release>:/O1 /GL>
)

target_link_options(mouse_crosshair PRIVATE
  $<$<CONFIG:Debug>:/DEBUG>
  $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
)

# 链接 Windows 系统库
target_link_libraries(mouse_crosshair PRIVATE user32 gdi32 d2d1 dwrite)

