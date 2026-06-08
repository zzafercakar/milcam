# SPDX-License-Identifier: LGPL-2.1-or-later
# Materials — material management for FreeCAD features

add_library(Materials SHARED)

target_compile_definitions(Materials PRIVATE Materials_EXPORTS QT_NO_KEYWORDS)

target_include_directories(Materials
    PUBLIC "${FREECAD_SRC_DIR}"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Materials SYSTEM PRIVATE
    "${FREECAD_SRC_DIR}/3rdParty"
)

# Pre-generated Python bindings
set(GENERATED_PY_SOURCES "")
file(GLOB _mat_pyi CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.pyi")
foreach(_pyi ${_mat_pyi})
    get_filename_component(_name ${_pyi} NAME_WE)
    generate_from_py(${_name})
endforeach()

# Source files — exclude only generated *Py.cpp (included via *PyImp.cpp)
file(GLOB Materials_ALL_SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
set(_gen_py_cpps "")
foreach(_pyi ${_mat_pyi})
    get_filename_component(_name ${_pyi} NAME_WE)
    list(APPEND _gen_py_cpps "${CMAKE_CURRENT_SOURCE_DIR}/${_name}Py.cpp")
endforeach()
list(REMOVE_ITEM Materials_ALL_SRCS ${_gen_py_cpps})

target_sources(Materials PRIVATE ${Materials_ALL_SRCS} ${GENERATED_PY_SOURCES})

target_link_libraries(Materials PUBLIC
    FreeCADApp
    Qt6::Core
    yaml-cpp::yaml-cpp
    fmt::fmt
)

target_compile_features(Materials PUBLIC cxx_std_20)
set_target_properties(Materials PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Materials PRIVATE
        -Wno-unused-parameter -Wno-missing-field-initializers
        -Wno-deprecated-declarations -Wno-sign-compare
    )
endif()

install(TARGETS Materials LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
