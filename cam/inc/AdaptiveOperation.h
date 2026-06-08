/**
 * @file AdaptiveOperation.h
 * @brief Adaptive clearing operation — constant tool engagement roughing.
 *
 * Ported from FreeCAD's CAM Adaptive operation concept (libarea Adaptive.hpp).
 * Generates toolpaths that maintain a constant tool engagement angle,
 * resulting in smoother cutting, reduced tool wear, and higher MRR.
 */

#ifndef MILCAD_ADAPTIVE_OPERATION_H
#define MILCAD_ADAPTIVE_OPERATION_H

#include "Operation.h"

#include <gp_Pnt.hxx>
#include <vector>

namespace MilCAD {

/**
 * @brief Operation type for adaptive clearing — inside or outside, clearing or profiling.
 */
enum class AdaptiveOpType {
    ClearingInside,    ///< Clear material inside a closed contour
    ClearingOutside,   ///< Clear material outside a contour (island avoidance)
    ProfilingInside,   ///< Profile the inside of a contour with constant engagement
    ProfilingOutside   ///< Profile the outside of a contour with constant engagement
};

/**
 * @class AdaptiveOperation
 * @brief Generates toolpaths with constant tool engagement for roughing.
 *
 * The adaptive algorithm spirals inward or outward, maintaining a target
 * engagement angle (typically 60-90 degrees). This avoids full-width cuts
 * that stress the tool and allows significantly higher feed rates and
 * step-downs compared to conventional pocket clearing.
 *
 * Key parameters:
 *   - stepOver:         lateral step as fraction of tool diameter (0.1 - 0.9)
 *   - stepDown:         axial depth per pass (mm)
 *   - helixAngle:       angle for helix ramp entry (degrees)
 *   - engagementAngle:  target tool engagement angle (degrees, typically 70)
 *   - liftDistance:      retract distance for linking moves (mm)
 */
class AdaptiveOperation : public Operation
{
public:
    AdaptiveOperation();

    std::string typeName() const override { return "Adaptive"; }
    Toolpath generateToolpath() const override;

    // Contour definition (boundary of the clearing region)
    void setContour(const std::vector<gp_Pnt>& contour) { contour_ = contour; }
    const std::vector<gp_Pnt>& contour() const { return contour_; }

    // Operation type (clearing/profiling, inside/outside)
    void setOpType(AdaptiveOpType type) { opType_ = type; }
    AdaptiveOpType opType() const { return opType_; }

    // Tool diameter (mm) — used for stepover computation
    void setToolDiameter(double d) { toolDiameter_ = d; }
    double toolDiameter() const { return toolDiameter_; }

    // Step-over as fraction of tool diameter (0.1 - 0.9)
    void setStepOver(double fraction) { stepOver_ = fraction; }
    double stepOver() const { return stepOver_; }

    // Step-down per depth pass (mm)
    void setStepDown(double d) { stepDown_ = d; }
    double stepDown() const { return stepDown_; }

    // Target Z depth
    void setTargetZ(double z) { targetZ_ = z; }
    double targetZ() const { return targetZ_; }

    // Safe Z height for rapids
    void setSafeZ(double z) { safeZ_ = z; }
    double safeZ() const { return safeZ_; }

    // Feed rates
    void setFeedCut(double f) { feedCut_ = f; }
    double feedCut() const { return feedCut_; }
    void setFeedPlunge(double f) { feedPlunge_ = f; }
    double feedPlunge() const { return feedPlunge_; }

    // Helix ramp angle for entry (degrees)
    void setHelixAngle(double deg) { helixAngle_ = deg; }
    double helixAngle() const { return helixAngle_; }

    // Lift distance for non-cutting link moves (mm)
    void setLiftDistance(double d) { liftDistance_ = d; }
    double liftDistance() const { return liftDistance_; }

private:
    std::vector<gp_Pnt> contour_;
    AdaptiveOpType opType_ = AdaptiveOpType::ClearingInside;
    double toolDiameter_ = 6.0;
    double stepOver_ = 0.4;        // 40% of tool diameter
    double stepDown_ = 2.0;
    double targetZ_ = -10.0;
    double safeZ_ = 5.0;
    double feedCut_ = 1500.0;
    double feedPlunge_ = 250.0;
    double helixAngle_ = 3.0;     // degrees
    double liftDistance_ = 1.0;

    // Internal methods for adaptive path generation
    void generateHelixEntry(Toolpath& tp, const gp_Pnt& center,
                            double radius, double startZ, double endZ) const;
    void generateAdaptivePasses(Toolpath& tp, double currentZ) const;
};

} // namespace MilCAD

#endif // MILCAD_ADAPTIVE_OPERATION_H
