# milcam/app/CMakeLists.txt — MilCAM Qt Widgets GUI (linuxfb, no GL)
# AUTOMOC for Q_OBJECT classes; AUTORCC compiles resources/milcam.qrc.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)

add_executable(milcam
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/TopBar.cpp
    src/gui/HmiSwitch.cpp
    src/gui/OperationToolBar.cpp
    src/gui/CanvasView.cpp
    src/gui/JobPanel.cpp
    src/gui/StatusBar.cpp
    resources/milcam.qrc
)
target_link_libraries(milcam PRIVATE Qt5::Widgets milcam_core)
# Expose src/ so #include "gui/X.h" resolves; milcam_core brings hmi/VtSwitch.h.
target_include_directories(milcam PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
