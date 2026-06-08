#!/usr/bin/env bash
# Convenience: clone the FreeCAD source repo to the maintainer's default
# location so MilCAM's default preset works out of the box.

set -euo pipefail

FREECAD_DIR="${HOME}/Downloads/FreeCAD-main-1-1-git"
FREECAD_BRANCH="${FREECAD_BRANCH:-main}"

if [[ -d "${FREECAD_DIR}/.git" ]]; then
    echo "FreeCAD source already at ${FREECAD_DIR}. Pulling latest."
    git -C "${FREECAD_DIR}" pull --ff-only
else
    echo "Cloning FreeCAD source to ${FREECAD_DIR} (this is a 3+ GB checkout)…"
    mkdir -p "$(dirname "${FREECAD_DIR}")"
    git clone --depth 1 -b "${FREECAD_BRANCH}" \
        https://github.com/FreeCAD/FreeCAD.git "${FREECAD_DIR}"
fi

echo
echo "FREECAD_SOURCE_DIR -> ${FREECAD_DIR}"
echo "MilCAM's default preset already points here. Now:"
echo "  cd $(dirname "$0")/.."
echo "  cmake --preset default"
echo "  cmake --build build -j\$(nproc)"
