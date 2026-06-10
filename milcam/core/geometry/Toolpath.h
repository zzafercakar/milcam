// milcam/core/geometry/Toolpath.h
#pragma once
#include <vector>

namespace milcam {

enum class MoveKind { Rapid, Feed, ArcCW, ArcCCW };

// One motion command. Coordinates are absolute target positions in mm.
// For arcs, (i,j) is the arc-center offset RELATIVE to the move's start
// point (DIN 66025 default; never emit G98/G99). feedMmMin is the
// commanded feed in mm/min (0 for rapids); the post converts to mm/s.
struct Move {
    MoveKind kind = MoveKind::Rapid;
    double x = 0.0, y = 0.0, z = 0.0;
    double i = 0.0, j = 0.0;      // arc center offset, relative to start
    double feedMmMin = 0.0;
};

struct Toolpath {
    std::vector<Move> moves;
};

}  // namespace milcam
