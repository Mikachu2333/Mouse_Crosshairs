cmake_minimum_required(VERSION 3.31)
project(mouse_crosshair)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_BUILD_TYPE Release)

if(WIN32)
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

  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(
  mouse_crosshair WIN32 main.cpp src/config_file_util.cpp src/crosshair.cpp
                        src/config.cpp src/hotkey.cpp res/resource.rc)

if(WIN32)
  target_link_libraries(mouse_crosshair -static user32 gdi32 gdiplus)
else()
  target_link_libraries(mouse_crosshair user32 gdi32 gdiplus)
endif()

if(MINGW)
  enable_language(RC)
  set(CMAKE_RC_COMPILER_INIT windres)
  set(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(mouse_crosshair PRIVATE -static-libgcc -static-libstdc++)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_link_options(mouse_crosshair PRIVATE -static-libgcc -static-libstdc++)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(mouse_crosshair PRIVATE /MT)
endif()
