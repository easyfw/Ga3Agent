# Build Instructions for Ga1Agent

## Prerequisites

### Windows Development Environment

You need one of the following:

1. **Visual Studio 2015 or later**
   - Install Visual Studio with C++ Desktop Development workload
   - Includes MSVC compiler and Windows SDK

2. **CMake 3.10+**
   - Download from https://cmake.org/download/
   - Add to PATH during installation

3. **MinGW (Optional alternative)**
   - Download from https://www.mingw-w64.org/
   - For GCC-based compilation

## Build Methods

### Method 1: Visual Studio IDE

1. Open `Ga1Agent.vcxproj` in Visual Studio
2. Select **Win32** platform from the configuration dropdown
3. Select **Release** or **Debug** configuration
4. Build > Build Solution (or press F7)
5. Output: `bin/Release/Ga1Agent.exe` or `bin/Debug/Ga1Agent.exe`

### Method 2: Visual Studio Developer Command Prompt

```cmd
# Open "Developer Command Prompt for VS"
# Navigate to project directory
cd C:\path\to\Ga1Agent

# Build using nmake
nmake /f Makefile.win

# Or build using MSBuild
msbuild Ga1Agent.vcxproj /p:Configuration=Release /p:Platform=Win32
```

### Method 3: CMake (Recommended for cross-platform)

```cmd
# Create build directory
mkdir build
cd build

# Generate Visual Studio solution (32-bit)
cmake -G "Visual Studio 16 2019" -A Win32 ..

# Or for Visual Studio 2017
cmake -G "Visual Studio 15 2017" -A Win32 ..

# Build
cmake --build . --config Release

# Output: build/bin/Ga1Agent.exe
```

### Method 4: CMake with Ninja (Faster builds)

```cmd
# Open "Developer Command Prompt for VS"
mkdir build
cd build

# Configure with Ninja generator (32-bit)
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..

# Build
ninja

# Output: build/bin/Ga1Agent.exe
```

## Platform-Specific Notes

### 32-bit vs 64-bit

**Important**: OPC DA is a 32-bit COM technology. For best compatibility:

- **Build as 32-bit** (Win32 platform) - Recommended
- If using 64-bit build, ensure OPC clients use WOW64 compatibility

**Force 32-bit in CMake:**
```cmd
cmake -A Win32 ..
```

**Force 32-bit in Visual Studio:**
- Select "Win32" from Platform dropdown (not x64)

### Windows SDK Version

If you encounter SDK errors, specify the version:

```cmd
# List installed SDKs
dir "C:\Program Files (x86)\Windows Kits\10\Include"

# Use specific SDK version in CMake
cmake -DCMAKE_SYSTEM_VERSION=10.0.19041.0 ..
```

## Quick Start Build Script

Create `build.bat` in the project root:

```batch
@echo off
echo Building Ga1Agent (32-bit Release)...

if not exist build mkdir build
cd build

cmake -G "Visual Studio 16 2019" -A Win32 ..
if errorlevel 1 goto error

cmake --build . --config Release
if errorlevel 1 goto error

echo.
echo Build successful!
echo Executable: build\bin\Ga1Agent.exe
goto end

:error
echo Build failed!
exit /b 1

:end
```

Run: `build.bat`

## Troubleshooting

### Error: "Windows.h not found"

- Install Windows SDK via Visual Studio Installer
- Or download standalone Windows SDK

### Error: "Cannot open include file: 'ole32.lib'"

- Ensure you're using Developer Command Prompt
- Or set environment variables:
  ```cmd
  call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
  ```

### Error: Platform mismatch

- Ensure you're building for Win32 (32-bit), not x64
- Clean build directory: `rmdir /s /q build`

### CMake can't find Visual Studio

```cmd
# Manually specify generator
cmake -G "Visual Studio 16 2019" -A Win32 ..

# Or list available generators
cmake --help
```

## Verification

After successful build:

```cmd
# Check if executable exists
dir bin\Release\Ga1Agent.exe

# Or in CMake build
dir build\bin\Ga1Agent.exe

# Test the executable
bin\Release\Ga1Agent.exe
# Should display usage information
```

## Clean Build

### Visual Studio
```cmd
# Clean solution
msbuild Ga1Agent.vcxproj /t:Clean
```

### CMake
```cmd
# Remove build directory
rmdir /s /q build
```

### nmake
```cmd
nmake /f Makefile.win clean
```

## Distribution Package

After building, create a distribution package:

```cmd
mkdir dist
copy bin\Release\Ga1Agent.exe dist\
copy config\tags.ini dist\
copy README.md dist\
```

## Advanced Build Options

### Debug Build with Symbols

```cmd
cmake --build . --config Debug
```

### Static Linking (no runtime dependencies)

Add to CMakeLists.txt:
```cmake
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
```

### Optimized Release Build

Add compiler flags in CMakeLists.txt:
```cmake
if(MSVC)
  add_compile_options(/O2 /GL)
  add_link_options(/LTCG)
endif()
```

## Continuous Integration

For automated builds, use this script in CI:

```batch
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
if errorlevel 1 exit /b 1
```
