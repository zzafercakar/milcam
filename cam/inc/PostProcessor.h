/**
 * @file PostProcessor.h
 * @brief Abstract post-processor interface and generic G-code post-processor.
 *
 * Defines the PostProcessor base class that translates toolpath segments into
 * G-code blocks, and the GenericPostProcessor that supports Fanuc, LinuxCNC,
 * and Grbl output flavors. Includes coolant control (M7/M8/M9) and tool
 * length compensation (G43/G49) support.
 */

#ifndef MILCAD_POST_PROCESSOR_H
#define MILCAD_POST_PROCESSOR_H

#include <string>
#include <vector>

#include "GCodeBlock.h"
#include "Toolpath.h"
#include "Operation.h"

namespace MilCAD {

/**
 * @class PostProcessor
 * @brief Abstract interface for converting toolpath data into machine-specific G-code.
 *
 * Concrete post-processors implement preamble (initialization), postamble
 * (shutdown), and per-segment motion generation according to the target
 * CNC controller's dialect. Now includes coolant and tool compensation support.
 */
class PostProcessor
{
public:
    virtual ~PostProcessor() = default;

    /**
     * @brief Generate the G-code initialization blocks (e.g. G90, G21).
     * @return Vector of GCodeBlock forming the program preamble.
     */
    virtual std::vector<GCodeBlock> generatePreamble() const = 0;

    /**
     * @brief Generate the G-code program-end blocks (e.g. M30).
     * @return Vector of GCodeBlock forming the program postamble.
     */
    virtual std::vector<GCodeBlock> generatePostamble() const = 0;

    /**
     * @brief Convert a single toolpath segment into a G-code motion block.
     * @param segment The toolpath segment to translate.
     * @param lineNumber The N-word line number to assign to the block.
     * @return A GCodeBlock representing the motion command.
     */
    virtual GCodeBlock generateMotion(const ToolpathSegment &segment, int lineNumber) const = 0;

    /**
     * @brief Generate coolant activation G-code block.
     * @param mode The coolant mode (Off, Flood, Mist, Through).
     * @param lineNumber The N-word line number.
     * @return GCodeBlock with the appropriate M-code (M7/M8/M9).
     */
    virtual GCodeBlock generateCoolant(CoolantMode mode, int lineNumber) const;

    /**
     * @brief Generate tool length compensation activation block.
     * @param toolNumber Tool number for H-word offset register.
     * @param lineNumber The N-word line number.
     * @return GCodeBlock with G43 and H-word.
     */
    virtual GCodeBlock generateToolLengthComp(int toolNumber, int lineNumber) const;

    /**
     * @brief Generate tool length compensation cancel block.
     * @param lineNumber The N-word line number.
     * @return GCodeBlock with G49.
     */
    virtual GCodeBlock generateToolLengthCompCancel(int lineNumber) const;
};

/**
 * @enum GenericPostFlavor
 * @brief Selects the CNC controller dialect for the generic post-processor.
 */
enum class GenericPostFlavor {
    Fanuc,    ///< Fanuc-compatible G-code (G90/G21/G17 preamble, M30 end)
    LinuxCNC, ///< LinuxCNC-compatible G-code (adds G94 feed-per-minute mode)
    Grbl      ///< Grbl-compatible G-code (minimal preamble, M2 end)
};

/**
 * @class GenericPostProcessor
 * @brief A general-purpose post-processor supporting multiple common CNC controller flavors.
 *
 * Generates standard RS-274-style G-code with flavor-specific preamble and
 * postamble differences. Arc moves use incremental I/J center offsets.
 */
class GenericPostProcessor : public PostProcessor
{
public:
    /**
     * @brief Construct a generic post-processor for the given flavor.
     * @param flavor The target CNC controller dialect (default: Fanuc).
     */
    explicit GenericPostProcessor(GenericPostFlavor flavor = GenericPostFlavor::Fanuc)
        : flavor_(flavor) {}

    /** @copydoc PostProcessor::generatePreamble */
    std::vector<GCodeBlock> generatePreamble() const override;

    /** @copydoc PostProcessor::generatePostamble */
    std::vector<GCodeBlock> generatePostamble() const override;

    /** @copydoc PostProcessor::generateMotion */
    GCodeBlock generateMotion(const ToolpathSegment &segment, int lineNumber) const override;

    /// @brief Get the current post-processor flavor.
    GenericPostFlavor flavor() const { return flavor_; }

private:
    GenericPostFlavor flavor_ = GenericPostFlavor::Fanuc; ///< Target CNC dialect
};

} // namespace MilCAD

#endif // MILCAD_POST_PROCESSOR_H
