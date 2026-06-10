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

    for (const auto& m : tp.moves) {
        std::ostringstream b;
        switch (m.kind) {
            case MoveKind::Rapid: b << "G0"; break;
            case MoveKind::Feed:  b << "G1"; break;
            case MoveKind::ArcCW: b << "G2"; break;
            case MoveKind::ArcCCW:b << "G3"; break;
        }
        b << " X" << num(m.x) << " Y" << num(m.y) << " Z" << num(m.z);
        if (m.kind == MoveKind::Feed || m.kind == MoveKind::ArcCW ||
            m.kind == MoveKind::ArcCCW) {
            // DIN 66025 F is path-units/SECOND, not mm/min.
            b << " F" << num(m.feedMmMin / 60.0);
            b << " E"  << num(cfg.accelMmS2);    // acceleration word
            b << " E-" << num(cfg.decelMmS2);    // deceleration word
        }
        line(b.str());
    }

    line("M30");                              // program end
    return out.str();
}

}  // namespace milcam
