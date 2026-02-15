#!/bin/bash
# Linux dependency installation script with source build for prometheus-cpp
# This script installs dependencies for the Distributed Rate Limiter on Linux

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║    Installing Dependencies for Distributed Rate Limiter       ║"
echo "║                    (Linux - Ubuntu/Debian)                    ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Detect package manager
if command -v apt-get &> /dev/null; then
    PKG_MANAGER="apt"
    INSTALL_CMD="sudo apt-get install -y"
elif command -v yum &> /dev/null; then
    PKG_MANAGER="yum"
    INSTALL_CMD="sudo yum install -y"
else
    echo "❌ Unsupported package manager. Please install dependencies manually."
    exit 1
fi

echo "[1/5] Updating package manager..."
if [ "$PKG_MANAGER" = "apt" ]; then
    sudo apt-get update
else
    sudo yum update -y
fi

echo "[2/5] Installing system dependencies..."
if [ "$PKG_MANAGER" = "apt" ]; then
    $INSTALL_CMD \
        build-essential \
        cmake \
        git \
        libgrpc++-dev \
        protobuf-compiler-grpc \
        libprotobuf-dev \
        libhiredis-dev \
        libcurl4-openssl-dev \
        libssl-dev \
        googletest \
        libgtest-dev \
        pkg-config
else
    # For RedHat/CentOS
    $INSTALL_CMD \
        gcc \
        gcc-c++ \
        cmake \
        git \
        grpc-devel \
        protobuf-devel \
        hiredis-devel \
        openssl-devel \
        curl-devel \
        gtest-devel \
        pkg-config
fi

echo "[3/5] Building Prometheus C++ from source..."
PROM_BUILD_DIR="/tmp/prometheus-cpp-build"
mkdir -p "$PROM_BUILD_DIR"
cd "$PROM_BUILD_DIR"

if [ ! -d "prometheus-cpp" ]; then
    git clone --recurse-submodules https://github.com/jupp0r/prometheus-cpp.git prometheus-cpp
else
    cd prometheus-cpp
    git pull origin main
    git submodule update --init --recursive
    cd ..
fi

cd prometheus-cpp

# Ensure all submodules are properly initialized
echo "   - Initializing submodules..."
git submodule update --init --recursive

echo "   - Configuring CMake..."
mkdir -p build
cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_TESTING=OFF

echo "   - Building (this may take a few minutes)..."
make -j$(nproc)

echo "   - Installing..."
sudo make install
sudo ldconfig

cd /

echo ""
echo "[4/5] Verifying installations..."
echo "    CMake version: $(cmake --version | head -1)"
echo "    Protoc version: $(protoc --version)"
echo "    gRPC version: $((grpc_cpp_plugin --version 2>/dev/null || echo "unknown") | head -1)"

echo ""
echo "[5/5] Configuration complete!"

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  ✅ All dependencies installed successfully!                   ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "Next steps - Build the Rate Limiter:"
echo ""
echo "  $ cd <project-directory>"
echo "  $ mkdir build && cd build"
echo "  $ cmake -DCMAKE_BUILD_TYPE=Release .."
echo "  $ make -j\$(nproc)"
echo "  $ ctest --output-on-failure"
echo ""
echo "Or use Docker:"
echo "  $ docker-compose up --build"
echo ""
