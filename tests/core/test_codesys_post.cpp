// tests/core/test_codesys_post.cpp
#include "check.h"
#include "post/codesys/CodesysPost.h"
#include <fstream>
#include <sstream>
using namespace milcam;

TEST(post_empty_program_has_header_and_end) {
    Toolpath tp;                       // no moves
    PostConfig cfg; cfg.programName = "JOB1";
    CHECK_EQ(postCodesys(tp, cfg),
        "% JOB1\n"
        "N10 G90\n"
        "N20 M30\n");
}

TEST(post_rapid_and_feed_with_units_per_second) {
    Toolpath tp;
    tp.moves.push_back({MoveKind::Rapid, 0, 0, 10, 0, 0, 0});       // G0
    tp.moves.push_back({MoveKind::Feed, 20, 0, 0, 0, 0, 600});      // F=600mm/min -> 10 mm/s
    PostConfig cfg; cfg.programName = "JOB1";
    cfg.accelMmS2 = 500; cfg.decelMmS2 = 500;
    CHECK_EQ(postCodesys(tp, cfg),
        "% JOB1\n"
        "N10 G90\n"
        "N20 G0 X0 Y0 Z10\n"
        "N30 G1 X20 Y0 Z0 F10 E500 E-500\n"
        "N40 M30\n");
}

TEST(post_arc_emits_relative_ij) {
    Toolpath tp;
    tp.moves.push_back({MoveKind::Rapid, 0, 0, 0, 0, 0, 0});
    // CCW quarter arc to (0,20), center offset relative to start = (-10? example)
    tp.moves.push_back({MoveKind::ArcCCW, 0, 20, 0, -10, 0, 4800}); // 80 mm/s? 4800/60=80
    PostConfig cfg; cfg.programName = "ARC";
    cfg.accelMmS2 = 500; cfg.decelMmS2 = 500;
    CHECK_EQ(postCodesys(tp, cfg),
        "% ARC\n"
        "N10 G90\n"
        "N20 G0 X0 Y0 Z0\n"
        "N30 G3 X0 Y20 Z0 I-10 J0 F80 E500 E-500\n"
        "N40 M30\n");
}

TEST(post_suppresses_repeated_modal_gcode) {
    Toolpath tp;
    tp.moves.push_back({MoveKind::Feed, 10, 0, 0, 0, 0, 600});
    tp.moves.push_back({MoveKind::Feed, 10, 10, 0, 0, 0, 600});
    PostConfig cfg; cfg.programName = "MODAL";
    CHECK_EQ(postCodesys(tp, cfg),
        "% MODAL\n"
        "N10 G90\n"
        "N20 G1 X10 Y0 Z0 F10 E500 E-500\n"
        "N30 X10 Y10 Z0 F10 E500 E-500\n"
        "N40 M30\n");
}

// Golden-file regression helper: reads a file from the golden directory
// (path resolved at compile time via MILCAM_GOLDEN_DIR definition).
static std::string readGolden(const std::string& name) {
    std::ifstream f(std::string(MILCAM_GOLDEN_DIR) + "/" + name, std::ios::binary);
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

TEST(post_golden_square_profile) {
    Toolpath tp;
    tp.moves.push_back({MoveKind::Rapid,  0,  0,  0, 0, 0,   0});  // rapid in
    tp.moves.push_back({MoveKind::Feed,  20,  0,  0, 0, 0, 600});  // edge 1 +X
    tp.moves.push_back({MoveKind::Feed,  20, 20,  0, 0, 0, 600});  // edge 2 +Y
    tp.moves.push_back({MoveKind::Feed,   0, 20,  0, 0, 0, 600});  // edge 3 -X
    tp.moves.push_back({MoveKind::Feed,   0,  0,  0, 0, 0, 600});  // edge 4 -Y back to start
    PostConfig cfg; cfg.programName = "SQUARE";  // accel/decel default 500/500
    CHECK_EQ(postCodesys(tp, cfg), readGolden("profile_square.cnc"));
}
