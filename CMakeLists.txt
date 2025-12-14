cmake_minimum_required(VERSION 3.31)
project(mouse_crosshair)

set(CMAKE_CXX_STANDARD 20)

# 强制静态编译
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)

# 通用静态链接设置
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL
  "Clang")
  # 保持通用静态链接标志（两种配置都适用）
  set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
  set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # MSVC 运行时按配置区分（Debug 用 MTd，Release 用 MT）
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MT")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(
  mouse_crosshair WIN32 main.cpp src/config_file_util.cpp src/crosshair.cpp
  src/config.cpp src/hotkey.cpp res/resource.rc)

# 体积优化选项：Release 保持现状；Debug 禁用优化并启用调试信息
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Debug：无优化，带调试信息；不使用 LTO/strip
  target_compile_options(mouse_crosshair PRIVATE
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-Os -ffunction-sections -fdata-sections -flto>
  )
  target_link_options(mouse_crosshair PRIVATE
    $<$<CONFIG:Release>:-static-libgcc -static-libstdc++ -static -Wl,--gc-sections -Wl,--strip-all -flto -s>

    # Debug 不添加上述瘦身/静态库重复标志，这里保持通用静态由全局 linker flags 控制
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(mouse_crosshair PRIVATE
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-Os -ffunction-sections -fdata-sections -flto>
  )
  target_link_options(mouse_crosshair PRIVATE
    $<$<CONFIG:Release>:-static-libgcc -static-libstdc++ -static -Wl,--gc-sections -Wl,--strip-all -flto -s>
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(mouse_crosshair PRIVATE
    $<$<CONFIG:Debug>:/Od /Zi>
    $<$<CONFIG:Release>:/MT /O1 /GL>
  )
  target_link_options(mouse_crosshair PRIVATE
    $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    $<$<CONFIG:Debug>:/DEBUG>
  )
else()
  # 其他编译器：Release 静态链接；Debug 默认即可
  target_link_options(mouse_crosshair PRIVATE
    $<$<CONFIG:Release>:-static>
  )
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

# UPX 压缩：仅在 Release 时执行
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
