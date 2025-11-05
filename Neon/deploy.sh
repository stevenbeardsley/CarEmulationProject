#!/bin/bash
set -e

# 🔹 Detect OS
OS_TYPE="$(uname)"
if [[ "$OS_TYPE" == "Linux" ]]; then
    EXEC_NAME="DashboardSim"         # Linux executable
else
    EXEC_NAME="DashboardSim.exe"     # Windows executable
fi

# 🔹 Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
ECM_DIR="$PROJECT_ROOT/dashboard"
IMAGE_NAME="dashboardsim:latest"
CONTAINER_NAME="dashboard"

echo "🔧 Building project..."

# Clean previous build
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Run cmake + make
cmake -G "Unix Makefiles" ..
CPU_CORES=$(nproc 2>/dev/null || echo 4)
make -j"$CPU_CORES"

cd "$PROJECT_ROOT"

# 🔹 Locate executable
EXEC_PATH=$(find "$BUILD_DIR" "$ECM_DIR" -type f -name "$EXEC_NAME" 2>/dev/null | head -n 1)
if [ -z "$EXEC_PATH" ]; then
    echo "❌ Executable not found!"
    echo "Checked: $BUILD_DIR and $ECM_DIR"
    exit 1
fi
echo "✅ Found executable: $EXEC_PATH"

# 🔹 Copy executable to project root for Docker build
cp "$EXEC_PATH" "$PROJECT_ROOT/$EXEC_NAME"

# 🔹 Build Docker image
echo "🐋 Building Docker image: $IMAGE_NAME"
docker build -t "$IMAGE_NAME" "$PROJECT_ROOT"

# 🔹 Run Docker container
echo "🚀 Starting container: $CONTAINER_NAME"
docker run -d \
  -p 8080:8080 \
  --name "$CONTAINER_NAME" \
  -e LOG_LEVEL=info \
  "$IMAGE_NAME"

echo "✅ Done!"
echo "➡️ To run interactively:"
echo "   docker exec -it $CONTAINER_NAME /app/$EXEC_NAME"
