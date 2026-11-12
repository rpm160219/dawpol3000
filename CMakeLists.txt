cmake_minimum_required(VERSION 3.16)
project(dawpol3000 C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)  # eva.c (op1r): __typeof__, inicjalizatory C
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions> $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(dpcore STATIC
  core/gfx/canvas.cpp core/gfx/font.cpp core/gfx/icons.cpp
  core/io/image_io.cpp core/io/fs.cpp core/io/project_file.cpp core/io/project_store.cpp core/io/preset_store.cpp core/io/factory_presets.cpp
  core/synth/tone_synth.cpp core/synth/saw_synth.cpp core/synth/wavetable_synth.cpp core/synth/taffy_dsp.cpp core/synth/pew_dsp.cpp core/synth/elg_dsp.cpp core/synth/eva_dsp.c core/synth/mbira_dsp.c core/synth/reactor_dsp.c core/synth/oxbow_dsp.c core/synth/oxbow_plus_dsp.c core/synth/sid_dsp_wrap.c core/synth/monolit_dsp.c core/synth/vapora_dsp.c core/synth/supersaw_dsp.c core/synth/mixtur_dsp.c core/synth/brass_dsp.c core/synth/hollowsaint_dsp.c core/synth/bloodmoon_dsp.c core/synth/blot_dsp.c third_party/op1r_vapora/vapora_dsp.c third_party/op1r_sid/sid_dsp.c
  core/seq/pattern.cpp core/seq/project.cpp core/seq/demo_song.cpp core/seq/sequencer.cpp
  core/audio/engine.cpp
  core/ui/test_screen.cpp core/ui/piano_roll.cpp core/ui/arrange.cpp core/ui/dialogs.cpp
  core/ui/widgets.cpp core/ui/app.cpp core/ui/project_view.cpp core/ui/synth_view.cpp core/ui/wavetable_screen.cpp core/ui/taffy_screen.cpp core/ui/pew_screen.cpp core/ui/elg_screen.cpp core/ui/eva_screen.cpp core/ui/mbira_screen.cpp core/ui/reactor_screen.cpp core/ui/oxbow_screen.cpp core/ui/sid_screen.cpp core/ui/monolit_screen.cpp core/ui/vapora_screen.cpp core/ui/vapora_screen_c.c core/ui/supersaw_screen.cpp core/ui/mixtur_screen.cpp core/ui/brass_screen.cpp core/ui/hollowsaint_screen.cpp core/ui/bloodmoon_screen.cpp core/ui/blot_screen.cpp core/ui/splash.cpp
  core/headless.cpp)
target_include_directories(dpcore PUBLIC core)
set_source_files_properties(third_party/op1r_sid/sid_dsp.c PROPERTIES COMPILE_OPTIONS "-Wno-misleading-indentation")  # oryginal 1:1

add_executable(dawpol3000 platform/main_sdl.cpp)
target_include_directories(dawpol3000 PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(dawpol3000 dpcore ${SDL2_LIBRARIES} pthread m)

# Na konsoli nie polegamy na systemowym libstdc++ (inna wersja niz w Debianie).
if(CMAKE_CROSSCOMPILING)
  target_link_options(dawpol3000 PRIVATE -static-libstdc++ -static-libgcc)
endif()
