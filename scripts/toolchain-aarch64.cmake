# MilCAM cross-compile toolchain for VEC-VE panel PC (aarch64).
#
# USAGE:
#   1. Build an aarch64 sysroot containing Qt6, OCCT, boost, xerces-c, etc.
#   2. Set MILCAM_AARCH64_SYSROOT below to your sysroot path.
#   3. cmake --preset panel-pc
#
# The default values assume Debian-style cross-toolchain installed via:
#   sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
# and a sysroot mirrored to /opt/sysroot/vec-ve.

set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Adjust to match your environment.
set(MILCAM_AARCH64_SYSROOT "/opt/sysroot/vec-ve" CACHE PATH "Sysroot for VEC-VE target")

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_SYSROOT  ${MILCAM_AARCH64_SYSROOT})
set(CMAKE_FIND_ROOT_PATH ${MILCAM_AARCH64_SYSROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Size discipline — VEC-VE has limited disk.
add_compile_options(-Os -ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections -Wl,--strip-all)
