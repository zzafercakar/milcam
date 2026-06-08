# SPDX-License-Identifier: Proprietary
# MilCAM — Touch-first CAM tool for CodeSys CNC panel PCs (VEC-VE class).
#
# Derived from CADNC's build system but slimmed:
#   - No Sketcher / PartDesign (CAM-only — no parametric authoring).
#   - No Nesting module (out of scope for CAM-only product).
#   - Qt VirtualKeyboard added for on-screen text input.
#   - Adapter glob excludes Sketch* and Nest* sources.

cmake_minimum_required(VERSION 3.22)
project(MilCAM VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MILCAM_ENABLE_ASAN     "Enable AddressSanitizer" OFF)
option(MILCAM_BUILD_TESTS     "Build unit tests" ON)
option(MILCAM_ENABLE_FREECAD  "Build with FreeCAD Part backend (STEP/IGES import)" ON)
option(MILCAM_ENABLE_OPCUA    "Build with OPC UA client (open62541)" OFF)

if(MILCAM_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# ---------------------------------------------------------------------------
# Qt6 — Quick, QuickControls2, VirtualKeyboard for touch input
# ---------------------------------------------------------------------------
find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core Gui Quick Qml QuickControls2 Widgets
    OpenGL OpenGLWidgets Svg Xml VirtualKeyboard
)
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# OpenCASCADE — geometry kernel
# ---------------------------------------------------------------------------
if(UNIX AND NOT APPLE AND NOT OpenCASCADE_DIR
   AND EXISTS "/usr/lib/x86_64-linux-gnu/cmake/opencascade")
    set(OpenCASCADE_DIR "/usr/lib/x86_64-linux-gnu/cmake/opencascade"
        CACHE PATH "OpenCASCADE CMake config path" FORCE)
endif()
find_package(OpenCASCADE REQUIRED COMPONENTS
    FoundationClasses ModelingData ModelingAlgorithms
    Visualization ApplicationFramework DataExchange
)

# ---------------------------------------------------------------------------
# FreeCAD slim subset (Base, App, Materials, Part)
# ---------------------------------------------------------------------------
if(MILCAM_ENABLE_FREECAD)
    include(FetchContent)
    find_package(Python3 REQUIRED COMPONENTS Development Interpreter)
    find_package(ZLIB REQUIRED)
    find_package(Boost REQUIRED COMPONENTS filesystem program_options regex)

    find_package(XercesC QUIET)
    if(NOT XercesC_FOUND)
        FetchContent_Declare(XercesC
            GIT_REPOSITORY https://github.com/apache/xerces-c.git
            GIT_TAG v3.2.5 GIT_SHALLOW TRUE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(XercesC)
        set(XercesC_INCLUDE_DIRS "${xercesc_SOURCE_DIR}/src" "${xercesc_BINARY_DIR}/src")
        set(XercesC_LIBRARIES xerces-c)
    endif()

    find_package(Eigen3 QUIET)
    if(NOT Eigen3_FOUND)
        FetchContent_Declare(Eigen3
            GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
            GIT_TAG 3.4.0 GIT_SHALLOW TRUE)
        set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(Eigen3)
    endif()

    find_package(ICU COMPONENTS uc data)

    find_package(yaml-cpp QUIET)
    if(NOT yaml-cpp_FOUND)
        FetchContent_Declare(yaml-cpp
            GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
            GIT_TAG 0.8.0 GIT_SHALLOW TRUE)
        set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(yaml-cpp)
    endif()

    find_package(Freetype QUIET)
    add_subdirectory(freecad)
endif()

# ---------------------------------------------------------------------------
# OPC UA client (optional — Faz 3)
# ---------------------------------------------------------------------------
if(MILCAM_ENABLE_OPCUA)
    find_package(open62541 QUIET)
    if(NOT open62541_FOUND)
        message(STATUS "open62541 not found — fetching")
        include(FetchContent)
        FetchContent_Declare(open62541
            GIT_REPOSITORY https://github.com/open62541/open62541.git
            GIT_TAG v1.3.10 GIT_SHALLOW TRUE)
        set(UA_ENABLE_AMALGAMATION OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(open62541)
    endif()
endif()

# ---------------------------------------------------------------------------
# AppVersion.h — configure-time generated
# ---------------------------------------------------------------------------
configure_file(
    "${CMAKE_SOURCE_DIR}/app/AppVersion.h.in"
    "${CMAKE_BINARY_DIR}/generated/AppVersion.h"
    @ONLY
)

# ---------------------------------------------------------------------------
# Source globs — adapter EXCLUDES Sketch* and Nest* (carried over from CADNC
# but unused in CAM-only MilCAM). They will be deleted after the migration
# stabilises; until then we just skip them at build time.
# ---------------------------------------------------------------------------
file(GLOB _ALL_ADAPTER_HEADERS CONFIGURE_DEPENDS adapter/inc/*.h)
file(GLOB _ALL_ADAPTER_SOURCES CONFIGURE_DEPENDS adapter/src/*.cpp)
set(ADAPTER_HEADERS "")
foreach(f ${_ALL_ADAPTER_HEADERS})
    if(NOT f MATCHES "Sketch|Nest|SceneManager")
        list(APPEND ADAPTER_HEADERS ${f})
    endif()
endforeach()
set(ADAPTER_SOURCES "")
foreach(f ${_ALL_ADAPTER_SOURCES})
    if(NOT f MATCHES "Sketch|Nest")
        list(APPEND ADAPTER_SOURCES ${f})
    endif()
endforeach()

file(GLOB CAM_HEADERS      CONFIGURE_DEPENDS cam/inc/*.h)
file(GLOB CAM_SOURCES      CONFIGURE_DEPENDS cam/src/*.cpp)
file(GLOB UTIL_HEADERS     CONFIGURE_DEPENDS util/inc/*.h)
file(GLOB UTIL_SOURCES     CONFIGURE_DEPENDS util/src/*.cpp)
file(GLOB VIEWPORT_HEADERS CONFIGURE_DEPENDS viewport/inc/*.h)
file(GLOB VIEWPORT_SOURCES CONFIGURE_DEPENDS viewport/src/*.cpp)
file(GLOB UI_SOURCES       CONFIGURE_DEPENDS ui/src/*.cpp ui/src/*.h)
file(GLOB QML_FILES        CONFIGURE_DEPENDS
    ui/qml/*.qml
    ui/qml/qmldir
    ui/qml/components/*.qml
    ui/qml/panels/*.qml
    ui/qml/toolbars/*.qml
    ui/qml/popups/*.qml
    ui/qml/*.js
)
file(GLOB ICON_FILES CONFIGURE_DEPENDS
    resources/icons/*.svg
    resources/icons/cam/*.svg
    resources/icons/part/*.svg
    resources/icons/quickaccess/*.svg
    resources/logos/*.svg
    resources/logos/*.png
)

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(milcam
    app/main.cpp
    ${CAM_HEADERS} ${CAM_SOURCES}
    ${UTIL_HEADERS} ${UTIL_SOURCES}
    ${VIEWPORT_HEADERS} ${VIEWPORT_SOURCES}
    ${UI_SOURCES}
)
set_target_properties(milcam PROPERTIES OUTPUT_NAME milcam)

target_include_directories(milcam PRIVATE
    "${CMAKE_BINARY_DIR}/generated"
    adapter/inc cam/inc util/inc viewport/inc ui/src
)
target_include_directories(milcam SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})

target_link_libraries(milcam PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml
    Qt6::QuickControls2 Qt6::Widgets
    Qt6::OpenGL Qt6::OpenGLWidgets
    Qt6::Svg Qt6::Xml
    Qt6::VirtualKeyboard
    OpenGL::GL
    ${OpenCASCADE_FoundationClasses_LIBRARIES}
    ${OpenCASCADE_ModelingData_LIBRARIES}
    ${OpenCASCADE_ModelingAlgorithms_LIBRARIES}
    ${OpenCASCADE_Visualization_LIBRARIES}
    ${OpenCASCADE_ApplicationFramework_LIBRARIES}
    ${OpenCASCADE_DataExchange_LIBRARIES}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(milcam PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_resources(milcam "qml_resources"
    PREFIX "/"
    BASE "${CMAKE_SOURCE_DIR}/ui"
    FILES ${QML_FILES}
)
qt_add_resources(milcam "icon_resources"
    PREFIX "/"
    BASE "${CMAKE_SOURCE_DIR}"
    FILES ${ICON_FILES}
)

# ---------------------------------------------------------------------------
# Adapter library — links the CAM-friendly subset of FreeCAD
# ---------------------------------------------------------------------------
if(MILCAM_ENABLE_FREECAD)
    add_library(milcam_adapter STATIC
        ${ADAPTER_HEADERS} ${ADAPTER_SOURCES}
        ${CAM_HEADERS}     ${CAM_SOURCES}
    )
    target_include_directories(milcam_adapter PUBLIC adapter/inc)
    target_include_directories(milcam_adapter PRIVATE
        "${CMAKE_SOURCE_DIR}/freecad"
        "${CMAKE_SOURCE_DIR}/viewport/inc"
        "${CMAKE_SOURCE_DIR}/cam/inc"
    )
    target_include_directories(milcam_adapter SYSTEM PRIVATE
        ${OpenCASCADE_INCLUDE_DIR} ${EIGEN3_INCLUDE_DIR}
    )
    target_link_libraries(milcam_adapter PUBLIC
        # NB: Sketcher / PartDesign deliberately NOT linked.
        Part Materials FreeCADApp FreeCADBase
        Qt6::Core Qt6::Quick
    )
    target_compile_definitions(milcam_adapter PRIVATE QT_NO_KEYWORDS)
    target_compile_features(milcam_adapter PUBLIC cxx_std_20)

    target_link_libraries(milcam PRIVATE milcam_adapter)
endif()

if(MILCAM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS milcam DESTINATION bin)
