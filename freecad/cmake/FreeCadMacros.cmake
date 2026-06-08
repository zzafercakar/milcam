# SPDX-License-Identifier: LGPL-2.1-or-later
# CADNC — CMake macros for FreeCAD module integration
#
# Provides generate_from_py() and generate_from_any() that replicate
# the FreeCAD build system's Python binding generation.

# Directory containing the generate.py tool and templates
set(FREECAD_TOOLS_DIR "${CMAKE_CURRENT_LIST_DIR}/../Tools/bindings"
    CACHE PATH "Path to FreeCAD binding generation tools")

# generate_from_py(BASE_NAME)
#   Generates ${BASE_NAME}Py.cpp and ${BASE_NAME}Py.h from ${BASE_NAME}.pyi
#   using the FreeCAD generate.py tool.
#
#   The generated files are placed in ${CMAKE_CURRENT_BINARY_DIR} and
#   automatically added to the parent target's sources.
function(generate_from_py BASE_NAME)
    set(_pyi_file "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.pyi")
    set(_out_cpp  "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}Py.cpp")
    set(_out_h    "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}Py.h")

    # Check if pre-generated files exist in source dir (preferred — avoids
    # needing Python3 + generate.py at build time)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}Py.cpp" AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}Py.h")
        # Use pre-generated files — copy to binary dir so include paths work
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}Py.cpp"
            "${_out_cpp}" COPYONLY)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}Py.h"
            "${_out_h}" COPYONLY)
    elseif(EXISTS "${_pyi_file}" AND Python3_EXECUTABLE)
        # Generate at configure time
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${FREECAD_TOOLS_DIR}/generate.py" "${_pyi_file}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE _gen_result
            ERROR_VARIABLE _gen_error
        )
        if(NOT _gen_result EQUAL 0)
            message(WARNING "generate_from_py(${BASE_NAME}) failed: ${_gen_error}")
        endif()
        # The tool writes output next to the .pyi file; move to binary dir
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}Py.cpp")
            file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}Py.cpp" "${_out_cpp}")
            file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}Py.h" "${_out_h}")
        endif()
    else()
        message(FATAL_ERROR
            "generate_from_py(${BASE_NAME}): no pre-generated files and no .pyi + Python3 available")
    endif()

    # NOTE: *Py.cpp is NOT added as a separate translation unit.
    # It is #include'd from *PyImp.cpp (FreeCAD convention).
    # We only add *Py.h to the source list so CMake tracks it as a dependency.
    # The *Py.cpp must be in the binary dir so #include "TypePy.cpp" resolves.
    set(GENERATED_PY_SOURCES ${GENERATED_PY_SOURCES} "${_out_h}" PARENT_SCOPE)
endfunction()

# generate_from_any(INPUT_FILE OUTPUT_FILE VARIABLE)
#   Converts a text file into a C++ const char[] definition.
#   Used for embedding XSD schemas etc.
function(generate_from_any INPUT_FILE OUTPUT_FILE VARIABLE)
    set(_input  "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}")
    set(_output "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}")

    # Check for pre-generated file in source dir
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OUTPUT_FILE}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${OUTPUT_FILE}"
            "${_output}" COPYONLY)
    elseif(EXISTS "${_input}" AND Python3_EXECUTABLE)
        set(_tool "${CMAKE_CURRENT_LIST_DIR}/../Tools/PythonToCPP.py")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${_tool}"
                    "${_input}" "${_output}" "${VARIABLE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE _gen_result
        )
        if(NOT _gen_result EQUAL 0)
            message(WARNING "generate_from_any(${INPUT_FILE}) failed")
        endif()
    else()
        message(FATAL_ERROR
            "generate_from_any(${INPUT_FILE}): no pre-generated file and no Python3 available")
    endif()
endfunction()
