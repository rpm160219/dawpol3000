cmake_minimum_required(VERSION 3.16)
project(dawpol3000 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra -fno-exceptions -fno-rtti)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(dpcore STATIC
  core/canvas.cpp core/font.cpp core/image_io.cpp core/tone_synth.cpp
  core/hello_app.cpp core/headless.cpp)
target_include_directories(dpcore PUBLIC core)

add_executable(dawpol3000 platform/main_sdl.cpp)
target_include_directories(dawpol3000 PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(dawpol3000 dpcore ${SDL2_LIBRARIES} pthread m)

# Na konsoli nie polegamy na systemowym libstdc++ (inna wersja niz w Debianie).
if(CMAKE_CROSSCOMPILING)
  target_link_options(dawpol3000 PRIVATE -static-libstdc++ -static-libgcc)
endif()
