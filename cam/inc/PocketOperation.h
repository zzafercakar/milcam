/**
 * @file PocketOperation.h
 * @brief Rectangular pocket milling operation with professional parameters.
 *
 * Generates a zigzag raster toolpath to clear a rectangular pocket area,
 * with configurable step-down, step-over, ramp/helix entry strategy,
 * stock-to-leave allowance, and separate cutting/plunge feed rates.
 */

#ifndef MILCAD_POCKET_OPERATION_H
#define MILCAD_POCKET_OPERATION_H

#include "Operation.h"

namespace MilCAD {

/**
 * @enum PocketEntryType
 * @brief Strategy for tool entry into the pocket (Z-axis engagement).
 *
 * Plunge is simplest but creates tool wear; ramp and helix distribute
 * the entry load over a longer path, extending tool life.
 */
enum class PocketEntryType {
    Plunge,  ///< Direct vertical plunge (standard, highest tool load)
    Ramp,    ///< Linear ramp entry at a shallow angle (SolidWorks/NX style)
    Helix    ///< Helical entry with configurable radius (Fusion 360 style)
};

/**
 * @class PocketOperation
 * @brief CAM operation that clears a rectangular pocket using zigzag raster passes.
 *
 * Professional features:
 * - Ramp or helical entry to reduce tool wear during Z engagement
 * - Stock-to-leave allowance for roughing/finishing strategy
 * - Multi-pass depth stepping with alternating row directions
 */
class PocketOperation : public Operation
{
public:
    std::string typeName() const override { return "Pocket"; }

    // --- Core parameters ---

    /// @brief Set the rectangular pocket bounds in the XY plane.
    void setBounds(double minX, double minY, double maxX, double maxY)
    {
        minX_ = minX; minY_ = minY; maxX_ = maxX; maxY_ = maxY;
    }

    /// @brief Set the final pocket depth Z coordinate.
    void setTargetZ(double v) { targetZ_ = v; }

    /// @brief Set the Z-axis depth of cut per pass in mm.
    void setStepDown(double v) { stepDown_ = v; }

    /// @brief Set the Y-axis lateral step-over between rows in mm.
    void setStepOver(double v) { stepOver_ = v; }

    /// @brief Set the safe Z height for rapid moves above the workpiece.
    void setSafeZ(double v) { safeZ_ = v; }

    /// @brief Set the cutting feed rate in mm/min.
    void setFeedCut(double v) { feedCut_ = v; }

    /// @brief Set the plunge feed rate in mm/min.
    void setFeedPlunge(double v) { feedPlunge_ = v; }

    // --- Entry strategy (ramp / helix) ---

    /// @brief Set the pocket entry type (Plunge, Ramp, or Helix).
    void setEntryType(PocketEntryType t) { entryType_ = t; }

    /// @brief Set the ramp angle in degrees (for Ramp entry). Typical: 2-5 degrees.
    void setRampAngle(double deg) { rampAngle_ = deg; }

    /// @brief Set the helix radius in mm (for Helix entry). Must be < pocket width/2.
    void setHelixRadius(double r) { helixRadius_ = r; }

    // --- Stock-to-Leave (finishing allowance) ---

    /// @brief Set the radial (wall) stock-to-leave allowance in mm.
    void setStockToLeaveRadial(double v) { stockToLeaveRadial_ = v; }

    /// @brief Set the axial (floor) stock-to-leave allowance in mm.
    void setStockToLeaveAxial(double v) { stockToLeaveAxial_ = v; }

    /**
     * @brief Generate the pocket clearing toolpath with professional features.
     * @return A Toolpath with ramp/helix entry and zigzag raster segments.
     *         Returns empty if bounds or parameters are invalid.
     */
    Toolpath generateToolpath() const override;

private:
    // --- Core parameters ---
    double minX_ = 0.0;        ///< Left boundary X coordinate
    double minY_ = 0.0;        ///< Bottom boundary Y coordinate
    double maxX_ = 0.0;        ///< Right boundary X coordinate
    double maxY_ = 0.0;        ///< Top boundary Y coordinate
    double targetZ_ = -2.0;    ///< Final pocket depth Z in mm
    double stepDown_ = 1.0;    ///< Depth per pass in mm
    double stepOver_ = 2.0;    ///< Lateral step between rows in mm
    double safeZ_ = 5.0;       ///< Safe retract height in mm
    double feedCut_ = 900.0;   ///< Cutting feed rate in mm/min
    double feedPlunge_ = 220.0;///< Plunge feed rate in mm/min

    // --- Entry strategy ---
    PocketEntryType entryType_ = PocketEntryType::Plunge; ///< Entry method
    double rampAngle_ = 3.0;   ///< Ramp entry angle in degrees (shallow)
    double helixRadius_ = 2.0; ///< Helix entry radius in mm

    // --- Stock-to-Leave ---
    double stockToLeaveRadial_ = 0.0;  ///< Wall finishing allowance (mm)
    double stockToLeaveAxial_ = 0.0;   ///< Floor finishing allowance (mm)
};

} // namespace MilCAD

#endif // MILCAD_POCKET_OPERATION_H
