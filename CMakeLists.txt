# SPDX-License-Identifier: Proprietary
# MilCAM — slim CAM distribution of FreeCAD for CodeSys CNC panel PCs.
#
# Architecture:
#   - FreeCAD source is built in-tree via add_subdirectory(${FREECAD_SOURCE_DIR})
#     with non-CAM modules disabled via BUILD_<X>=OFF.
#   - milcam/CodesysBridge/ is a Python C extension installed alongside the
#     MilCAM workbench (Mod/MilCAM/).
#   - milcam/Mod/MilCAM/ is a Python workbench overlay that hides non-CAM
#     workbenches and adds the "Send to CodeSys" and "Switch to HMI" commands.
#
# Result: when the user runs the resulting `FreeCAD` binary, they see a
# touch-friendly window where the only workbench they can pick is MilCAM
# (built on top of FreeCAD's CAM workbench).

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# FreeCAD source location
# ---------------------------------------------------------------------------
# We do NOT vendor FreeCAD source — it's 3+ GB. Instead, the user points
# us at a local clone via FREECAD_SOURCE_DIR. Defaults to /home/embed/Downloads/
# FreeCAD-main-1-1-git (the maintainer's local clone). On a CI / fresh setup,
# the user can override:
#   cmake -DFREECAD_SOURCE_DIR=/path/to/FreeCAD -B build
set(FREECAD_SOURCE_DIR "/home/embed/Downloads/FreeCAD-main-1-1-git"
    CACHE PATH "Path to a FreeCAD source checkout")

if(NOT EXISTS "${FREECAD_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "MilCAM: FREECAD_SOURCE_DIR='${FREECAD_SOURCE_DIR}' does not look like a "
        "FreeCAD source tree (missing CMakeLists.txt). "
        "Clone FreeCAD and re-run with -DFREECAD_SOURCE_DIR=<path>.")
endif()

# ---------------------------------------------------------------------------
# Disable non-CAM FreeCAD modules BEFORE add_subdirectory().
# FreeCAD reads these as cache variables in its
# cMake/FreeCAD_Helpers/InitializeFreeCADBuildOptions.cmake.
# ---------------------------------------------------------------------------
# Keep ON (CAM-essential):
#   BUILD_GUI         — the FreeCAD application window
#   BUILD_PART        — geometry import (STEP/IGES/BREP)
#   BUILD_PART_DESIGN — bodies (CAM operates on these)
#   BUILD_SKETCHER    — sketches (some CAM ops use them)
#   BUILD_CAM         — THE workbench (FreeCAD 1.0+; renamed from Path)
#   BUILD_DRAFT       — DXF import
#   BUILD_MESH        — STL import
#   BUILD_MESH_PART   — Mesh ↔ Part interop
#   BUILD_MATERIAL    — Part dependency
#   BUILD_SPREADSHEET — CAM tool tables / job parameters
#   BUILD_MEASURE     — measure widget
#   BUILD_IMPORT      — STEP/IGES/BREP importer
#   BUILD_HELP        — help system (small; nice to have)
#   BUILD_START       — start page (small; can be turned off later)
#   BUILD_TUX         — Tux palette (small)
#   BUILD_ADDONMGR    — let user install extra postprocessors at runtime

# Disable (CAD-only / heavy / unneeded):
set(BUILD_FEM             OFF CACHE BOOL "" FORCE)
set(BUILD_BIM             OFF CACHE BOOL "" FORCE)
set(BUILD_ASSEMBLY        OFF CACHE BOOL "" FORCE)
set(BUILD_DRAWING         OFF CACHE BOOL "" FORCE)
set(BUILD_INSPECTION      OFF CACHE BOOL "" FORCE)
set(BUILD_JTREADER        OFF CACHE BOOL "" FORCE)
set(BUILD_MATERIAL_EXTERNAL OFF CACHE BOOL "" FORCE)
set(BUILD_OPENSCAD        OFF CACHE BOOL "" FORCE)
set(BUILD_PLOT            OFF CACHE BOOL "" FORCE)
set(BUILD_POINTS          OFF CACHE BOOL "" FORCE)
set(BUILD_REVERSEENGINEERING OFF CACHE BOOL "" FORCE)
set(BUILD_ROBOT           OFF CACHE BOOL "" FORCE)
set(BUILD_SHOW            OFF CACHE BOOL "" FORCE)
set(BUILD_SURFACE         OFF CACHE BOOL "" FORCE)
set(BUILD_TECHDRAW        OFF CACHE BOOL "" FORCE)
set(BUILD_TEST            OFF CACHE BOOL "" FORCE)
set(BUILD_WEB             OFF CACHE BOOL "" FORCE)
set(BUILD_CLOUD           OFF CACHE BOOL "" FORCE)
set(BUILD_VR              OFF CACHE BOOL "" FORCE)
set(BUILD_FLAT_MESH       OFF CACHE BOOL "" FORCE)
set(BUILD_DESIGNER_PLUGIN OFF CACHE BOOL "" FORCE)
set(BUILD_SANDBOX         OFF CACHE BOOL "" FORCE)
set(BUILD_TEMPLATE        OFF CACHE BOOL "" FORCE)
set(BUILD_TRACY_FRAME_PROFILER OFF CACHE BOOL "" FORCE)

# Keep ON (defaults are already ON for these but we record intent):
set(BUILD_GUI         ON CACHE BOOL "" FORCE)
set(BUILD_PART        ON CACHE BOOL "" FORCE)
set(BUILD_PART_DESIGN ON CACHE BOOL "" FORCE)
set(BUILD_SKETCHER    ON CACHE BOOL "" FORCE)
set(BUILD_CAM         ON CACHE BOOL "" FORCE)
set(BUILD_DRAFT       ON CACHE BOOL "" FORCE)
set(BUILD_MESH        ON CACHE BOOL "" FORCE)
set(BUILD_MESH_PART   ON CACHE BOOL "" FORCE)
set(BUILD_MATERIAL    ON CACHE BOOL "" FORCE)
set(BUILD_SPREADSHEET ON CACHE BOOL "" FORCE)
set(BUILD_MEASURE     ON CACHE BOOL "" FORCE)
set(BUILD_IMPORT      ON CACHE BOOL "" FORCE)
set(BUILD_HELP        OFF CACHE BOOL "" FORCE)     # heavy QtWebEngine dep — drop on panel
set(BUILD_START       OFF CACHE BOOL "" FORCE)     # start page not relevant for kiosk use
set(BUILD_TUX         OFF CACHE BOOL "" FORCE)
set(BUILD_ADDONMGR    OFF CACHE BOOL "" FORCE)     # field machines do not install addons

# Optional disables that often surface in FreeCAD's CMake outputs:
set(FREECAD_USE_PCH OFF CACHE BOOL "" FORCE)

# ---------------------------------------------------------------------------
# Project — MilCAM owns this CMake; FreeCAD is a subdirectory we configure.
# ---------------------------------------------------------------------------
project(MilCAM VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Where the MilCAM workbench + CodesysBridge module install to inside FreeCAD's
# data directory. FreeCAD's CMake sets CMAKE_INSTALL_DATADIR via GNUInstallDirs;
# Mod is conventionally at $datadir/freecad/Mod.
include(GNUInstallDirs)
set(MILCAM_MOD_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/freecad/Mod"
    CACHE PATH "Where to install MilCAM workbench + CodesysBridge module")

option(MILCAM_BUILD_FREECAD "Build the bundled FreeCAD source tree" ON)
option(MILCAM_BRAND_AS_MILCAM "Patch resulting binary to advertise as 'MilCAM' instead of 'FreeCAD'" OFF)

# ---------------------------------------------------------------------------
# FreeCAD build
# ---------------------------------------------------------------------------
if(MILCAM_BUILD_FREECAD)
    message(STATUS "MilCAM: building FreeCAD from ${FREECAD_SOURCE_DIR}")
    message(STATUS "        — CAM stack ON  : Gui, Part, PartDesign, Sketcher, CAM, Draft, Mesh, Material, Spreadsheet, Measure, Import")
    message(STATUS "        — Heavy modules OFF: FEM, BIM, Assembly, TechDraw, Surface, OpenSCAD, Plot, Robot, Web, Help, Inspection, Points, RE, Tux, AddonMgr, Cloud, Drawing, Show")
    add_subdirectory(${FREECAD_SOURCE_DIR} "${CMAKE_BINARY_DIR}/freecad-build")
endif()

# ---------------------------------------------------------------------------
# MilCAM's CodesysBridge Python extension
# ---------------------------------------------------------------------------
add_subdirectory(milcam/CodesysBridge)

# ---------------------------------------------------------------------------
# Install the Python workbench overlay (Mod/MilCAM/) — InitGui.py, commands,
# icons, postprocessor.
# ---------------------------------------------------------------------------
install(DIRECTORY milcam/Mod/MilCAM/
    DESTINATION "${MILCAM_MOD_INSTALL_DIR}/MilCAM"
    FILES_MATCHING
        PATTERN "*.py"
        PATTERN "*.svg"
        PATTERN "*.png"
        PATTERN "*.qm"
        PATTERN "__pycache__" EXCLUDE
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
