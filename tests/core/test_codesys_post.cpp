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
