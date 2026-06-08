# MilCAM tests — initial placeholder.
# The CADNC parity suite is intentionally NOT migrated; MilCAM has its own
# narrow scope (import + CAM + post). Add tests here as features land.

# Smoke test for CodesysBridge (filesystem-only, no PLC dep).
add_executable(test_codesys_bridge test_codesys_bridge.cpp)
target_link_libraries(test_codesys_bridge PRIVATE milcam_adapter Qt6::Core)
target_include_directories(test_codesys_bridge PRIVATE ${CMAKE_SOURCE_DIR}/adapter/inc)
set_target_properties(test_codesys_bridge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME codesys_bridge COMMAND $<TARGET_FILE:test_codesys_bridge>)
