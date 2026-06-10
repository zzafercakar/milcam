#!/usr/bin/env bash
# scripts/build-arm64.sh
# Cross-build the MilCAM C++ core + console tests for the VEC-VE panel (aarch64,
# statically linked). Produces build-arm64/tests/core_tests — a self-contained
# binary that runs on the device with no library dependencies.
set -euo pipefail
cmake -S . -B build-arm64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-vecve-aarch64.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64 -j"$(nproc)"
echo "--- artifact ---"
file build-arm64/tests/core_tests
