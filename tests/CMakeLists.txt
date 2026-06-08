# MilCAM tests.
# Python-driven: ctest invokes pytest on tests/python/.

find_program(PYTEST_EXECUTABLE NAMES pytest pytest-3)

if(PYTEST_EXECUTABLE)
    add_test(NAME python_unit
        COMMAND ${PYTEST_EXECUTABLE} -v "${CMAKE_SOURCE_DIR}/tests/python"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    set_tests_properties(python_unit PROPERTIES
        ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}/milcam/Mod/MilCAM:${CMAKE_BINARY_DIR}/milcam/CodesysBridge:$ENV{PYTHONPATH}"
    )
else()
    message(STATUS "MilCAM tests: pytest not found, skipping python_unit test target.")
endif()
