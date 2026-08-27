#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."

cmake -S . -B build
cmake --build build -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
cp build/gui gui
