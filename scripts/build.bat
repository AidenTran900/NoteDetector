@echo off
cd /d "%~dp0\.."

cmake -S . -B build -G "MinGW Makefiles" || exit /b 1
cmake --build build || exit /b 1
copy /Y build\gui.exe gui.exe
