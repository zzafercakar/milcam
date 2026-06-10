# scripts/build-host.sh
#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host -j"$(nproc)"
ctest --test-dir build-host --output-on-failure
