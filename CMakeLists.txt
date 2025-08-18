cmake_minimum_required(VERSION 3.31)
project(mouse_crosshair)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)

# 强制静态编译
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)

# 通用静态链接设置
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL
                                           "Clang")
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MT")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(
  mouse_crosshair WIN32 main.cpp src/config_file_util.cpp src/crosshair.cpp
                        src/config.cpp src/hotkey.cpp res/resource.rc)

# 体积优化选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(mouse_crosshair PRIVATE -Os -ffunction-sections
                                                 -fdata-sections -flto)
  target_link_options(
    mouse_crosshair
    PRIVATE
    -static-libgcc
    -static-libstdc++
    -static
    -Wl,--gc-sections
    -Wl,--strip-all
    -flto
    -s)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(mouse_crosshair PRIVATE -Os -ffunction-sections
                                                 -fdata-sections -flto)
  target_link_options(
    mouse_crosshair
    PRIVATE
    -static-libgcc
    -static-libstdc++
    -static
    -Wl,--gc-sections
    -Wl,--strip-all
    -flto
    -s)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(mouse_crosshair PRIVATE /MT /O1 /GL)
  target_link_options(mouse_crosshair PRIVATE /LTCG /OPT:REF /OPT:ICF)
else()
  target_link_options(mouse_crosshair PRIVATE -static)
endif()

# 静态链接 Windows 系统库
target_link_libraries(mouse_crosshair -static user32 gdi32 gdiplus)

# 资源文件
if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  enable_language(RC)
  set(CMAKE_RC_COMPILER_INIT windres)
  set(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif()

# UPX 压缩
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  find_program(UPX_EXECUTABLE upx)
  if(UPX_EXECUTABLE)
    add_custom_command(
      TARGET mouse_crosshair
      POST_BUILD
      COMMAND ${UPX_EXECUTABLE} --best --lzma $<TARGET_FILE:mouse_crosshair>
      COMMENT "Compressing executable with UPX")
  endif()
endif()

# 强制静态链接验证
get_target_property(LINK_FLAGS mouse_crosshair LINK_FLAGS)
message(STATUS "Link flags: ${LINK_FLAGS}")
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build shared libs: ${BUILD_SHARED_LIBS}")
