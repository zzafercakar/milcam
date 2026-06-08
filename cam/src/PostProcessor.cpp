/**
 * @file PostProcessor.cpp
 * @brief Machine-specific G-code post-processor for LinuxCNC, Grbl, and Fanuc flavors.
 *
 * Translates abstract toolpath segments into concrete G-code blocks tailored
 * to a specific CNC controller dialect. Handles preamble (modal setup codes),
 * per-segment motion commands (G0/G1/G2/G3), coolant control (M7/M8/M9),
 * tool length compensation (G43/G49), and postamble (program end).
 */

#include "PostProcessor.h"

namespace MilCAD {

namespace {

GCodeBlock makeModalBlock(int lineNumber, int gCode)
{
    GCodeBlock block;
    block.n = lineNumber;
    block.g = gCode;
    return block;
}

GCodeBlock makeMiscBlock(int lineNumber, int mCode)
{
    GCodeBlock block;
    block.n = lineNumber;
    block.m = mCode;
    return block;
}

} // namespace

// ─── PostProcessor base class default implementations ────────────────────────

/**
 * @brief Generate coolant M-code block.
 * @param mode Coolant mode to activate.
 * @param lineNumber N-word line number.
 * @return GCodeBlock with M7 (mist), M8 (flood), or M9 (off).
 *
 * Through-spindle coolant uses M8 (flood) as the standard G-code;
 * actual spindle-through config is machine-specific.
 */
GCodeBlock PostProcessor::generateCoolant(CoolantMode mode, int lineNumber) const
{
    GCodeBlock b;
    b.n = lineNumber;
    switch (mode) {
    case CoolantMode::Mist:
        b.m = 7;  // Mist coolant ON
        break;
    case CoolantMode::Flood:
    case CoolantMode::Through:
        b.m = 8;  // Flood coolant ON (through uses same M-code)
        break;
    case CoolantMode::Off:
    default:
        b.m = 9;  // Coolant OFF
        break;
    }
    return b;
}

/**
 * @brief Generate tool length compensation activation (G43 H{tool}).
 * @param toolNumber Tool offset register number (maps to H-word).
 * @param lineNumber N-word line number.
 * @return GCodeBlock with G43 and the tool's offset register.
 */
GCodeBlock PostProcessor::generateToolLengthComp(int toolNumber, int lineNumber) const
{
    GCodeBlock b;
    b.n = lineNumber;
    b.g = 43;           // G43: tool length compensation positive
    b.t = toolNumber;   // H-word stored in T for simplicity (maps to offset register)
    return b;
}

/**
 * @brief Generate tool length compensation cancel (G49).
 * @param lineNumber N-word line number.
 * @return GCodeBlock with G49.
 */
GCodeBlock PostProcessor::generateToolLengthCompCancel(int lineNumber) const
{
    GCodeBlock b;
    b.n = lineNumber;
    b.g = 49;  // G49: cancel tool length compensation
    return b;
}

// ─── GenericPostProcessor ────────────────────────────────────────────────────

/**
 * @brief Generate the G-code preamble blocks for the selected controller flavor.
 * @return Vector of GCodeBlock containing machine initialization commands.
 *
 * Typical preamble includes: G90 (absolute mode), G21 (metric/mm), G17 (XY plane
 * selection), and optionally G94 (feed-per-minute) depending on the flavor.
 */
std::vector<GCodeBlock> GenericPostProcessor::generatePreamble() const
{
    switch (flavor_) {
    case GenericPostFlavor::LinuxCNC:
        return {
            makeModalBlock(10, 90), // absolute
            makeModalBlock(20, 21), // mm
            makeModalBlock(30, 17), // XY plane
            makeModalBlock(40, 94)  // feed/min
        };
    case GenericPostFlavor::Grbl:
        return {
            makeModalBlock(10, 90), // absolute
            makeModalBlock(20, 21)  // mm
        };
    case GenericPostFlavor::Fanuc:
    default:
        return {
            makeModalBlock(10, 90), // absolute
            makeModalBlock(20, 21), // mm
            makeModalBlock(30, 17)  // XY plane
        };
    }
}

/**
 * @brief Generate the G-code postamble (program end) blocks.
 * @return Vector of GCodeBlock containing the program termination command.
 *
 * Grbl uses M2 (program end); Fanuc and LinuxCNC use M30 (program end and rewind).
 */
std::vector<GCodeBlock> GenericPostProcessor::generatePostamble() const
{
    if (flavor_ == GenericPostFlavor::Grbl) {
        return {makeMiscBlock(9990, 2)};
    }

    return {makeMiscBlock(9990, 30)};
}

/**
 * @brief Convert a single toolpath segment into a G-code motion block.
 * @param segment  The toolpath segment to encode.
 * @param lineNumber  The N-word line number to assign to this block.
 * @return GCodeBlock with the appropriate G-code (G0/G1/G2/G3), coordinates, and feed rate.
 *
 * Rapid moves become G0, linear cuts and plunges become G1, CW arcs become G2,
 * and CCW arcs become G3. Arc center offsets (I, J) are computed as incremental
 * distances from the segment start point to the arc center.
 */
GCodeBlock GenericPostProcessor::generateMotion(const ToolpathSegment &segment, int lineNumber) const
{
    GCodeBlock b;
    b.n = lineNumber;

    switch (segment.type) {
    case ToolpathSegmentType::Rapid:
        b.g = 0;
        break;
    case ToolpathSegmentType::Linear:
    case ToolpathSegmentType::Plunge:
        b.g = 1;
        b.f = segment.feedRate;
        break;
    case ToolpathSegmentType::CWArc:
        b.g = 2;
        b.f = segment.feedRate;
        b.i = segment.center.X() - segment.start.X();
        b.j = segment.center.Y() - segment.start.Y();
        break;
    case ToolpathSegmentType::CCWArc:
        b.g = 3;
        b.f = segment.feedRate;
        b.i = segment.center.X() - segment.start.X();
        b.j = segment.center.Y() - segment.start.Y();
        break;
    }

    b.x = segment.end.X();
    b.y = segment.end.Y();
    b.z = segment.end.Z();
    return b;
}

} // namespace MilCAD
