#!/bin/bash
VCPKG_ROOT=~/vcpkg
cmake -S . -B build-linux -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build-linux --config Debug
