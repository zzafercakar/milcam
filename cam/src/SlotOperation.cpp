/**
 * @file SlotOperation.cpp
 * @brief Implementation of SlotOperation — slot milling along a path.
 *
 * Generates a simple back-and-forth toolpath along the slot centerline
 * at progressive depth levels. Each pass traces from start to end (or
 * end to start for alternating direction), stepping down until target Z.
 */

#include "SlotOperation.h"

#include <cmath>

namespace MilCAD {

SlotOperation::SlotOperation()
{
    setName("Slot");
}

Toolpath SlotOperation::generateToolpath() const
{
    Toolpath tp;

    double startZ = 0.0;  // Top of stock
    double effectiveTargetZ = targetZ_;
    double currentZ = startZ;

    // Rapid to safe Z above start point
    {
        ToolpathSegment rapid;
        rapid.type = ToolpathSegmentType::Rapid;
        rapid.start = gp_Pnt(0, 0, safeZ_);
        rapid.end = gp_Pnt(startPoint_.X(), startPoint_.Y(), safeZ_);
        tp.add(rapid);
    }

    bool forward = true;  // Alternating direction flag

    while (currentZ > effectiveTargetZ + 1e-6) {
        double nextZ = std::max(currentZ - stepDown_, effectiveTargetZ);

        gp_Pnt from = forward ? startPoint_ : endPoint_;
        gp_Pnt to   = forward ? endPoint_ : startPoint_;

        // Plunge to cutting depth at the approach point
        {
            ToolpathSegment plunge;
            plunge.type = ToolpathSegmentType::Plunge;
            plunge.start = gp_Pnt(from.X(), from.Y(), safeZ_);
            plunge.end = gp_Pnt(from.X(), from.Y(), nextZ);
            plunge.feedRate = feedPlunge_;
            tp.add(plunge);
        }

        // Cut along the slot
        {
            ToolpathSegment cut;
            cut.type = ToolpathSegmentType::Linear;
            cut.start = gp_Pnt(from.X(), from.Y(), nextZ);
            cut.end = gp_Pnt(to.X(), to.Y(), nextZ);
            cut.feedRate = feedCut_;
            tp.add(cut);
        }

        // Retract for next pass
        {
            ToolpathSegment retract;
            retract.type = ToolpathSegmentType::Rapid;
            retract.start = gp_Pnt(to.X(), to.Y(), nextZ);
            retract.end = gp_Pnt(to.X(), to.Y(), safeZ_);
            tp.add(retract);
        }

        currentZ = nextZ;
        forward = !forward;  // Alternate direction
    }

    // Final retract to safe Z
    if (!tp.empty()) {
        ToolpathSegment finalRetract;
        finalRetract.type = ToolpathSegmentType::Rapid;
        finalRetract.start = tp.segments().back().end;
        finalRetract.end = gp_Pnt(startPoint_.X(), startPoint_.Y(), safeZ_);
        tp.add(finalRetract);
    }

    return tp;
}

} // namespace MilCAD
