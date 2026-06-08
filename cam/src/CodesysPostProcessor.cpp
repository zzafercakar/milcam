/**
 * @file CodesysPostProcessor.cpp
 * @brief Implementation of the CODESYS-specific post-processor.
 *
 * Generates DIN 66025 style G-code and provides two Structured Text export
 * formats for CODESYS PLC integration: string-array (SMC_CNC_REF) and
 * struct-array (SMC_Outqueue).
 */

#include "CodesysPostProcessor.h"

#include <algorithm>

namespace MilCAD {

/**
 * @brief Generate CODESYS preamble: G75 (reset), G90 (absolute mode), G21 (metric).
 * @return Vector of three preamble blocks.
 */
std::vector<GCodeBlock> CodesysPostProcessor::generatePreamble() const
{
    // CODESYS DIN66025 style: G75 reset + absolute metric mode.
    GCodeBlock reset;
    reset.n = 10;
    reset.g = 75;

    GCodeBlock absolute;
    absolute.n = 20;
    absolute.g = 90;

    GCodeBlock metric;
    metric.n = 30;
    metric.g = 21;

    return {reset, absolute, metric};
}

/**
 * @brief Generate CODESYS postamble: M30 (program end).
 * @return Vector containing a single M30 block.
 */
std::vector<GCodeBlock> CodesysPostProcessor::generatePostamble() const
{
    GCodeBlock end;
    end.n = 9990;
    end.m = 30;
    return {end};
}

/**
 * @brief Convert a toolpath segment into a CODESYS motion G-code block.
 *
 * Maps segment types to G-codes: Rapid->G0, Linear/Plunge->G1, CWArc->G2,
 * CCWArc->G3. Arc center offsets are computed as incremental I/J relative
 * to the segment start point.
 *
 * @param segment The toolpath segment to convert.
 * @param lineNumber N-word line number for the output block.
 * @return The generated GCodeBlock.
 */
GCodeBlock CodesysPostProcessor::generateMotion(const ToolpathSegment &segment, int lineNumber) const
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
        // Incremental arc center offsets from start point
        b.i = segment.center.X() - segment.start.X();
        b.j = segment.center.Y() - segment.start.Y();
        break;
    case ToolpathSegmentType::CCWArc:
        b.g = 3;
        b.f = segment.feedRate;
        // Incremental arc center offsets from start point
        b.i = segment.center.X() - segment.start.X();
        b.j = segment.center.Y() - segment.start.Y();
        break;
    }

    b.x = segment.end.X();
    b.y = segment.end.Y();
    b.z = segment.end.Z();
    return b;
}

/**
 * @brief Export G-code blocks as a CODESYS Structured Text STRING array.
 *
 * Produces a VAR_GLOBAL declaration with an ARRAY OF STRING(120) containing
 * each G-code block serialized to text. Single quotes in G-code strings are
 * escaped by doubling them (ST string literal convention).
 *
 * @param blocks The G-code blocks to export.
 * @param arrayName The ST variable name for the array.
 * @return Complete VAR_GLOBAL..END_VAR declaration text.
 */
std::string CodesysPostProcessor::exportSmcCncRefArray(const std::vector<GCodeBlock> &blocks,
                                                       const std::string &arrayName) const
{
    SmcCncRefExporter exporter;
    return exportSmcCncRefArrayWith(blocks, exporter, arrayName);
}

std::string CodesysPostProcessor::exportSmcCncRefArrayWith(const std::vector<GCodeBlock> &blocks,
                                                           const ICodesysRefArrayExporter &exporter,
                                                           const std::string &arrayName) const
{
    return exporter.exportArray(blocks, arrayName);
}

/**
 * @brief Export a toolpath as a CODESYS SMC_GEOINFO struct array.
 *
 * Generates a TYPE definition for the SMC_GEOINFO struct and a VAR_GLOBAL
 * array of struct initializers. Each toolpath segment is mapped to:
 * - MoveType: 0=Rapid, 1=Linear, 2=CWArc, 3=CCWArc, 4=Plunge
 * - X/Y/Z: endpoint coordinates
 * - I/J/K: incremental arc center offsets (zero for non-arc moves)
 * - F: feed rate
 *
 * @param path The toolpath to export.
 * @param arrayName The ST variable name for the array.
 * @return Complete TYPE + VAR_GLOBAL..END_VAR declaration text.
 */
std::string CodesysPostProcessor::exportSmcOutqueue(const Toolpath &path,
                                                    const std::string &arrayName) const
{
    SmcOutqueueExporter exporter;
    return exportSmcOutqueueWith(path, exporter, arrayName);
}

std::string CodesysPostProcessor::exportSmcOutqueueWith(const Toolpath &path,
                                                        const ICodesysOutqueueExporter &exporter,
                                                        const std::string &arrayName) const
{
    return exporter.exportQueue(path, arrayName);
}

std::vector<GCodeBlock> CodesysPostProcessor::generateDrillCannedCycle(const std::vector<gp_Pnt> &points,
                                                                        const DrillCycleParams &params,
                                                                        int firstLineNumber) const
{
    std::vector<GCodeBlock> blocks;
    if (points.empty() || params.feedRate <= 1e-9 || params.retractZ <= params.targetZ)
        return blocks;

    int line = std::max(0, firstLineNumber);
    const int cycleCode = static_cast<int>(params.cycle);
    const bool usePeck = (params.cycle == DrillCycleCode::G83 || params.cycle == DrillCycleCode::G73)
                      && params.peckDepth > 1e-9;
    const bool useDwell = (params.cycle == DrillCycleCode::G82) && params.dwellSeconds > 1e-9;

    blocks.reserve(points.size() + 1);
    for (const auto &point : points) {
        GCodeBlock b;
        b.n = line;
        b.g = cycleCode;
        b.x = point.X();
        b.y = point.Y();
        b.z = params.targetZ;
        b.r = params.retractZ;
        b.f = params.feedRate;
        if (usePeck)
            b.q = params.peckDepth;
        if (useDwell)
            b.p = params.dwellSeconds;
        blocks.push_back(std::move(b));
        line += 10;
    }

    GCodeBlock cancelCycle;
    cancelCycle.n = line;
    cancelCycle.g = 80;
    blocks.push_back(std::move(cancelCycle));

    return blocks;
}

} // namespace MilCAD
