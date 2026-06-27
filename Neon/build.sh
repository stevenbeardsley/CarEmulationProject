#!/bin/bash

set -e  

BUILD_DIR="build"
BUILD_TYPE="Release"

if [ $# -lt 1 ]; then
    echo "Usage: $0 DashboardSim [build_type]"
    exit 1
fi

TARGET_NAME=$1
if [ $# -ge 2 ]; then
    BUILD_TYPE=$2
fi

echo ">>> Building target '$TARGET_NAME' (Build Type: $BUILD_TYPE)"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f "CMakeCache.txt" ]; then
    echo ">>> Running CMake configuration..."
    cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
fi

echo ">>> Running build..."
cmake --build . --target "DashboardSim" --config "$BUILD_TYPE" -j$(nproc)

echo ">>> Build complete for target '$TARGET_NAME'"
