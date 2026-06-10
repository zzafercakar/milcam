# milcam/core/CMakeLists.txt
add_library(milcam_core
    post/codesys/CodesysPost.cpp
)
target_include_directories(milcam_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(milcam_core PUBLIC cxx_std_17)
