/**
 * @file GCodeGenerator.cpp
 * @brief Converts a Toolpath into a sequence of G-code blocks using a PostProcessor.
 *
 * Acts as the bridge between the CAM toolpath representation and the textual
 * G-code output. The generator delegates preamble/postamble and per-segment
 * motion encoding to a PostProcessor, then assembles the final block list with
 * sequential line numbering. Supports coolant control and tool compensation.
 */

#include "GCodeGenerator.h"

#include <sstream>

namespace MilCAD {

/**
 * @brief Generate a complete G-code program from a toolpath and post-processor.
 * @param path  The toolpath to convert into G-code blocks.
 * @param postProcessor  Machine-specific post-processor for preamble, motion, and postamble.
 * @return Ordered vector of GCodeBlock representing the full CNC program.
 *
 * The output consists of: preamble blocks, one motion block per toolpath segment
 * (with auto-incremented line numbers), and postamble blocks.
 */
std::vector<GCodeBlock> GCodeGenerator::generate(const Toolpath &path,
                                                 const PostProcessor &postProcessor)
{
    std::vector<GCodeBlock> out = postProcessor.generatePreamble();

    int line = out.empty() ? 100 : ((*out.back().n) + 10);
    for (const auto &s : path.segments()) {
        out.push_back(postProcessor.generateMotion(s, line));
        line += 10;
    }

    const auto trailer = postProcessor.generatePostamble();
    out.insert(out.end(), trailer.begin(), trailer.end());
    return out;
}

/**
 * @brief Generate a G-code program with coolant activation and tool compensation.
 *
 * Output sequence:
 * 1. Preamble (G90, G21, etc.)
 * 2. Tool length compensation (G43 H{tool}) — if toolNumber > 0
 * 3. Coolant activation (M7/M8) — if coolant != Off
 * 4. Motion blocks for all toolpath segments
 * 5. Coolant off (M9) — if coolant was activated
 * 6. Tool compensation cancel (G49) — if toolNumber > 0
 * 7. Postamble (M30)
 */
std::vector<GCodeBlock> GCodeGenerator::generateWithOptions(const Toolpath &path,
                                                             const PostProcessor &postProcessor,
                                                             CoolantMode coolant,
                                                             int toolNumber)
{
    std::vector<GCodeBlock> out = postProcessor.generatePreamble();

    int line = out.empty() ? 100 : ((*out.back().n) + 10);

    // Insert tool length compensation after preamble
    if (toolNumber > 0) {
        out.push_back(postProcessor.generateToolLengthComp(toolNumber, line));
        line += 10;
    }

    // Insert coolant activation after tool comp
    if (coolant != CoolantMode::Off) {
        out.push_back(postProcessor.generateCoolant(coolant, line));
        line += 10;
    }

    // Motion blocks
    for (const auto &s : path.segments()) {
        out.push_back(postProcessor.generateMotion(s, line));
        line += 10;
    }

    // Coolant off before postamble
    if (coolant != CoolantMode::Off) {
        out.push_back(postProcessor.generateCoolant(CoolantMode::Off, line));
        line += 10;
    }

    // Tool compensation cancel before postamble
    if (toolNumber > 0) {
        out.push_back(postProcessor.generateToolLengthCompCancel(line));
        line += 10;
    }

    const auto trailer = postProcessor.generatePostamble();
    out.insert(out.end(), trailer.begin(), trailer.end());
    return out;
}

/**
 * @brief Convert a vector of G-code blocks into a newline-separated text string.
 * @param blocks  The G-code blocks to serialize.
 * @return Complete G-code program as a single string, one block per line.
 */
std::string GCodeGenerator::toText(const std::vector<GCodeBlock> &blocks)
{
    std::ostringstream ss;
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        ss << blocks[i].toString();
        if (i + 1 < blocks.size())
            ss << '\n';
    }
    return ss.str();
}

} // namespace MilCAD
