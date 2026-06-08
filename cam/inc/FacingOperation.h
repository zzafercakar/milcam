/**
 * @file FacingOperation.h
 * @brief Facing (surface leveling) machining operation.
 *
 * Generates a zigzag raster toolpath across the top of the stock to machine
 * it down to a target Z depth, using configurable step-over, pass depth,
 * and feed rates. The pattern alternates left-to-right and right-to-left
 * passes (boustrophedon) for efficient material removal.
 */

#ifndef MILCAD_FACING_OPERATION_H
#define MILCAD_FACING_OPERATION_H

#include "Operation.h"
#include "Tooling.h"

namespace MilCAD {

/**
 * @class FacingOperation
 * @brief CAM operation that faces (levels) the top surface of the stock.
 *
 * Produces a zigzag raster pattern over the stock's XY footprint,
 * with multiple depth passes if the total depth exceeds passDepth.
 */
class FacingOperation : public Operation
{
public:
    std::string typeName() const override { return "Facing"; }

    /// @brief Set the workpiece stock defining the XY area and top Z.
    void setStock(const Stock &stock) { stock_ = stock; }

    /// @brief Set the lateral step-over distance between passes in mm.
    void setStepOver(double v) { stepOver_ = v; }

    /// @brief Set the depth of cut per Z-level pass in mm.
    void setPassDepth(double v) { passDepth_ = v; }

    /// @brief Set the final target Z depth to face down to.
    void setTargetZ(double v) { targetZ_ = v; }

    /// @brief Set the safe Z height for rapid moves above the workpiece.
    void setSafeZ(double v) { safeZ_ = v; }

    /// @brief Set the cutting feed rate in mm/min.
    void setFeedCut(double v) { feedCut_ = v; }

    /// @brief Set the plunge feed rate in mm/min.
    void setFeedPlunge(double v) { feedPlunge_ = v; }

    /**
     * @brief Generate the facing toolpath as a zigzag raster pattern.
     * @return A Toolpath containing rapid, plunge, linear-cut, and retract segments.
     *         Returns an empty toolpath if stock dimensions or parameters are invalid.
     */
    Toolpath generateToolpath() const override;

private:
    Stock stock_;               ///< Workpiece stock defining area and height
    double stepOver_ = 5.0;    ///< Y-axis step between adjacent passes in mm
    double passDepth_ = 1.0;   ///< Z-axis depth of cut per level in mm
    double targetZ_ = 0.0;     ///< Final target Z depth in mm
    double safeZ_ = 5.0;       ///< Safe retract height in mm
    double feedCut_ = 1200.0;  ///< Cutting feed rate in mm/min
    double feedPlunge_ = 300.0;///< Plunge feed rate in mm/min
};

} // namespace MilCAD

#endif // MILCAD_FACING_OPERATION_H
