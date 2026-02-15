# Dependency Installation Guide for Windows

## Option 1: Using vcpkg (Recommended for Windows)

vcpkg is Microsoft's C++ package manager for Windows. This is the easiest method.

### Step 1: Install vcpkg

```bash
# Clone vcpkg repository
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Run bootstrap script
./bootstrap-vcpkg.bat

# (Optional) Add vcpkg to system PATH for easier access
# On Windows, edit your system environment variables
```

### Step 2: Install Dependencies

```bash
# From the vcpkg directory:
./vcpkg install grpc:x64-windows protobuf:x64-windows hiredis:x64-windows gtest:x64-windows cmake:x64-windows

# For a debug build as well:
./vcpkg install grpc:x64-windows-static protobuf:x64-windows-static hiredis:x64-windows-static gtest:x64-windows-static
```

### Step 3: Build the Project with vcpkg

```bash
cd your-project-directory
mkdir build && cd build

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build . --config Release
```

---

## Option 2: Using Chocolatey (Alternative)

If you have Chocolatey installed:

```powershell
# Run PowerShell as Administrator, then:

choco install cmake --version=3.27.0
choco install protoc
choco install mingw

# For gRPC and others, you may need to build from source
```

---

## Option 3: Manual Installation (Advanced)

### Prerequisites to Download Manually:

1. **CMake** (3.16+)
   - Download: https://cmake.org/download/
   - Install and add to PATH

2. **Visual Studio Build Tools** or **MinGW**
   - Visual Studio: https://visualstudio.microsoft.com/downloads/
   - MinGW: https://www.mingw-w64.org/

3. **Git** (if not already installed)
   - Download: https://git-scm.com/download/win

4. **gRPC** - Build from source
   ```bash
   git clone --recurse-submodules -b v1.60.0 https://github.com/grpc/grpc.git
   cd grpc
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release --parallel 4
   cmake --install . --config Release
   ```

5. **Protobuf** - Usually comes with gRPC
   - Or download separately: https://github.com/protocolbuffers/protobuf/releases

6. **hiredis** - Redis C client
   ```bash
   git clone https://github.com/redis/hiredis.git
   cd hiredis
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   cmake --install . --config Release
   ```

7. **prometheus-cpp** - Prometheus C++ client
   ```bash
   git clone https://github.com/jupp0r/prometheus-cpp.git
   cd prometheus-cpp
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
   cmake --build . --config Release --parallel 4
   cmake --install . --config Release
   ```

8. **Google Test (gtest)**
   ```bash
   git clone https://github.com/google/googletest.git
   cd googletest
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   cmake --install . --config Release
   ```

---

## Option 4: Using Docker (Easiest for Standardized Environment)

If you don't want to deal with Windows dependencies:

```bash
# Simply use docker-compose (requires Docker Desktop installed)
docker-compose up --build

# This sets up everything automatically in containers
```

---

## Verifying Installation

After installing dependencies, verify they're accessible:

```bash
cmake --version
protoc --version
grpc_cpp_plugin --version
```

If these work, you're ready to build.

---

## Building the Project

Once dependencies are installed:

```bash
cd d:\Swagat Suman Mishra\CORE PROJECTS\Distributed-Rate-Limiter

mkdir build
cd build

# Configure CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (use -j for parallel build)
cmake --build . --config Release --parallel 4

# Run tests
ctest --output-on-failure

# Or manually run
.\Debug\unit_tests.exe  # or .\Release\unit_tests.exe
```

---

## Troubleshooting

### "CMake not found"
- Install CMake from https://cmake.org/download/
- Add CMake to system PATH

### "protoc not found"
- Ensure Protobuf is installed
- Add protoc to PATH

### "Can't find grpc library"
- Make sure gRPC is fully built and installed
- Set CMAKE_PREFIX_PATH to gRPC installation directory:
  ```bash
  cmake .. -DCMAKE_PREFIX_PATH=C:\path\to\grpc\install
  ```

### Link errors
- Ensure all libraries are built for the same architecture (x64)
- Use consistent build configuration (Release vs Debug)
- Check that library paths are correctly set in CMake

---

## Recommended: Use vcpkg

**vcpkg** is the most straightforward option for Windows development as it:
- Automatically handles dependencies
- Works seamlessly with CMake
- Manages multiple configurations
- Reduces build time with pre-built binaries

Detailed vcpkg guide: https://github.com/Microsoft/vcpkg/blob/master/README.md
