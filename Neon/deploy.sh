#!/usr/bin/env bash
set -euo pipefail

# --- Detect OS ---
OS_TYPE="$(uname -s)"
case "$OS_TYPE" in
  Linux*)   PLATFORM="linux"; EXEC_EXT="";;
  Darwin*)  PLATFORM="mac";   EXEC_EXT="";;   # if you ever run on macOS
  MINGW*|MSYS*|CYGWIN*) PLATFORM="win"; EXEC_EXT=".exe";;
  *)        PLATFORM="unknown"; EXEC_EXT="";;
esac

# --- Paths ---
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-$PLATFORM"

# --- CPU cores ---
if command -v nproc >/dev/null 2>&1; then
  CPU_CORES="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
  CPU_CORES="$(sysctl -n hw.ncpu)"
else
  CPU_CORES=4
fi

# ==============================
# Target Configuration
# ==============================
EXEC_NAMES=("Dashboard" "Ecm" "Tcm")
DOCKERFILES=("Dockerfile.dashboard" "Dockerfile.ecm" "Dockerfile.tcm")
IMAGE_NAMES=("dashboardsim:latest" "ecmsim:latest" "tcmsim:latest")

if [[ ${#EXEC_NAMES[@]} -ne ${#DOCKERFILES[@]} || ${#EXEC_NAMES[@]} -ne ${#IMAGE_NAMES[@]} ]]; then
  echo "Error: EXEC_NAMES, DOCKERFILES, and IMAGE_NAMES must have the same length."
  exit 1
fi

# ==============================
# Docker Network 
# ==============================
DOCKER_NETWORK="neon_network"

if ! docker network ls --format '{{.Name}}' | grep -Eq "^${DOCKER_NETWORK}\$"; then 
	echo "Creating Docker network: $DOCKER_NETWORK"
	docker network create "$DOCKER_NETWORK" > /dev/null
else 
	echo "Docker netowkr already exists: $DOCKER_NETWORK"
fi


# ==============================
# Build Section
# ==============================
echo "Cleaning previous build: $BUILD_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "Configuring with CMake..."
# Prefer Ninja if available, else fall back to Unix Makefiles on Linux/mac.
GENERATOR=""
if command -v ninja >/dev/null 2>&1; then
  GENERATOR="Ninja"
elif [[ "$PLATFORM" == "linux" || "$PLATFORM" == "mac" ]]; then
  GENERATOR="Unix Makefiles"
fi

if [[ -n "$GENERATOR" ]]; then
  cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G "$GENERATOR"
else
  # Let CMake choose a default generator (common on Windows with VS)
  cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"
fi

echo "Building..."
cmake --build "$BUILD_DIR" -- -j"$CPU_CORES"

# ==============================
# Docker Build Section
# ==============================
for i in "${!EXEC_NAMES[@]}"; do
  EXEC_NAME="${EXEC_NAMES[$i]}"
  DOCKERFILE="${DOCKERFILES[$i]}"
  IMAGE_NAME="${IMAGE_NAMES[$i]}"
  CONTAINER_NAME="${EXEC_NAME,,}"

  echo ""
  echo "=============================="
  echo "Building target: $EXEC_NAME"
  echo "=============================="

  EXEC_PATH="$(find "$BUILD_DIR" -type f -name "${EXEC_NAME}${EXEC_EXT}" 2>/dev/null | head -n 1 || true)"
  if [[ -z "$EXEC_PATH" ]]; then
    echo "Executable not found for $EXEC_NAME in $BUILD_DIR"
    continue
  fi

  echo "Found executable: $EXEC_PATH"
  cp "$EXEC_PATH" "$PROJECT_ROOT/${EXEC_NAME}${EXEC_EXT}"

  DOCKERFILE_PATH="$PROJECT_ROOT/$DOCKERFILE"
  if [[ ! -f "$DOCKERFILE_PATH" ]]; then
    echo "Dockerfile not found: $DOCKERFILE_PATH"
    continue
  fi

  echo "Building Docker image: $IMAGE_NAME"
  docker build -t "$IMAGE_NAME" -f "$DOCKERFILE_PATH" "$PROJECT_ROOT"

  # Remove old container if it exists
  if docker ps -a --format '{{.Names}}' | grep -Eq "^${CONTAINER_NAME}\$"; then
    echo "Removing existing container: $CONTAINER_NAME"
    docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true
  fi

  # Conditional port exposure
  if [[ "$CONTAINER_NAME" == "dashboard" ]]; then
    echo "Running with ports exposed for $EXEC_NAME"
    docker run -d \
      -p 8080:8080 \
      -p 8081:8081 \
      --network "$DOCKER_NETWORK" \
      --name "$CONTAINER_NAME" \
      -e LOG_LEVEL=info \
      "$IMAGE_NAME"
    echo "Dashboard ports: ws://localhost:8080 (WS/telemetry), http://localhost:8081 (commands)"
  else
    echo "Running container without port exposure: $CONTAINER_NAME"
    docker run -d \
	--network "$DOCKER_NETWORK" \
      --name "$CONTAINER_NAME" \
      -e LOG_LEVEL=info \
      "$IMAGE_NAME"
  fi

  echo "Done for $EXEC_NAME"
done

echo "Finished deploying."
exit 0;
