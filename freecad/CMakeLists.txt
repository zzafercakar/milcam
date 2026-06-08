# SPDX-License-Identifier: LGPL-2.1-or-later
# MilCAM — FreeCAD backend modules build configuration (slim subset)
#
# Builds: FreeCADBase, FreeCADApp, Materials, Part
# Sketcher and PartDesign are intentionally OMITTED — MilCAM is CAM-only and
# never authors parametric geometry. See .ai/FREECAD_SUBSET.md for rationale.

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# Macros
# ---------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FreeCadMacros.cmake)

# ---------------------------------------------------------------------------
# Dependencies (found by parent CMakeLists.txt when CADNC_ENABLE_FREECAD_BACKEND=ON)
# ---------------------------------------------------------------------------

# Python3, Boost, XercesC, ZLIB, ICU, Eigen3 are found by the root CMakeLists.txt.
# OpenCASCADE is also found there.

# ---------------------------------------------------------------------------
# fmt — modern C++ formatting (required by FreeCAD Console/Quantity/etc.)
# ---------------------------------------------------------------------------
find_package(fmt QUIET)
if(NOT fmt_FOUND)
    message(STATUS "fmt not found via find_package — fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        10.2.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(fmt)
endif()

# ---------------------------------------------------------------------------
# PyCXX — Python C++ bindings (bundled in 3rdParty)
# ---------------------------------------------------------------------------
set(PYCXX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/PyCXX")
set(PYCXX_INCLUDE_DIRS "${PYCXX_DIR}")
set(PYCXX_SOURCES
    "${PYCXX_DIR}/CXX/cxxextensions.c"
    "${PYCXX_DIR}/CXX/cxx_extensions.cxx"
    "${PYCXX_DIR}/CXX/cxxsupport.cxx"
    "${PYCXX_DIR}/CXX/IndirectPythonInterface.cxx"
    "${PYCXX_DIR}/CXX/cxx_exceptions.cxx"
)

# ---------------------------------------------------------------------------
# zipios++ — ZIP I/O (bundled in 3rdParty, used by FreeCADBase Reader/Writer)
# ---------------------------------------------------------------------------
# zipios++ headers are included as <zipios++/foo.h>, so the parent dir is the include path
set(ZIPIOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/zipios++")
set(ZIPIOS_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty")
file(GLOB zipios_SRCS "${ZIPIOS_DIR}/*.cpp")

# ---------------------------------------------------------------------------
# FastSignals — signal/slot library (bundled in 3rdParty)
# ---------------------------------------------------------------------------
set(FASTSIGNALS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/FastSignals/libfastsignals")
add_library(libfastsignals STATIC
    "${FASTSIGNALS_DIR}/src/connection.cpp"
    "${FASTSIGNALS_DIR}/src/function_detail.cpp"
    "${FASTSIGNALS_DIR}/src/signal_impl.cpp"
)
target_include_directories(libfastsignals PUBLIC
    "${FASTSIGNALS_DIR}/include"
)
target_compile_features(libfastsignals PUBLIC cxx_std_17)
set_target_properties(libfastsignals PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# FreeCAD include paths — replicate src/ layout so #include <Base/Foo.h> works
# ---------------------------------------------------------------------------
# FreeCAD headers use includes like:
#   #include <Base/Vector3D.h>
#   #include <App/Document.h>
#   #include <Mod/Part/App/Geometry.h>
# The parent directory of Base/, App/, Mod/ is freecad/ itself.
set(FREECAD_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# Subdirectories (order matters — dependency chain)
# ---------------------------------------------------------------------------
add_subdirectory(Base)
add_subdirectory(App)
add_subdirectory(Mod/Material/App)
add_subdirectory(Mod/Part/App)
# Sketcher + PartDesign deliberately not included — MilCAM is CAM-only.
