#!/usr/bin/env bash
# scripts/build-app-arm64.sh
#
# Cross-build of the MilCAM Qt Widgets GUI for the VEC-VE panel (aarch64,
# Qt 5.12.4, linuxfb). Deliberately a DIRECT g++ invocation, not CMake
# find_package(Qt5): the device ships only Qt runtime .so files (no CMake config,
# no dev symlinks, no headers), so the usual imported-target flow can't resolve
# the right-arch Qt. Instead we:
#   * compile against Qt 5.12.4 *headers* from a desktop aqt install (arch-neutral, LP64),
#   * run that desktop Qt's *moc*/*rcc* on the host (x86_64 tools),
#   * link directly against the *device's* real libQt5*.so.5 via `-l:` (true ABI),
#   * static-link libstdc++/libgcc to dodge the device's older GCC-8 libstdc++.
# Qt + glibc stay dynamic and resolve from the device's /usr/lib at runtime.
# See .ai/TOOLCHAIN_NOTES.md (P0.3). A CMake-sysroot build supersedes this later.
set -euo pipefail

# CRITICAL: use a glibc<=2.29 cross-compiler, NOT the host's aarch64-linux-gnu-g++
# (gcc 13 / glibc 2.39 -> pulls GLIBC_2.32+ the device's 2.29 lacks). Bootlin
# 2018.11 (gcc 7.3 / glibc 2.27) links against 2.27, which runs on 2.29.
TC=${TC:-$HOME/milcam-sysroot/aarch64--glibc--stable-2018.11-1}
CXX=${CXX:-$TC/bin/aarch64-linux-g++}
DESKTOP=${DESKTOP:-$HOME/milcam-sysroot/qt-desktop/5.12.4/gcc_64}
SYSROOT=${SYSROOT:-$HOME/milcam-sysroot/vec-ve}
MOC=${MOC:-$DESKTOP/bin/moc}
RCC=${RCC:-$DESKTOP/bin/rcc}
OUT=${OUT:-build-app-arm64}
APP=milcam/app/src
CORE=milcam/core

command -v "$CXX" >/dev/null 2>&1 || { echo "ERROR: glibc<=2.29 cross-g++ not found at: $CXX"; echo "Fetch the Bootlin 2018.11 aarch64 toolchain (see .ai/TOOLCHAIN_NOTES.md) or set CXX/TC."; exit 1; }
[ -f "$DESKTOP/include/QtCore/qconfig.h" ] || { echo "ERROR: Qt 5.12.4 headers not found at: $DESKTOP/include (aqt install qtbase). See .ai/TOOLCHAIN_NOTES.md"; exit 1; }
[ -f "$SYSROOT/usr/lib/libQt5Widgets.so.5" ] || { echo "ERROR: device Qt libs not found at: $SYSROOT/usr/lib (pull from device). See .ai/TOOLCHAIN_NOTES.md"; exit 1; }
[ -x "$RCC" ] || { echo "ERROR: rcc not found at: $RCC"; exit 1; }

mkdir -p "$OUT"

QT_INC="$DESKTOP/include"
QT_LIBDIR_USR="$SYSROOT/usr/lib"

# Every Q_OBJECT header needs a moc translation unit.
QOBJECTS="MainWindow TopBar HmiSwitch OperationToolBar CanvasView JobPanel StatusBar"
echo "=== moc (host x86_64 tool) ==="
MOC_SRCS=""
for h in $QOBJECTS; do
    "$MOC" "$APP/gui/$h.h" -o "$OUT/moc_$h.cpp"
    MOC_SRCS="$MOC_SRCS $OUT/moc_$h.cpp"
done

echo "=== rcc (host x86_64 tool) ==="
"$RCC" milcam/app/resources/milcam.qrc -o "$OUT/qrc_milcam.cpp"

echo "=== cross-compile + link (aarch64) ==="
# Linker notes:
#  * -L/-rpath-link ONLY the device's /usr/lib (Qt + libpng/freetype/...). NOT
#    /lib — its libc/pthread/dl carry GLIBC_PRIVATE symbols only its own 2.29
#    provides; the cross-toolchain supplies libc/pthread/dl at link time, the
#    device resolves them at runtime.
#  * --allow-shlib-undefined: tolerate device Qt .so referencing libc/libstdc++
#    symbols not resolved here (resolved on-device).
#  * -l:libFoo.so.5 links the versioned soname directly (no dev symlinks on device).
"$CXX" -std=c++17 -O2 -march=armv8-a -mtune=cortex-a53 -fPIC \
    -DQT_NO_DEBUG \
    -I"$QT_INC" \
    -I"$QT_INC/QtCore" -I"$QT_INC/QtGui" -I"$QT_INC/QtWidgets" \
    -I"$APP" -I"$CORE" \
    "$APP/main.cpp" \
    "$APP/gui/MainWindow.cpp" "$APP/gui/TopBar.cpp" "$APP/gui/HmiSwitch.cpp" \
    "$APP/gui/OperationToolBar.cpp" "$APP/gui/CanvasView.cpp" \
    "$APP/gui/JobPanel.cpp" "$APP/gui/StatusBar.cpp" \
    $MOC_SRCS "$OUT/qrc_milcam.cpp" \
    -L"$QT_LIBDIR_USR" -Wl,-rpath-link,"$QT_LIBDIR_USR" \
    -Wl,--allow-shlib-undefined \
    -l:libQt5Widgets.so.5 -l:libQt5Gui.so.5 -l:libQt5Core.so.5 \
    -static-libstdc++ -static-libgcc \
    -pthread \
    -o "$OUT/milcam"

echo "--- artifact ---"
file "$OUT/milcam"
echo "--- NEEDED libs ---"
aarch64-linux-gnu-objdump -p "$OUT/milcam" | grep NEEDED || true
echo "--- glibc symbol versions (want <= 2.27) ---"
aarch64-linux-gnu-objdump -T "$OUT/milcam" | grep -oE "GLIBC_[0-9.]+" | sort -uV | tail -3 || true
