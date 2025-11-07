#!/bin/bash
set -e

# ==============================
# 🚀 Multi-Target Build & Docker Runner
# ==============================

# --- Detect OS ---
OS_TYPE="$(uname)"
if [[ "$OS_TYPE" == "Linux" ]]; then
    EXEC_EXT=""
else
    EXEC_EXT=".exe"
fi

# --- Configuration ---
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
CPU_CORES=$(nproc 2>/dev/null || echo 4)

# ==============================
# 🔧 Target Configuration
# ==============================

# --- Executable names (built output names) ---
EXEC_NAMES=(
    "Dashboard"
    "Ecm"
    "Tcm"
)

# --- Dockerfile names (relative to project root) ---
DOCKERFILES=(
    "Dockerfile.dashboard"
    "Dockerfile.ecm"
    "Dockerfile.tcm"
)

# --- Docker image names ---
IMAGE_NAMES=(
    "dashboardsim:latest"
    "ecmsim:latest"
    "tcmsim:latest"
)

# --- Validate list lengths ---
if [[ ${#EXEC_NAMES[@]} -ne ${#DOCKERFILES[@]} || ${#EXEC_NAMES[@]} -ne ${#IMAGE_NAMES[@]} ]]; then
    echo "❌ Error: EXEC_NAMES, DOCKERFILES, and IMAGE_NAMES must have the same length."
    exit 1
fi

# ==============================
# 🏗️  Build Section
# ==============================

echo "🔧 Cleaning previous build..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "🏗️  Running CMake and Make..."
cmake -G "Unix Makefiles" ..
make -j"$CPU_CORES"

cd "$PROJECT_ROOT"

# ==============================
# 🐋 Docker Build Section
# ==============================

for i in "${!EXEC_NAMES[@]}"; do
    EXEC_NAME="${EXEC_NAMES[$i]}"
    DOCKERFILE="${DOCKERFILES[$i]}"
    IMAGE_NAME="${IMAGE_NAMES[$i]}"
    CONTAINER_NAME="${EXEC_NAME,,}"

    echo ""
    echo "=============================="
    echo "⚙️  Building target: $EXEC_NAME"
    echo "=============================="

    EXEC_PATH=$(find "$BUILD_DIR" -type f -name "${EXEC_NAME}${EXEC_EXT}" 2>/dev/null | head -n 1)
    if [ -z "$EXEC_PATH" ]; then
        echo "❌ Executable not found for $EXEC_NAME"
        continue
    fi

    echo "✅ Found executable: $EXEC_PATH"
    cp "$EXEC_PATH" "$PROJECT_ROOT/${EXEC_NAME}${EXEC_EXT}"

    DOCKERFILE_PATH="$PROJECT_ROOT/$DOCKERFILE"
    if [ ! -f "$DOCKERFILE_PATH" ]; then
        echo "❌ Dockerfile not found: $DOCKERFILE_PATH"
        continue
    fi

    echo "🐋 Building Docker image: $IMAGE_NAME (using $DOCKERFILE_PATH)"
    docker build -t "$IMAGE_NAME" -f "$DOCKERFILE_PATH" "$PROJECT_ROOT"

    # 🧹 Remove old container if it exists
    if docker ps -a --format '{{.Names}}' | grep -Eq "^${CONTAINER_NAME}\$"; then
        echo "🧹 Removing existing container: $CONTAINER_NAME"
        docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true
    fi

    # 🚀 Conditional port exposure
    if [[ "${EXEC_NAME,,}" == "dashboard" ]]; then
        echo "🌐 Exposing ports for $EXEC_NAME"
        docker run -d \
            -p 8080:8080 \
            --name "$CONTAINER_NAME" \
            -e LOG_LEVEL=info \
            "$IMAGE_NAME"
        echo "➡️  Running at: http://localhost:8080"
    else
        echo "🚀 Starting container without port exposure: $CONTAINER_NAME"
        docker run -d \
            --name "$CONTAINER_NAME" \
            -e LOG_LEVEL=info \
            "$IMAGE_NAME"
    fi

    echo "✅ Done for $EXEC_NAME"
    echo "➡️  To run interactively: docker exec -it $CONTAINER_NAME /app/${EXEC_NAME}${EXEC_EXT}"
exit 0
done
