// tests/core/test_codesys_post.cpp
#include "check.h"
#include "post/codesys/CodesysPost.h"
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
