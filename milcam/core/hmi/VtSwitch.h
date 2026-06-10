// milcam/core/hmi/VtSwitch.h
#pragma once
#include <string>

namespace milcam {

// Linux virtual-terminal switch primitives. MilCAM and the CodeSYS HMI each own
// a VT on the single framebuffer; activating a VT shows that app (no WM needed).
inline std::string vtSwitchProgram() { return "/usr/bin/chvt"; }
inline std::string vtSwitchArg(int vt) { return std::to_string(vt); }

// VT assignments on the VEC-VE panel.
constexpr int kCodesysVt = 1;   // CodeSYS runtime HMI (env.sh tty=/dev/tty1)
constexpr int kMilcamVt  = 2;   // MilCAM runs here (launch with tty=/dev/tty2)

}  // namespace milcam
