# Build Instructions

## Prerequisites

### Linux (Fedora/RHEL)
```bash
sudo dnf install alsa-lib-devel glfw-devel cmake g++
```

### Linux (Debian/Ubuntu)
```bash
sudo apt install libasound2-dev libglfw3-dev cmake g++
```

### Windows
- Install [CMake](https://cmake.org/download/)
- Install [MinGW-w64](https://www.mingw-w64.org/) or Visual Studio
- GLFW is bundled in the repository

### macOS
```bash
brew install cmake glfw
```

## Building with CMake

### Linux/macOS
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
./gui
```

### Windows (MinGW)
```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
gui.exe
```

### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
Release\gui.exe
```

## Building with build.bat (Windows only)

The legacy build script is still available (run from anywhere, it `cd`s to the repo root itself):
```bash
scripts\build.bat
```

## Project Structure

```
NoteDetector/
├── src/
│   ├── main.cpp                     # Main application entry point
│   ├── gui.hpp                      # GUI initialization functions
│   ├── audio/                       # Audio analysis: FFT, peak 
│   ├── piano/                       # Piano keyboard visualization widget
│   └── util/                        # Misc helpers (texture loading, ...)
├── libraries/
│   ├── imgui/                       # Dear ImGui library
│   ├── implot/                      # ImPlot plotting library
│   ├── rtAudio/                     # Cross-platform audio I/O
│   ├── GLFW/                        # Window/input library (Windows only)
│   └── filedialog/                  # tinyfiledialogs
├── assets/                          # Images and sample sounds
├── scripts/                         # build.bat / clean.bat
└── docs/                            # This file
```

## Troubleshooting

### Missing GLFW on Linux
If CMake can't find GLFW via pkg-config, install the development package:
- Fedora: `sudo dnf install glfw-devel`
- Ubuntu: `sudo apt install libglfw3-dev`

### Missing ALSA on Linux
Install ALSA development libraries:
- Fedora: `sudo dnf install alsa-lib-devel`
- Ubuntu: `sudo apt install libasound2-dev`

### OpenGL Errors
Make sure you have OpenGL drivers installed for your GPU.
