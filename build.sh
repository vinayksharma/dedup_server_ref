#!/bin/bash
set -e

mkdir -p build
cd build
cmake ..
make -j$(nproc || sysctl -n hw.ncpu) 