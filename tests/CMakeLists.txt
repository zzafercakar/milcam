# tests/CMakeLists.txt
add_executable(core_tests
    core/test_main.cpp
    core/test_codesys_post.cpp
)
target_link_libraries(core_tests PRIVATE milcam_core)
target_include_directories(core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)
add_test(NAME core_tests COMMAND core_tests)

# Where core_tests looks for golden .cnc files AT RUNTIME. Defaults to the
# in-tree source path (host runs). Override at configure time for relocated
# runs — e.g. cross-build for the device: -DMILCAM_GOLDEN_DIR=/root/milcam-test/golden
set(MILCAM_GOLDEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core/golden"
    CACHE STRING "Path to golden .cnc files at test runtime")
target_compile_definitions(core_tests PRIVATE
    MILCAM_GOLDEN_DIR="${MILCAM_GOLDEN_DIR}")
