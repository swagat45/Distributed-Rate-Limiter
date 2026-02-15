#!/bin/bash
# Automated vcpkg setup and dependency installation for Windows
# This script clones vcpkg, bootstraps it, and installs all required dependencies

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
VCPKG_ROOT="${HOME}/vcpkg"

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Distributed Rate Limiter - Dependency Setup (vcpkg)          ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check if Git is available
if ! command -v git &> /dev/null; then
  echo "❌ Git is required but not installed."
  echo "   Download from: https://git-scm.com/download/win"
  exit 1
fi

# Step 1: Clone vcpkg if not already present
if [ ! -d "$VCPKG_ROOT" ]; then
  echo "[1/4] Cloning vcpkg repository..."
  git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_ROOT"
else
  echo "[1/4] vcpkg already present at $VCPKG_ROOT"
fi

# Step 2: Bootstrap vcpkg
echo "[2/4] Bootstrapping vcpkg..."
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
  # Windows batch script
  "$VCPKG_ROOT/bootstrap-vcpkg.bat" -disableMetrics
else
  # Unix-like shells (Git Bash, MSYS2)
  bash "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics
fi

# Step 3: Install dependencies
echo "[3/4] Installing C++ dependencies with vcpkg..."
echo "     (This may take 5-10 minutes on first run)"
echo ""

"$VCPKG_ROOT/vcpkg" install \
  grpc:x64-windows \
  protobuf:x64-windows \
  hiredis:x64-windows \
  gtest:x64-windows \

# Step 4: Display next steps
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  ✅ Dependencies installed successfully!                       ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "vcpkg Installation Directory: $VCPKG_ROOT"
echo ""
echo "Next steps - Build the Rate Limiter:"
echo ""
echo "  $ cd '$PROJECT_ROOT'"
echo "  $ mkdir -p build && cd build"
echo "  $ cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
echo "  $ cmake --build . --config Release --parallel 4"
echo "  $ ctest --output-on-failure"
echo ""
echo "Or use Docker (no Windows dependencies needed):"
echo "  $ docker-compose up --build"
echo ""
