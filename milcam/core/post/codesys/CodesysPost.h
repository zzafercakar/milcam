// milcam/core/post/codesys/CodesysPost.h
#pragma once
#include <string>
#include "geometry/Toolpath.h"

namespace milcam {

struct PostConfig {
    std::string programName = "MILCAM";
    double accelMmS2 = 500.0;   // E  word  (path accel)
    double decelMmS2 = 500.0;   // E- word  (path decel)
    int lineStart = 10;
    int lineStep = 10;
};

// Convert a Toolpath to CODESYS DIN 66025 .cnc text.
std::string postCodesys(const Toolpath& tp, const PostConfig& cfg);

}  // namespace milcam
