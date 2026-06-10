# CMakeLists.txt  (top-level)
cmake_minimum_required(VERSION 3.16)
project(MilCAM VERSION 0.3.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()
add_subdirectory(milcam/core)
add_subdirectory(tests)

# The Qt Widgets GUI is opt-in (needs Qt5). Host core-tests build without it;
# the device/app build configures with -DMILCAM_BUILD_APP=ON.
option(MILCAM_BUILD_APP "Build the MilCAM Qt Widgets GUI app" OFF)
if(MILCAM_BUILD_APP)
    add_subdirectory(milcam/app)
endif()
