/**
 * @file DrillOperation.cpp
 * @brief Drilling CAM operation with support for standard, peck, and chip-break cycles.
 *
 * Generates toolpath segments for drilling holes at a list of XY point locations.
 * Supports three canned cycle types: simple drill (G81), full-retract peck (G83),
 * and chip-break peck (G73). Each hole sequence consists of rapid positioning,
 * one or more plunge moves (with intermediate retracts for peck cycles), and a
 * final retract to the safe/retract plane.
 */

#include "DrillOperation.h"

#include <algorithm>
#include <cmath>

namespace MilCAD {

/**
 * @brief Generate the complete drilling toolpath for all configured hole locations.
 * @return Toolpath containing rapid, plunge, and retract segments for every hole.
 *
 * For non-peck cycles, a single plunge from retract-Z to target-Z is emitted.
 * For peck cycles (G83/G73), the hole is drilled in increments of peckDepth_,
 * with a full retract (G83) or short chip-break retract (G73) between pecks.
 */
Toolpath DrillOperation::generateToolpath() const
{
    Toolpath out;
    if (points_.empty() || feedPlunge_ <= 1e-9 || retractZ_ <= targetZ_)
        return out;

    const bool isPeck = (cycleType_ == DrillCycleType::G83 || cycleType_ == DrillCycleType::G73);
    const double effectivePeck = (peckDepth_ > 1e-9) ? peckDepth_ : std::abs(targetZ_ - retractZ_);

    for (const auto &p : points_) {
        const gp_Pnt pRetract(p.X(), p.Y(), retractZ_);
        const gp_Pnt pTarget(p.X(), p.Y(), targetZ_);

        ToolpathSegment rapidToHole;
        rapidToHole.type = ToolpathSegmentType::Rapid;
        rapidToHole.start = pRetract;
        rapidToHole.end = pRetract;
        out.add(rapidToHole);

        if (!isPeck) {
            ToolpathSegment plunge;
            plunge.type = ToolpathSegmentType::Plunge;
            plunge.start = pRetract;
            plunge.end = pTarget;
            plunge.feedRate = feedPlunge_;
            if (cycleType_ == DrillCycleType::G82 && dwellSeconds_ > 1e-9)
                plunge.dwellSeconds = dwellSeconds_;
            out.add(plunge);
        } else {
            double drilledZ = retractZ_;
            while (drilledZ - targetZ_ > 1e-9) {
                const double nextZ = std::max(drilledZ - effectivePeck, targetZ_);

                ToolpathSegment plunge;
                plunge.type = ToolpathSegmentType::Plunge;
                plunge.start = gp_Pnt(p.X(), p.Y(), (drilledZ < retractZ_ ? drilledZ : retractZ_));
                plunge.end = gp_Pnt(p.X(), p.Y(), nextZ);
                plunge.feedRate = feedPlunge_;
                out.add(plunge);

                if (nextZ > targetZ_ + 1e-9) {
                    const double chipBreakRetract = (cycleType_ == DrillCycleType::G73)
                        ? std::min(nextZ + 1.0, retractZ_)
                        : retractZ_;
                    ToolpathSegment retract;
                    retract.type = ToolpathSegmentType::Rapid;
                    retract.start = gp_Pnt(p.X(), p.Y(), nextZ);
                    retract.end = gp_Pnt(p.X(), p.Y(), chipBreakRetract);
                    out.add(retract);
                }
                drilledZ = nextZ;
            }
        }

        ToolpathSegment retract;
        retract.type = ToolpathSegmentType::Rapid;
        retract.start = pTarget;
        retract.end = pRetract;
        out.add(retract);
    }

    return out;
}

} // namespace MilCAD
