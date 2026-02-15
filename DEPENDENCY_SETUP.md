# Dependency Installation - CRITICAL REQUIREMENT

## ⚠️ Visual Studio Build Tools Required

Your system is missing **Visual Studio Build Tools**, which are required to compile C++ code on Windows.

### What You Need to Install

You have **two options**:

---

## Option A: Visual Studio Build Tools (Lightweight - Recommended)

This is the minimal C++ development environment for Windows.

### Installation Steps:

1. **Download Visual Studio Build Tools 2022**
   - Go to: https://visualstudio.microsoft.com/downloads/
   - Scroll to "All Downloads" → "Tools for Visual Studio 2022"
   - Click "Download" under "Build Tools for Visual Studio 2022"

2. **Run the installer**
   ```
   vs_BuildTools.exe
   ```

3. **Custom Installation** - Select the following:
   - ✅ **Desktop development with C++**
   - ✅ **Windows SDK** (for your Windows version)
   - ✅ **CMake tools for Windows**

4. **Complete installation** (may take 10-15 minutes)

5. **Verify installation**
   ```bash
   cl.exe
   cmake --version
   ```

### After Installation: Proceed with vcpkg

Once Visual Studio Build Tools are installed, run:

```bash
cd d:\Swagat Suman Mishra\CORE PROJECTS\Distributed-Rate-Limiter
bash scripts/install-deps-vcpkg.sh
```

---

## Option B: Full Visual Studio Community Edition (Alternative)

If you prefer a complete IDE:

1. Download: https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community
2. Run installer
3. Select "Desktop development with C++"
4. Install

This gives you both the compiler AND an IDE for development.

---

## Option C: Use Docker Instead (Easiest - No Compilation Needed)

If you don't want to install Visual Studio, use Docker:

```bash
cd d:\Swagat Suman Mishra\CORE PROJECTS\Distributed-Rate-Limiter

# This automatically compiles everything in a container
docker-compose up --build

# Access:
# - Rate Limiter gRPC: localhost:50051
# - Prometheus: http://localhost:9090
# - Grafana: http://localhost:3000 (admin/admin)
```

**Advantages of Docker:**
- ✅ No native dependencies needed
- ✅ Exact reproducible environment
- ✅ Easier to manage versions
- ✅ Works on any OS (Windows, Mac, Linux)

---

## What Happens Next After Installing Build Tools

Once Visual Studio Build Tools are installed:

```bash
# vcpkg will automatically download and compile:
# - gRPC (with Protobuf)
# - hiredis (Redis client)
# - Google Test (gtest)
# - All dependencies (OpenSSL, zlib, abseil, etc.)

# Total time: 15-30 minutes on first run
# Subsequent builds: Much faster (uses cache)
```

---

## Quick Reference

| Component | Purpose | Status |
|-----------|---------|--------|
| **Visual Studio Build Tools** | C++ compiler & toolchain | ❌ **MISSING** - Install first! |
| **CMake** | Build system | Can be installed via Build Tools |
| **Git** | Version control | ✅ Installed |
| **vcpkg** | Package manager | ✅ Ready to use |
| **gRPC** | RPC framework | Installed by vcpkg |
| **Protobuf** | Message serialization | Installed by vcpkg |
| **hiredis** | Redis client | Installed by vcpkg |
| **gtest** | Unit testing | Installed by vcpkg |

---

## Summary

**Choose ONE path:**

1. **Path A (Fastest)**: Install VS Build Tools → Run vcpkg script → Build locally
2. **Path B (Easiest)**: Install Docker Desktop → Run docker-compose
3. **Path C (IDE)**: Install full Visual Studio Community → Build locally with IDE

---

## Help & Support

- Visual Studio Build Tools installation issues: https://docs.microsoft.com/en-us/visualstudio/install/install-visual-studio
- vcpkg documentation: https://github.com/Microsoft/vcpkg
- Docker documentation: https://docs.docker.com/

Once Visual Studio Build Tools are installed, **everything else is automated!**
