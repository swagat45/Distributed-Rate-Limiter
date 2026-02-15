#!/bin/bash
# Dependency installation script for MSYS2 (Windows)
# This script installs all required dependencies for the Distributed Rate Limiter

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Installing Dependencies for Distributed Rate Limiter (MSYS2)  ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Update package manager
echo "[1/5] Updating package manager..."
pacman -Syy

# Core build tools
echo "[2/5] Installing core build tools..."
pacman -S --noconfirm \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake \
  git

# gRPC and Protobuf
echo "[3/5] Installing gRPC and Protobuf..."
pacman -S --noconfirm \
  mingw-w64-x86_64-grpc \
  mingw-w64-x86_64-protobuf

# Redis client library
echo "[4/5] Installing Redis client library (hiredis)..."
pacman -S --noconfirm \
  mingw-w64-x86_64-hiredis

# Testing framework
echo "[5/5] Installing Google Test..."
pacman -S --noconfirm \
  mingw-w64-x86_64-gtest

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Additional Setup: Prometheus C++ from Source                  ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check if prometheus-cpp is available in pacman
if pacman -Si mingw-w64-x86_64-prometheus-cpp &>/dev/null; then
  echo "[6/6] Installing prometheus-cpp from repository..."
  pacman -S --noconfirm mingw-w64-x86_64-prometheus-cpp
else
  echo "[6/6] Building prometheus-cpp from source..."
  echo "     (repository package not available)"
  
  PROM_BUILD_DIR="/tmp/prometheus-cpp-build"
  mkdir -p "$PROM_BUILD_DIR"
  cd "$PROM_BUILD_DIR"
  
  if [ ! -d "prometheus-cpp" ]; then
    git clone https://github.com/jupp0r/prometheus-cpp.git
  fi
  
  cd prometheus-cpp
  mkdir -p build
  cd build
  
  cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_TESTING=OFF
  
  make -j$(nproc)
  make install
  
  echo "✓ prometheus-cpp installed successfully"
fi

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  ✅ All dependencies installed successfully!                   ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "Verification:"
cmake --version
echo ""
protoc --version
echo ""
echo "Next steps:"
echo "  1. Create build directory: mkdir build && cd build"
echo "  2. Configure CMake:        cmake .."
echo "  3. Build project:          make -j$(nproc)"
echo "  4. Run tests:              ctest --output-on-failure"
echo ""
