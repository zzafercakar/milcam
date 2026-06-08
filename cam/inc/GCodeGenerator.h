/**
 * @file GCodeGenerator.h
 * @brief High-level G-code generation from toolpaths using a post-processor.
 *
 * Orchestrates the conversion of a Toolpath into a complete G-code program
 * by delegating preamble, motion, and postamble generation to a PostProcessor
 * instance. Supports coolant control and tool length compensation insertion.
 */

#ifndef MILCAD_GCODE_GENERATOR_H
#define MILCAD_GCODE_GENERATOR_H

#include <string>
#include <vector>

#include "PostProcessor.h"
#include "Toolpath.h"

namespace MilCAD {

/**
 * @class GCodeGenerator
 * @brief Static utility class that converts Toolpath data into G-code via a PostProcessor.
 */
class GCodeGenerator
{
public:
    /**
     * @brief Generate a complete G-code program from a toolpath.
     *
     * Produces preamble blocks, then one motion block per toolpath segment
     * (with auto-incrementing N-word line numbers), then postamble blocks.
     *
     * @param path The toolpath to convert.
     * @param postProcessor The post-processor defining the output dialect.
     * @return Ordered vector of GCodeBlocks forming the complete program.
     */
    static std::vector<GCodeBlock> generate(const Toolpath &path, const PostProcessor &postProcessor);

    /**
     * @brief Generate a complete G-code program with coolant and tool compensation.
     *
     * Extended version that inserts coolant activation after preamble,
     * tool length compensation (G43 H{tool}) before motion, and
     * coolant off (M9) + G49 before postamble.
     *
     * @param path The toolpath to convert.
     * @param postProcessor The post-processor defining the output dialect.
     * @param coolant Coolant mode to activate.
     * @param toolNumber Tool number for length compensation (0 = skip).
     * @return Ordered vector of GCodeBlocks forming the complete program.
     */
    static std::vector<GCodeBlock> generateWithOptions(const Toolpath &path,
                                                        const PostProcessor &postProcessor,
                                                        CoolantMode coolant,
                                                        int toolNumber = 0);

    /**
     * @brief Serialize a vector of G-code blocks to a newline-separated text string.
     * @param blocks The G-code blocks to serialize.
     * @return Multi-line G-code text string.
     */
    static std::string toText(const std::vector<GCodeBlock> &blocks);
};

} // namespace MilCAD

#endif // MILCAD_GCODE_GENERATOR_H
