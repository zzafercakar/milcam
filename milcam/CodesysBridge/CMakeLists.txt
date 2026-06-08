# MilCAM CodesysBridge — Python C extension module.
#
# Imported by FreeCAD's MilCAM workbench (Mod/MilCAM/InitGui.py):
#     import CodesysBridge
#
# Built against the same Python ABI as the host FreeCAD install.

find_package(Python3 REQUIRED COMPONENTS Development.Module)

add_library(CodesysBridge MODULE
    src/CodesysBridge.cpp
    src/CodesysBridgePy.cpp
)

target_include_directories(CodesysBridge PRIVATE inc)
target_link_libraries(CodesysBridge PRIVATE Python3::Module)
target_compile_features(CodesysBridge PRIVATE cxx_std_17)

set_target_properties(CodesysBridge PROPERTIES
    PREFIX ""                              # Python wants CodesysBridge.so, not libCodesysBridge.so
    OUTPUT_NAME "CodesysBridge"
)

# Install next to FreeCAD's Mod directory so MilCAM workbench can `import CodesysBridge`.
install(TARGETS CodesysBridge
    LIBRARY DESTINATION "${MILCAM_MOD_INSTALL_DIR}/MilCAM"
)
