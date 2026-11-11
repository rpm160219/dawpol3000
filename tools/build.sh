#!/bin/sh
# Budowanie: ./tools/build.sh [desktop|arm]   (domyslnie desktop)
set -e
cd "$(dirname "$0")/.."
T=${1:-desktop}
if command -v cmake >/dev/null 2>&1; then
  if [ "$T" = arm ]; then
    cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64.cmake && cmake --build build-arm -j
  else
    cmake -S . -B build && cmake --build build -j
  fi
else
  # Awaryjnie bez CMake (tylko desktop).
  [ "$T" = arm ] && { echo "Do builda ARM potrzebny cmake + aarch64-linux-gnu-g++"; exit 1; }
  mkdir -p build
  g++ -std=c++17 -O2 -Wall -Wextra -fno-exceptions -fno-rtti -Icore $(sdl2-config --cflags) \
    core/*.cpp core/*/*.cpp platform/main_sdl.cpp -o build/dawpol3000 $(sdl2-config --libs) -lpthread -lm
fi
echo "OK"
