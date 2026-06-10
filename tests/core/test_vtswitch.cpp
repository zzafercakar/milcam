#include "check.h"
#include "hmi/VtSwitch.h"
using namespace milcam;

TEST(vtswitch_program_is_chvt) {
    CHECK_EQ(vtSwitchProgram(), std::string("/usr/bin/chvt"));
}
TEST(vtswitch_arg_is_vt_number) {
    CHECK_EQ(vtSwitchArg(1), std::string("1"));
    CHECK_EQ(vtSwitchArg(2), std::string("2"));
}
