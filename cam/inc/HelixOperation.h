/**
 * @file HelixOperation.h
 * @brief Helix operation — helical interpolation for hole making/ramping.
 *
 * Ported from FreeCAD's CAM Helix operation. Generates a helical toolpath
 * for circular hole enlargement, thread milling, or ramping into material.
 */

#ifndef MILCAD_HELIX_OPERATION_H
#define MILCAD_HELIX_OPERATION_H

#include "Operation.h"

#include <gp_Pnt.hxx>

namespace MilCAD {

/**
 * @class HelixOperation
 * @brief Generates helical toolpaths for circular features.
 *
 * Creates a continuous helical path descending from startZ to targetZ
 * at a given center point and radius. Useful for:
 *   - Enlarging pre-drilled holes to a precise diameter
 *   - Thread milling
 *   - Helical ramping into material (as an alternative to plunging)
 */
class HelixOperation : public Operation
{
public:
    HelixOperation();

    std::string typeName() const override { return "Helix"; }
    Toolpath generateToolpath() const override;

    void setCenter(const gp_Pnt& center) { center_ = center; }
    const gp_Pnt& center() const { return center_; }

    void setRadius(double r) { radius_ = r; }
    double radius() const { return radius_; }

    void setTargetZ(double z) { targetZ_ = z; }
    double targetZ() const { return targetZ_; }

    void setStartZ(double z) { startZ_ = z; }
    double startZ() const { return startZ_; }

    void setSafeZ(double z) { safeZ_ = z; }
    double safeZ() const { return safeZ_; }

    void setStepDown(double d) { stepDown_ = d; }
    double stepDown() const { return stepDown_; }

    void setFeedCut(double f) { feedCut_ = f; }
    double feedCut() const { return feedCut_; }

    void setFeedPlunge(double f) { feedPlunge_ = f; }
    double feedPlunge() const { return feedPlunge_; }

    /// Direction: true = CW (G2), false = CCW (G3)
    void setClockwise(bool cw) { clockwise_ = cw; }
    bool clockwise() const { return clockwise_; }

private:
    gp_Pnt center_ = gp_Pnt(0, 0, 0);
    double radius_ = 5.0;
    double targetZ_ = -10.0;
    double startZ_ = 0.0;
    double safeZ_ = 5.0;
    double stepDown_ = 1.0;
    double feedCut_ = 800.0;
    double feedPlunge_ = 200.0;
    bool clockwise_ = true;
};

} // namespace MilCAD

#endif // MILCAD_HELIX_OPERATION_H
