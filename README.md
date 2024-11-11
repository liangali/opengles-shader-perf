# OpenGL ES Shader Performance Test

This project implements an ANGLE-based OpenGL ES shader performance test program with the following features:
- Uses OpenGL ES API to run on Windows through the ANGLE framework
- Implements NV12 to ARGB color space conversion shader
- Performs performance testing at 4K resolution
- Records shader execution time (average/minimum/maximum)

## Build Requirements
- Visual Studio 2022
- CMake 3.15+
- ANGLE SDK

## Build Steps
1. Install ANGLE/OGL/EGL libraries through vcpkg

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat # For Windows
vcpkg install angle
```

2. build the test app

```bash
mkdir build
cd build

# or run satvars.bat for setting environment variables
set ANGLE_DIR=C:\data\code\opengles_angle_shader\vcpkg\packages\angle_x64-windows
set EGL_DIR=C:\data\code\opengles_angle_shader\vcpkg\packages\egl-registry_x64-windows
set OGL_DIR=C:\data\code\opengles_angle_shader\vcpkg\packages\opengl-registry_x64-windows
set ZLIB_DIR=C:\data\code\opengles_angle_shader\vcpkg\packages\zlib_x64-windows

cmake ..
cmake --build . --config Release
```

## Usage

The application supports the following command line arguments:

### Command Line Options

- `--gpu <index>`: Select GPU adapter by index. If not specified, the application will use GPU 0 by default.
- `--verbose`: Enable verbose debug logging during EGL initialization and other critical sections. Useful for debugging GPU selection and initialization issues.
- `--help`: Show detailed command line usage information.

```bash

# List all available GPUs
opengles-shader-perf.exe --list-gpus

# Run the test on a specific GPU (e.g., GPU 1)
opengles-shader-perf.exe --gpu 1
```

If no GPU is specified, the application will use GPU 0 by default.
