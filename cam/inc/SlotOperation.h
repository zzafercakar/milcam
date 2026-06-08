/**
 * @file SlotOperation.h
 * @brief Slot milling operation — cuts a slot along a defined path.
 *
 * Ported from FreeCAD's CAM Slot operation. Generates linear toolpaths
 * that trace a slot (elongated pocket) defined by two endpoints and a width.
 */

#ifndef MILCAD_SLOT_OPERATION_H
#define MILCAD_SLOT_OPERATION_H

#include "Operation.h"

#include <gp_Pnt.hxx>

namespace MilCAD {

/**
 * @class SlotOperation
 * @brief Generates toolpaths for slot milling (elongated pocket).
 *
 * A slot is defined by two endpoint centers and the tool width.
 * The toolpath traces back and forth along the slot center line
 * at progressive depth levels.
 */
class SlotOperation : public Operation
{
public:
    SlotOperation();

    std::string typeName() const override { return "Slot"; }
    Toolpath generateToolpath() const override;

    void setStartPoint(const gp_Pnt& p) { startPoint_ = p; }
    const gp_Pnt& startPoint() const { return startPoint_; }

    void setEndPoint(const gp_Pnt& p) { endPoint_ = p; }
    const gp_Pnt& endPoint() const { return endPoint_; }

    void setTargetZ(double z) { targetZ_ = z; }
    double targetZ() const { return targetZ_; }

    void setStepDown(double d) { stepDown_ = d; }
    double stepDown() const { return stepDown_; }

    void setSafeZ(double z) { safeZ_ = z; }
    double safeZ() const { return safeZ_; }

    void setFeedCut(double f) { feedCut_ = f; }
    double feedCut() const { return feedCut_; }

    void setFeedPlunge(double f) { feedPlunge_ = f; }
    double feedPlunge() const { return feedPlunge_; }

private:
    gp_Pnt startPoint_ = gp_Pnt(0, 0, 0);
    gp_Pnt endPoint_ = gp_Pnt(50, 0, 0);
    double targetZ_ = -5.0;
    double stepDown_ = 1.0;
    double safeZ_ = 5.0;
    double feedCut_ = 1000.0;
    double feedPlunge_ = 250.0;
};

} // namespace MilCAD

#endif // MILCAD_SLOT_OPERATION_H
