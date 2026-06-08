/**
 * @file CodesysStExporters.cpp
 * @brief Default Structured Text exporters for CODESYS integration.
 */

#include "CodesysStExporters.h"

#include <iomanip>
#include <sstream>

namespace MilCAD {

std::string SmcCncRefExporter::exportArray(const std::vector<GCodeBlock> &blocks,
                                           const std::string &arrayName) const
{
    auto escapeStString = [](const std::string &s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            out.push_back(c);
            if (c == '\'')
                out.push_back('\'');
        }
        return out;
    };

    std::ostringstream ss;
    ss << "VAR_GLOBAL\n";
    if (blocks.empty()) {
        ss << "    " << arrayName << " : ARRAY[0..0] OF STRING(1) := [''];\n";
    } else {
        ss << "    " << arrayName << " : ARRAY[0.." << (blocks.size() - 1)
           << "] OF STRING(120) := [\n";
        for (size_t i = 0; i < blocks.size(); ++i) {
            ss << "        '" << escapeStString(blocks[i].toString()) << "'";
            if (i + 1 < blocks.size())
                ss << ",";
            ss << "\n";
        }
        ss << "    ];\n";
    }
    ss << "END_VAR";
    return ss.str();
}

std::string SmcOutqueueExporter::exportQueue(const Toolpath &path,
                                             const std::string &arrayName) const
{
    auto moveType = [](ToolpathSegmentType t) -> int {
        switch (t) {
        case ToolpathSegmentType::Rapid: return 0;
        case ToolpathSegmentType::Linear: return 1;
        case ToolpathSegmentType::CWArc: return 2;
        case ToolpathSegmentType::CCWArc: return 3;
        case ToolpathSegmentType::Plunge: return 4;
        }
        return 1;
    };

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);

    ss << "TYPE SMC_GEOINFO :\n";
    ss << "STRUCT\n";
    ss << "    MoveType : INT;\n";
    ss << "    X : LREAL;\n";
    ss << "    Y : LREAL;\n";
    ss << "    Z : LREAL;\n";
    ss << "    I : LREAL;\n";
    ss << "    J : LREAL;\n";
    ss << "    K : LREAL;\n";
    ss << "    F : LREAL;\n";
    ss << "END_STRUCT\n";
    ss << "END_TYPE\n\n";

    ss << "VAR_GLOBAL\n";
    if (path.empty()) {
        ss << "    " << arrayName << " : ARRAY[0..0] OF SMC_GEOINFO := "
           << "[(MoveType:=0, X:=0.0, Y:=0.0, Z:=0.0, I:=0.0, J:=0.0, K:=0.0, F:=0.0)];\n";
    } else {
        ss << "    " << arrayName << " : ARRAY[0.." << (path.size() - 1)
           << "] OF SMC_GEOINFO := [\n";
        for (size_t i = 0; i < path.size(); ++i) {
            const auto &seg = path.segments()[i];
            const double iOff = seg.center.X() - seg.start.X();
            const double jOff = seg.center.Y() - seg.start.Y();
            const double kOff = seg.center.Z() - seg.start.Z();
            ss << "        (MoveType:=" << moveType(seg.type)
               << ", X:=" << seg.end.X()
               << ", Y:=" << seg.end.Y()
               << ", Z:=" << seg.end.Z()
               << ", I:=" << (seg.isArc() ? iOff : 0.0)
               << ", J:=" << (seg.isArc() ? jOff : 0.0)
               << ", K:=" << (seg.isArc() ? kOff : 0.0)
               << ", F:=" << seg.feedRate
               << ")";
            if (i + 1 < path.size())
                ss << ",";
            ss << "\n";
        }
        ss << "    ];\n";
    }
    ss << "END_VAR";
    return ss.str();
}

} // namespace MilCAD
