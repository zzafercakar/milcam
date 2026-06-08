# MilCAM cross-compile toolchain for VEC-VE panel PC (aarch64).
#
# Cross-building FreeCAD is heavy. The sysroot at MILCAM_AARCH64_SYSROOT must
# contain matching versions of:
#   - Qt5 or Qt6 (Quick, QuickControls2, OpenGL, Svg, XmlPatterns, VirtualKeyboard)
#   - Coin3D + pivy
#   - OpenCASCADE
#   - Python3-dev (ABI must match the host FreeCAD's Python)
#   - Boost (filesystem, program_options, regex)
#   - Xerces-C
#   - Eigen3, yaml-cpp, fmt
#
# Easiest path: bootstrap the sysroot with debootstrap or use Buildroot.
# See scripts/setup-sysroot.sh (TODO, Faz 5).

set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(MILCAM_AARCH64_SYSROOT "/opt/sysroot/vec-ve" CACHE PATH "Sysroot for VEC-VE target")

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_SYSROOT       ${MILCAM_AARCH64_SYSROOT})
set(CMAKE_FIND_ROOT_PATH ${MILCAM_AARCH64_SYSROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Footprint discipline — VEC-VE panel has limited disk.
add_compile_options(-Os -ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections)

# Python ABI alignment for CodesysBridge.so. Override if your sysroot has
# a different Python version.
set(Python3_EXECUTABLE   "${MILCAM_AARCH64_SYSROOT}/usr/bin/python3"   CACHE FILEPATH "")
set(Python3_INCLUDE_DIR  "${MILCAM_AARCH64_SYSROOT}/usr/include/python3.11" CACHE PATH "")
set(Python3_LIBRARY      "${MILCAM_AARCH64_SYSROOT}/usr/lib/aarch64-linux-gnu/libpython3.11.so" CACHE FILEPATH "")
