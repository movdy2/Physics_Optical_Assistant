#!/usr/bin/env bash
# Arch Linux build script for PHY104 Optical Assistant v3.0.0
# Installs build dependencies, configures, builds, and tests the project.
set -euo pipefail

echo "=== PHY104 Optical Assistant — Arch Linux build script ==="
echo "Version: 3.0.0"
echo

# 1. Install build dependencies via pacman
echo "[1/4] Installing build dependencies (pacman)..."
sudo pacman -Syu --needed --noconfirm \
    base-devel cmake ninja qt6-base qt6-svg mesa libegl

# 2. Configure the project
echo "[2/4] Configuring with CMake + Ninja..."
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. Build
echo "[3/4] Building..."
cmake --build build --parallel

# 4. Run tests
echo "[4/4] Running tests..."
ctest --test-dir build --output-on-failure

echo
echo "=== Build complete ==="
echo "Run the app: ./build/src/PHY104OpticalAssistant"
