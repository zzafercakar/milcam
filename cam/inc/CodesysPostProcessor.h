/**
 * @file CodesysPostProcessor.h
 * @brief CODESYS-specific post-processor for SMC_CNC and SMC_Outqueue integration.
 *
 * Extends the PostProcessor interface to generate DIN 66025 style G-code
 * compatible with CODESYS CNC controllers. Additionally provides export
 * methods for IEC 61131-3 Structured Text arrays suitable for direct
 * inclusion in CODESYS PLC programs (SMC_CNC_REF and SMC_Outqueue formats).
 */

#ifndef MILCAD_CODESYS_POST_PROCESSOR_H
#define MILCAD_CODESYS_POST_PROCESSOR_H

#include <gp_Pnt.hxx>

#include "PostProcessor.h"
#include "CodesysStExporters.h"

namespace MilCAD {

/**
 * @class CodesysPostProcessor
 * @brief Post-processor targeting CODESYS CNC SoftMotion controllers.
 *
 * Generates G-code with CODESYS-specific preamble (G75 reset) and provides
 * two Structured Text export formats for embedding toolpaths directly into
 * PLC global variable declarations.
 */
class CodesysPostProcessor : public PostProcessor
{
public:
    enum class DrillCycleCode {
        G81 = 81,
        G82 = 82,
        G83 = 83,
        G73 = 73
    };

    struct DrillCycleParams {
        DrillCycleCode cycle = DrillCycleCode::G81;
        double retractZ = 5.0;
        double targetZ = -5.0;
        double feedRate = 200.0;
        double peckDepth = 2.0;
        double dwellSeconds = 0.0;
    };

    /**
     * @brief Generate CODESYS-style preamble: G75 (reset), G90 (absolute), G21 (metric).
     * @return Vector of preamble GCodeBlocks.
     */
    std::vector<GCodeBlock> generatePreamble() const override;

    /**
     * @brief Generate CODESYS-style postamble: M30 (program end).
     * @return Vector of postamble GCodeBlocks.
     */
    std::vector<GCodeBlock> generatePostamble() const override;

    /**
     * @brief Convert a toolpath segment into a CODESYS DIN 66025 motion block.
     * @param segment The toolpath segment to translate.
     * @param lineNumber The N-word line number to assign.
     * @return A GCodeBlock with arc offsets computed as incremental I/J from start.
     */
    GCodeBlock generateMotion(const ToolpathSegment &segment, int lineNumber) const override;

    /**
     * @brief Export G-code blocks as a CODESYS Structured Text STRING array.
     *
     * Produces a VAR_GLOBAL declaration containing an array of STRING(120)
     * values, each holding one serialized G-code line. Suitable for feeding
     * into SMC_CNC_REF via string-based G-code interpretation.
     *
     * @param blocks The G-code blocks to export.
     * @param arrayName The Structured Text variable name (default: "gCodeWords").
     * @return Complete VAR_GLOBAL..END_VAR text block.
     */
    std::string exportSmcCncRefArray(const std::vector<GCodeBlock> &blocks,
                                     const std::string &arrayName = "gCodeWords") const;
    std::string exportSmcCncRefArrayWith(const std::vector<GCodeBlock> &blocks,
                                         const ICodesysRefArrayExporter &exporter,
                                         const std::string &arrayName = "gCodeWords") const;

    /**
     * @brief Export a toolpath as a CODESYS SMC_GEOINFO struct array.
     *
     * Produces a TYPE definition for SMC_GEOINFO and a VAR_GLOBAL array of
     * struct literals, where each entry maps a segment to MoveType, X/Y/Z
     * coordinates, I/J/K arc offsets, and feed rate. Suitable for
     * SMC_Outqueue-based CNC path feeding.
     *
     * @param path The toolpath to export.
     * @param arrayName The Structured Text variable name (default: "geoQueue").
     * @return Complete TYPE + VAR_GLOBAL..END_VAR text block.
     */
    std::string exportSmcOutqueue(const Toolpath &path,
                                  const std::string &arrayName = "geoQueue") const;
    std::string exportSmcOutqueueWith(const Toolpath &path,
                                      const ICodesysOutqueueExporter &exporter,
                                      const std::string &arrayName = "geoQueue") const;

    std::vector<GCodeBlock> generateDrillCannedCycle(const std::vector<gp_Pnt> &points,
                                                     const DrillCycleParams &params,
                                                     int firstLineNumber = 100) const;
};

} // namespace MilCAD

#endif // MILCAD_CODESYS_POST_PROCESSOR_H
