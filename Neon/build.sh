#!/bin/bash
# build_target.sh
# A simple script to build a specific CMake target.

set -e  # Exit immediately if a command exits with a non-zero status

# --- Configuration ---
BUILD_DIR="build"
BUILD_TYPE="Release"

# --- Arguments ---
if [ $# -lt 1 ]; then
    echo "Usage: $0 DashboardSim [build_type]"
    exit 1
fi

TARGET_NAME=$1
if [ $# -ge 2 ]; then
    BUILD_TYPE=$2
fi

# --- Script ---
echo ">>> Building target '$TARGET_NAME' (Build Type: $BUILD_TYPE)"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure if not already done
if [ ! -f "CMakeCache.txt" ]; then
    echo ">>> Running CMake configuration..."
    cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
fi

# Build the target
echo ">>> Running build..."
cmake --build . --target "DashboardSim" --config "$BUILD_TYPE" -j$(nproc)

echo ">>> Build complete for target '$TARGET_NAME'"
