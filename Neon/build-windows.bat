@echo off
set VCPKG_ROOT=C:\path\to\vcpkg
cmake -S . -B build-windows -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build-windows --config Debug
