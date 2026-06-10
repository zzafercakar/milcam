# milcam/app/CMakeLists.txt — MilCAM Qt Widgets GUI (linuxfb, no GL)
# Links Qt5::Widgets and the milcam_core static lib.
# AUTOMOC is required for Q_OBJECT classes (MainWindow).

set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)

add_executable(milcam
    src/main.cpp
    src/gui/MainWindow.cpp
)

target_link_libraries(milcam PRIVATE Qt5::Widgets milcam_core)

# Expose src/ so #include "gui/MainWindow.h" resolves from any translation unit
# in this target without needing relative path gymnastics.
target_include_directories(milcam PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
