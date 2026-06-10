// milcam/core/post/codesys/CodesysPost.cpp
#include "post/codesys/CodesysPost.h"
#include <sstream>

namespace milcam {

namespace {
// Trim trailing zeros: 10.0 -> "10", 1.5 -> "1.5", -0.0 -> "0".
std::string num(double v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.4f", v);
    std::string s(buf);
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        auto last = s.find_last_not_of('0');
        if (last == dot) last--;            // drop dot too
        s.erase(last + 1);
    }
    if (s == "-0") s = "0";
    return s;
}
}  // namespace

std::string postCodesys(const Toolpath& tp, const PostConfig& cfg) {
    std::ostringstream out;
    int n = cfg.lineStart;
    auto line = [&](const std::string& body) {
        out << 'N' << n << ' ' << body << '\n';
        n += cfg.lineStep;
    };

    out << "% " << cfg.programName << '\n';   // DIN 66025 mandatory header
    line("G90");                              // absolute coordinates

    // (moves emitted in later tasks)
    (void)tp;

    line("M30");                              // program end
    return out.str();
}

}  // namespace milcam
