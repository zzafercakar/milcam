# SPDX-License-Identifier: LGPL-2.1-or-later
# Part — TopoShape, Geometry, Part features, Boolean ops, Import/Export

add_library(Part SHARED)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(Part PRIVATE Part_EXPORTS QT_NO_KEYWORDS)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(Part
    PUBLIC
        "${FREECAD_SRC_DIR}"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(Part SYSTEM PRIVATE
    ${OpenCASCADE_INCLUDE_DIR}
)

# HAVE_FREETYPE requires both FreeType AND harfbuzz
find_package(harfbuzz QUIET)
if((Freetype_FOUND OR FREETYPE_FOUND) AND harfbuzz_FOUND)
    target_include_directories(Part SYSTEM PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_compile_definitions(Part PRIVATE HAVE_FREETYPE)
endif()

# ---------------------------------------------------------------------------
# Collect source files — all .cpp except generated *Py.cpp
# (those are #include'd from *PyImp.cpp, not compiled separately)
# ---------------------------------------------------------------------------
file(GLOB Part_MAIN_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB Part_BREP_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/BRepFeat/*.cpp")
file(GLOB Part_CHFI_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ChFi2d/*.cpp")
file(GLOB Part_GEOM2D_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Geom2d/*.cpp")
file(GLOB Part_GEOMPLATE_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/GeomPlate/*.cpp")
file(GLOB Part_HLR_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/HLRBRep/*.cpp")
file(GLOB Part_IGES_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/IGES/*.cpp")
file(GLOB Part_OCAF_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/OCAF/*.cpp")
file(GLOB Part_SHAPEFIX_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ShapeFix/*.cpp")
file(GLOB Part_SHAPEUP_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ShapeUpgrade/*.cpp")
file(GLOB Part_STEP_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/STEP/*.cpp")

# Remove generated *Py.cpp files — they are #include'd from *PyImp.cpp
set(Part_ALL_SRCS
    ${Part_MAIN_SRCS} ${Part_BREP_SRCS} ${Part_CHFI_SRCS}
    ${Part_GEOM2D_SRCS} ${Part_GEOMPLATE_SRCS} ${Part_HLR_SRCS}
    ${Part_IGES_SRCS} ${Part_OCAF_SRCS} ${Part_SHAPEFIX_SRCS}
    ${Part_SHAPEUP_SRCS} ${Part_STEP_SRCS}
)

# Build exclusion list from .pyi files — only GENERATED *Py.cpp should be excluded
# (they are #include'd from *PyImp.cpp, not compiled as separate TUs)
set(_generated_py_cpps "")
file(GLOB _all_pyi "${CMAKE_CURRENT_SOURCE_DIR}/*.pyi")
foreach(_pyi ${_all_pyi})
    get_filename_component(_name ${_pyi} NAME_WE)
    list(APPEND _generated_py_cpps "${CMAKE_CURRENT_SOURCE_DIR}/${_name}Py.cpp")
endforeach()
foreach(_subdir BRepFeat ChFi2d Geom2d GeomPlate HLRBRep ShapeFix ShapeUpgrade)
    file(GLOB _sub_pyi "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/*.pyi")
    foreach(_pyi ${_sub_pyi})
        get_filename_component(_name ${_pyi} NAME_WE)
        list(APPEND _generated_py_cpps "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/${_name}Py.cpp")
    endforeach()
endforeach()
list(REMOVE_ITEM Part_ALL_SRCS ${_generated_py_cpps})

# Make generated .h files discoverable in binary dir (for subdirs too)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/BRepFeat")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ChFi2d")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Geom2d")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/GeomPlate")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/HLRBRep")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ShapeFix")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ShapeUpgrade")

# ---------------------------------------------------------------------------
# Pre-generated Python bindings — copy .h and .cpp to binary dir
# (using generate_from_py for main dir, manual copy for subdirs)
# ---------------------------------------------------------------------------
set(GENERATED_PY_SOURCES "")

# Main dir .pyi files
file(GLOB _main_pyi CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.pyi")
foreach(_pyi ${_main_pyi})
    get_filename_component(_name ${_pyi} NAME_WE)
    generate_from_py(${_name})
endforeach()

# Subdirectory .pyi files
foreach(_subdir BRepFeat ChFi2d Geom2d GeomPlate HLRBRep ShapeFix ShapeUpgrade)
    file(GLOB _sub_pyi CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/*.pyi")
    foreach(_pyi ${_sub_pyi})
        get_filename_component(_name ${_pyi} NAME_WE)
        # Copy pre-generated files to binary subdir
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/${_name}Py.cpp" AND
           EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/${_name}Py.h")
            configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/${_name}Py.cpp"
                "${CMAKE_CURRENT_BINARY_DIR}/${_subdir}/${_name}Py.cpp" COPYONLY)
            configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}/${_name}Py.h"
                "${CMAKE_CURRENT_BINARY_DIR}/${_subdir}/${_name}Py.h" COPYONLY)
        endif()
    endforeach()
endforeach()

target_sources(Part PRIVATE
    ${Part_ALL_SRCS}
    ${GENERATED_PY_SOURCES}
)

# ---------------------------------------------------------------------------
# OCCT link libraries (listed individually per CADNC critical rule)
# ---------------------------------------------------------------------------
set(Part_OCC_LIBS
    TKernel TKMath TKBRep TKGeomBase TKGeomAlgo
    TKG2d TKG3d TKTopAlgo TKShHealing TKMesh
    TKBO TKPrim TKBool TKOffset TKFeat TKFillet
    TKHLR TKService TKV3d TKIGES TKSTEPBase
    TKSTEP TKSTEP209 TKSTEPAttr TKXCAF TKXDESTEP
    TKLCAF TKVCAF TKXSBase TKRWMesh TKSTL TKVRML
    TKXmlXCAF TKBinXCAF
)

target_link_libraries(Part PUBLIC
    FreeCADApp
    Materials
    ${Part_OCC_LIBS}
    fmt::fmt
)

if((Freetype_FOUND OR FREETYPE_FOUND) AND harfbuzz_FOUND)
    target_link_libraries(Part PRIVATE ${FREETYPE_LIBRARIES} harfbuzz::harfbuzz)
endif()

# ---------------------------------------------------------------------------
# Compiler settings
# ---------------------------------------------------------------------------
target_compile_features(Part PUBLIC cxx_std_20)
set_target_properties(Part PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Part PRIVATE
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-deprecated-declarations
        -Wno-sign-compare
        -Wno-write-strings
        -Wno-overloaded-virtual
    )
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS Part
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
