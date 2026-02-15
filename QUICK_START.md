# 🚀 Quick Start Guide - Distributed Rate Limiter Setup

This guide provides multiple paths to set up the project based on your preferences.

---

## ⚡ FASTEST OPTION: Docker (Recommended)

If you just want to **run** the rate limiter without dealing with native dependencies:

### Prerequisites:
- Docker Desktop installed (https://www.docker.com/products/docker-desktop)

### Steps:
```bash
cd "d:\Swagat Suman Mishra\CORE PROJECTS\Distributed-Rate-Limiter"

# Start the entire stack in containers
docker-compose up --build
```

### What you get automatically:
- ✅ Rate Limiter gRPC server on `localhost:50051`
- ✅ Redis database
- ✅ Prometheus metrics on `http://localhost:9090`
- ✅ Grafana dashboard on `http://localhost:3000` (admin/admin)
- ✅ All dependencies pre-configured

**Time: 3-5 minutes first run, 30 seconds on subsequent runs**

---

## 🛠️ LOCAL DEVELOPMENT: Native Compilation

If you want to **develop locally** with full IDE support:

### Step 1: Install Visual Studio Build Tools (ONE-TIME ONLY)

1. Download: https://visualstudio.microsoft.com/downloads/
2. Get "Build Tools for Visual Studio 2022"
3. Run: `vs_BuildTools.exe`
4. Install "Desktop development with C++"
5. Time: ~15 minutes

✅ After installation, you have a C++ compiler!

### Step 2: Run Automated Setup

```bash
cd "d:\Swagat Suman Mishra\CORE PROJECTS\Distributed-Rate-Limiter"
bash scripts/install-deps-vcpkg.sh
```

This automatically:
- ✅ Clones vcpkg (Microsoft's C++ package manager)
- ✅ Downloads & compiles gRPC, Protobuf, hiredis, gtest
- ✅ Configures CMake integration
- ✅ Time: 20-30 minutes first run

### Step 3: Build the Project

```bash
cd "d:\Swagat Suman Mishra\CORE PROJECTS\Distributed-Rate-Limiter"
mkdir build && cd build

# Replace C:\Users\LENOVO\vcpkg with your actual vcpkg path if different
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\Users\LENOVO\vcpkg\scripts\buildsystems\vcpkg.cmake

cmake --build . --config Release --parallel 4

# Run tests
ctest --output-on-failure
```

### Step 4: Deploy Remaining Services (Redis, Prometheus, Grafana)

Option A - Use Docker for supporting services:
```bash
docker-compose up redis prometheus grafana
```

Option B - Install Redis locally:
1. Download: https://github.com/microsoftarchive/redis/releases/tag/win-3.2.100
2. Run `redis-server.exe`

---

## 📋 Dependency Checklist

Check which dependencies are installed:

```bash
cmake --version              # Should show 3.16+
cl.exe                       # Visual Studio compiler
git --version                # Version control
```

### Expected Output After Installation:

✅ `cmake --version` → cmake version 3.27.0 or higher
✅ `cl.exe` → Microsoft (R) C/C++ Optimizing Compiler
✅ `git --version` → git version 2.xx.x

---

## 🐛 Troubleshooting

### Problem: "Visual Studio not found"
**Solution**: Install Build Tools (see Step 1 above)

### Problem: "cmake not found"
**Solution**: Either:
1. Install CMake standalone from https://cmake.org/download/
2. Let Visual Studio Build Tools install it for you
3. Use Docker instead

### Problem: "vcpkg taking too long"
**Solution**: This is normal! First compilation is slow (20-30 min).
Check your Task Manager - VCP should show active compilation.

### Problem: "CMakeLists.txt errors"
**Solution**: Make sure to use `-DCMAKE_TOOLCHAIN_FILE` with vcpkg:
```
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\Users\LENOVO\vcpkg\scripts\buildsystems\vcpkg.cmake
```

---

## 📊 Comparison: Docker vs Local Development

| Aspect | Docker | Local |
|--------|--------|-------|
| **Setup Time** | 5 min | 45-60 min |
| **Storage** | ~1 GB | ~3-5 GB |
| **IDE Support** | ❌ No | ✅ Yes |
| **Hot Reload** | ❌ No | ✅ Yes |
| **Debugging** | ❌ No | ✅ Yes |
| **Just Run It** | ✅ Yes | ❌ No |
| **Production** | ✅ Better | ❌ OS-dependent |

---

## 📁 Project Structure

```
Distributed-Rate-Limiter/
├── src/                    # C++ source code
│   ├── server.cpp         # gRPC server
│   ├── redis_store.*      # Redis integration
│   ├── metrics.*          # Prometheus metrics
│   └── token_bucket.h     # Token bucket algorithm
├── tests/                 # Unit tests
├── proto/                 # gRPC definitions
├── docker/                # Docker configuration
│   └── Dockerfile         # Builds entire stack
├── docker-compose.yml     # Orchestrates all services
├── CMakeLists.txt         # Build configuration
├── scripts/
│   ├── install-deps-vcpkg.sh    # Automated vcpkg setup
│   └── install-deps-msys2.sh    # MSYS2 setup (if available)
└── README.md              # Main documentation
```

---

## 🎯 Next Steps

Choose ONE approach:

### For Quick Testing (Recommended):
```bash
docker-compose up --build
# Access http://localhost:3000
```

### For Local Development:
```bash
# 1. Install Visual Studio Build Tools
# 2. bash scripts/install-deps-vcpkg.sh
# 3. mkdir build && cd build
# 4. cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\Users\LENOVO\vcpkg\scripts\buildsystems\vcpkg.cmake
# 5. cmake --build . --config Release --parallel 4
```

### For Detailed Information:
- See **DEPENDENCY_SETUP.md** for detailed dependency instructions
- See **WINDOWS_SETUP.md** for Windows-specific notes
- See **README.md** for project overview

---

## 🆘 Still Stuck?

1. **Docker Path**: Use Docker - it handles everything
2. **vcpkg Errors**: Run Visual Studio Build Tools installer
3. **Build Errors**: Check that you have the `-DCMAKE_TOOLCHAIN_FILE` flag
4. **Path Issues**: Adjust paths to match your vcpkg installation directory

**Most Common Issue**: Visual Studio Build Tools not installed. Install it first, then everything else works!

---

## 📞 Support Resources

- **Visual Studio Build Tools**: https://docs.microsoft.com/en-us/visualstudio/
- **vcpkg for Windows**: https://github.com/Microsoft/vcpkg
- **CMake**: https://cmake.org/documentation/
- **Docker**: https://docs.docker.com/
- **gRPC C++ Setup**: https://grpc.io/docs/languages/cpp/quickstart/

Good luck! 🚀
