#!/usr/bin/env bash
# scripts/build-app-arm64.sh
#
# First-light cross-build of the MilCAM Qt Widgets GUI for the VEC-VE panel
# (aarch64, Qt 5.12.4, linuxfb). This is a deliberately DIRECT g++ invocation,
# not CMake find_package(Qt5): the device ships only Qt runtime .so files (no
# CMake config, no dev symlinks, no headers), so the usual imported-target flow
# can't resolve the right-arch Qt. Instead we:
#   * compile against Qt 5.12.4 *headers* from a desktop aqt install (arch-neutral, LP64),
#   * run that desktop Qt's *moc* on the host (x86_64 tool),
#   * link directly against the *device's* real libQt5*.so.5 via `-l:` (true ABI),
#   * static-link libstdc++/libgcc to dodge the device's older GCC-8 libstdc++.
# Qt + glibc stay dynamic and resolve from the device's /usr/lib at runtime.
#
# When a proper VEC-VE sysroot + matching Qt CMake package is built (P0.3 full),
# this script is superseded by a CMake toolchain build of the `milcam` target.
set -euo pipefail

# CRITICAL: use a glibc<=2.29 cross-compiler, NOT the host's aarch64-linux-gnu-g++
# (gcc 13 / glibc 2.39). The host compiler pulls GLIBC_2.32+ symbol versions the
# device's glibc 2.29 lacks ("version `GLIBC_2.34' not found" at runtime). The
# Bootlin 2018.11 aarch64 toolchain (gcc 7.3 / glibc 2.27) links against 2.27,
# which runs fine on 2.29 (older symbol versions are forward-compatible).
# See .ai/TOOLCHAIN_NOTES.md. Override CXX/DESKTOP/SYSROOT via env if relocated.
TC=${TC:-$HOME/milcam-sysroot/aarch64--glibc--stable-2018.11-1}
CXX=${CXX:-$TC/bin/aarch64-linux-g++}
DESKTOP=${DESKTOP:-$HOME/milcam-sysroot/qt-desktop/5.12.4/gcc_64}
SYSROOT=${SYSROOT:-$HOME/milcam-sysroot/vec-ve}
MOC=${MOC:-$DESKTOP/bin/moc}
OUT=${OUT:-build-app-arm64}
APP=milcam/app/src

command -v "$CXX" >/dev/null 2>&1 || { echo "ERROR: glibc<=2.29 cross-g++ not found at: $CXX"; echo "Fetch the Bootlin 2018.11 aarch64 toolchain (see .ai/TOOLCHAIN_NOTES.md) or set CXX/TC."; exit 1; }
[ -f "$DESKTOP/include/QtCore/qconfig.h" ] || { echo "ERROR: Qt 5.12.4 headers not found at: $DESKTOP/include (aqt install qtbase). See .ai/TOOLCHAIN_NOTES.md"; exit 1; }
[ -f "$SYSROOT/usr/lib/libQt5Widgets.so.5" ] || { echo "ERROR: device Qt libs not found at: $SYSROOT/usr/lib (pull from device). See .ai/TOOLCHAIN_NOTES.md"; exit 1; }

mkdir -p "$OUT"

QT_INC="$DESKTOP/include"
QT_LIBDIR_USR="$SYSROOT/usr/lib"

echo "=== moc (host x86_64 tool) ==="
"$MOC" "$APP/gui/MainWindow.h" -o "$OUT/moc_MainWindow.cpp"

# IMPORTANT linker notes:
#  * Point -L / -rpath-link ONLY at the device's /usr/lib (Qt + its non-core
#    deps: libpng, freetype, ...). Do NOT add the device's /lib — its
#    libc/libpthread/libdl reference GLIBC_PRIVATE symbols that only its own
#    libc-2.29 provides; mixing them with the host's glibc-2.39 breaks the link.
#    Let the host cross-toolchain satisfy libc/pthread/dl at LINK time; the
#    device resolves them (2.29) at RUNTIME.
#  * --allow-shlib-undefined: tolerate the device Qt .so files referencing libc/
#    libstdc++ symbols we don't resolve here (resolved on-device at runtime).
#  * -l:libFoo.so.5 links the versioned runtime soname directly (device has no
#    unversioned dev symlinks).
echo "=== cross-compile + link (aarch64) ==="
"$CXX" -std=c++17 -O2 -march=armv8-a -mtune=cortex-a53 -fPIC \
    -DQT_NO_DEBUG \
    -I"$QT_INC" \
    -I"$QT_INC/QtCore" -I"$QT_INC/QtGui" -I"$QT_INC/QtWidgets" \
    -I"$APP" \
    "$APP/main.cpp" "$APP/gui/MainWindow.cpp" "$OUT/moc_MainWindow.cpp" \
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
