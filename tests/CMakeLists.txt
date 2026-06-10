# tests/CMakeLists.txt
add_executable(core_tests
    core/test_main.cpp
    core/test_codesys_post.cpp
)
target_link_libraries(core_tests PRIVATE milcam_core)
target_include_directories(core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)
add_test(NAME core_tests COMMAND core_tests)
target_compile_definitions(core_tests PRIVATE
    MILCAM_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/core/golden")
