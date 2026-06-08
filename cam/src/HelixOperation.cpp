/**
 * @file HelixOperation.cpp
 * @brief Implementation of HelixOperation — helical interpolation.
 *
 * Generates circular arcs (G2/G3) with progressive Z descent to create
 * a helical toolpath. The helix descends stepDown per revolution until
 * reaching the target Z depth.
 */

#include "HelixOperation.h"

#include <cmath>

namespace MilCAD {

HelixOperation::HelixOperation()
{
    setName("Helix");
}

Toolpath HelixOperation::generateToolpath() const
{
    Toolpath tp;

    if (radius_ < 0.01 || std::fabs(targetZ_ - startZ_) < 1e-6) return tp;

    double totalDepth = std::fabs(startZ_ - targetZ_);
    double depthPerRev = stepDown_;
    if (depthPerRev <= 0.0) depthPerRev = 1.0;

    int numRevolutions = std::max(1, static_cast<int>(std::ceil(totalDepth / depthPerRev)));

    // Entry point: offset from center by radius in X direction
    gp_Pnt entryPt(center_.X() + radius_, center_.Y(), startZ_);

    // Rapid to safe Z above center
    {
        ToolpathSegment rapid;
        rapid.type = ToolpathSegmentType::Rapid;
        rapid.start = gp_Pnt(0, 0, safeZ_);
        rapid.end = gp_Pnt(center_.X() + radius_, center_.Y(), safeZ_);
        tp.add(rapid);
    }

    // Plunge to start Z
    {
        ToolpathSegment plunge;
        plunge.type = ToolpathSegmentType::Plunge;
        plunge.start = gp_Pnt(center_.X() + radius_, center_.Y(), safeZ_);
        plunge.end = entryPt;
        plunge.feedRate = feedPlunge_;
        tp.add(plunge);
    }

    // Generate helical arcs (two semicircles per revolution for smoother path)
    double currentZ = startZ_;
    double zPerHalfRev = depthPerRev / 2.0;
    auto arcType = clockwise_ ? ToolpathSegmentType::CWArc : ToolpathSegmentType::CCWArc;

    for (int rev = 0; rev < numRevolutions; ++rev) {
        // First half revolution: entry side → opposite side
        double z1 = std::max(currentZ - zPerHalfRev, targetZ_);
        {
            ToolpathSegment arc;
            arc.type = arcType;
            arc.start = gp_Pnt(center_.X() + radius_, center_.Y(), currentZ);
            arc.end = gp_Pnt(center_.X() - radius_, center_.Y(), z1);
            arc.center = gp_Pnt(center_.X(), center_.Y(), (currentZ + z1) * 0.5);
            arc.feedRate = feedCut_;
            tp.add(arc);
        }

        currentZ = z1;
        if (currentZ <= targetZ_ + 1e-6) break;

        // Second half revolution: opposite side → entry side
        double z2 = std::max(currentZ - zPerHalfRev, targetZ_);
        {
            ToolpathSegment arc;
            arc.type = arcType;
            arc.start = gp_Pnt(center_.X() - radius_, center_.Y(), currentZ);
            arc.end = gp_Pnt(center_.X() + radius_, center_.Y(), z2);
            arc.center = gp_Pnt(center_.X(), center_.Y(), (currentZ + z2) * 0.5);
            arc.feedRate = feedCut_;
            tp.add(arc);
        }

        currentZ = z2;
        if (currentZ <= targetZ_ + 1e-6) break;
    }

    // Optional: final full-depth revolution at target Z for finish
    {
        ToolpathSegment arc1;
        arc1.type = arcType;
        arc1.start = gp_Pnt(center_.X() + radius_, center_.Y(), targetZ_);
        arc1.end = gp_Pnt(center_.X() - radius_, center_.Y(), targetZ_);
        arc1.center = gp_Pnt(center_.X(), center_.Y(), targetZ_);
        arc1.feedRate = feedCut_;
        tp.add(arc1);

        ToolpathSegment arc2;
        arc2.type = arcType;
        arc2.start = gp_Pnt(center_.X() - radius_, center_.Y(), targetZ_);
        arc2.end = gp_Pnt(center_.X() + radius_, center_.Y(), targetZ_);
        arc2.center = gp_Pnt(center_.X(), center_.Y(), targetZ_);
        arc2.feedRate = feedCut_;
        tp.add(arc2);
    }

    // Retract to safe Z
    {
        ToolpathSegment retract;
        retract.type = ToolpathSegmentType::Rapid;
        retract.start = gp_Pnt(center_.X() + radius_, center_.Y(), targetZ_);
        retract.end = gp_Pnt(center_.X() + radius_, center_.Y(), safeZ_);
        tp.add(retract);
    }

    return tp;
}

} // namespace MilCAD
