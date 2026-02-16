#!/bin/bash
# Mind Palette - Preprocess Server Linux Verification Script
# This script should be run inside WSL2 (Ubuntu 22.04)

set -e # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script Location: $SCRIPT_DIR"

echo "=== 🐧 Linux(WSL2) Verification Started ==="

# Check if running on a mounted Windows drive
if [[ $(pwd) == /mnt/* ]]; then
    echo "⚠️  WARNING: You are running this script on a mounted Windows drive (/mnt/*)."
    echo "Building OpenCV and other dependencies on /mnt/ can be extremely slow and may fail due to filesystem differences."
    echo "Recommended: Copy the 'preprocess-server' folder to your Linux home directory (e.g., ~/project) and run it there."
    echo ""
fi

# 1. Install System Dependencies
echo "[1/4] Installing system dependencies..."
sudo apt-get update -y
sudo apt-get install -y build-essential cmake ninja-build pkg-config curl zip unzip tar git \
    bison flex autoconf automake libtool \
    libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev libgtk-3-dev

# 2. Setup vcpkg
VCPKG_ROOT="$HOME/vcpkg"
if [ ! -d "$VCPKG_ROOT" ]; then
    echo "[2/4] Cloning vcpkg to $VCPKG_ROOT..."
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
    "$VCPKG_ROOT/bootstrap-vcpkg.sh"
else
    echo "[2/4] vcpkg already exists. Updating..."
    cd "$VCPKG_ROOT"
    git pull
    ./bootstrap-vcpkg.sh
fi

# 3. Configure & Build
echo "[3/4] Configuring and Building with CMake..."
# SCRIPT_DIR is already defined at the top
BUILD_DIR="$SCRIPT_DIR/build_linux"

# Cleanup existing build dir to avoid CMake cache issues (source/binary dir mismatch)
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Explicitly find tools to avoid 'not found' issues in some environments
NINJA_PATH=$(which ninja)
CXX_PATH=$(which g++)
CC_PATH=$(which gcc)

echo "Using CXX: $CXX_PATH"
echo "Using Ninja: $NINJA_PATH"

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$NINJA_PATH" \
    -DCMAKE_CXX_COMPILER="$CXX_PATH" \
    -DCMAKE_C_COMPILER="$CC_PATH" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR"

# 4. Run Tests
echo "[4/4] Running unit tests..."
cd "$BUILD_DIR"
ctest --output-on-failure

echo ""
echo "=== ✅ Verification Complete! ==="
echo "Linux binaries are located in: $BUILD_DIR/bin"
