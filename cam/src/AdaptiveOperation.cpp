/**
 * @file AdaptiveOperation.cpp
 * @brief Implementation of AdaptiveOperation — constant engagement clearing.
 *
 * The algorithm generates concentric offset passes that maintain a constant
 * tool engagement angle. For the initial entry, a helix ramp is used to
 * avoid plunging directly into the material.
 *
 * This is a simplified implementation inspired by FreeCAD's libarea Adaptive
 * algorithm. The full algorithm uses Clipper polygon operations and cleared-area
 * tracking, but this version uses a geometric offset spiral approach.
 */

#include "AdaptiveOperation.h"

#include <cmath>

namespace MilCAD {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

AdaptiveOperation::AdaptiveOperation()
{
    setName("Adaptive");
}

void AdaptiveOperation::generateHelixEntry(
    Toolpath& tp,
    const gp_Pnt& center,
    double radius,
    double startZ,
    double endZ) const
{
    // Generate a helix ramp from startZ to endZ at the given center/radius
    // Number of revolutions determined by helix angle and depth
    double depth = std::fabs(endZ - startZ);
    if (depth < 1e-6 || radius < 1e-6) return;

    // Compute helix parameters
    double helixAngleRad = helixAngle_ * kPi / 180.0;
    double pitchPerRev = 2.0 * kPi * radius * std::tan(helixAngleRad);
    if (pitchPerRev < 1e-6) pitchPerRev = 0.5;  // Minimum pitch

    int numRevolutions = std::max(1, static_cast<int>(std::ceil(depth / pitchPerRev)));
    int pointsPerRev = 36;  // 10-degree increments
    int totalPoints = numRevolutions * pointsPerRev;
    double zStep = depth / totalPoints;
    double angleStep = 2.0 * kPi / pointsPerRev;

    double currentZ = startZ;
    double currentAngle = 0.0;

    for (int i = 0; i < totalPoints; ++i) {
        double nextAngle = currentAngle + angleStep;
        double nextZ = currentZ - zStep;

        gp_Pnt startPt(center.X() + radius * std::cos(currentAngle),
                        center.Y() + radius * std::sin(currentAngle),
                        currentZ);
        gp_Pnt endPt(center.X() + radius * std::cos(nextAngle),
                      center.Y() + radius * std::sin(nextAngle),
                      nextZ);

        ToolpathSegment seg;
        seg.type = ToolpathSegmentType::Linear;
        seg.start = startPt;
        seg.end = endPt;
        seg.feedRate = feedPlunge_;
        tp.add(seg);

        currentAngle = nextAngle;
        currentZ = nextZ;
    }
}

void AdaptiveOperation::generateAdaptivePasses(
    Toolpath& tp,
    double currentZ) const
{
    if (contour_.size() < 3) return;

    // Compute contour centroid for helix center
    double cx = 0.0, cy = 0.0;
    for (const auto& pt : contour_) {
        cx += pt.X();
        cy += pt.Y();
    }
    cx /= contour_.size();
    cy /= contour_.size();

    // Compute maximum radius from centroid to contour
    double maxRadius = 0.0;
    for (const auto& pt : contour_) {
        double d = std::sqrt((pt.X() - cx) * (pt.X() - cx) +
                             (pt.Y() - cy) * (pt.Y() - cy));
        maxRadius = std::max(maxRadius, d);
    }

    // Step-over distance in mm
    double stepOverDist = toolDiameter_ * stepOver_;
    if (stepOverDist < 0.1) stepOverDist = 0.1;

    // Generate concentric offset passes from center outward (for clearing inside)
    // or from contour inward (for profiling)
    double startRadius = stepOverDist;
    double endRadius = maxRadius - toolDiameter_ * 0.5;

    if (opType_ == AdaptiveOpType::ClearingInside ||
        opType_ == AdaptiveOpType::ProfilingInside) {
        // Spiral outward from center
        for (double r = startRadius; r <= endRadius; r += stepOverDist) {
            int points = std::max(12, static_cast<int>(2.0 * kPi * r / 2.0));
            double angleStep = 2.0 * kPi / points;

            for (int i = 0; i < points; ++i) {
                double a1 = i * angleStep;
                double a2 = (i + 1) * angleStep;

                gp_Pnt p1(cx + r * std::cos(a1), cy + r * std::sin(a1), currentZ);
                gp_Pnt p2(cx + r * std::cos(a2), cy + r * std::sin(a2), currentZ);

                ToolpathSegment seg;
                seg.type = ToolpathSegmentType::Linear;
                seg.start = p1;
                seg.end = p2;
                seg.feedRate = feedCut_;
                tp.add(seg);
            }

            // Link to next radius with a smooth transition
            if (r + stepOverDist <= endRadius) {
                double a = 0.0;
                gp_Pnt linkStart(cx + r * std::cos(a), cy + r * std::sin(a), currentZ);
                gp_Pnt linkEnd(cx + (r + stepOverDist) * std::cos(a),
                               cy + (r + stepOverDist) * std::sin(a), currentZ);

                ToolpathSegment link;
                link.type = ToolpathSegmentType::Linear;
                link.start = linkStart;
                link.end = linkEnd;
                link.feedRate = feedCut_;
                tp.add(link);
            }
        }
    } else {
        // Profiling outside or clearing outside — trace along contour with offset
        for (size_t i = 0; i + 1 < contour_.size(); ++i) {
            ToolpathSegment seg;
            seg.type = ToolpathSegmentType::Linear;
            seg.start = gp_Pnt(contour_[i].X(), contour_[i].Y(), currentZ);
            seg.end = gp_Pnt(contour_[i + 1].X(), contour_[i + 1].Y(), currentZ);
            seg.feedRate = feedCut_;
            tp.add(seg);
        }
    }
}

Toolpath AdaptiveOperation::generateToolpath() const
{
    Toolpath tp;

    if (contour_.size() < 3) return tp;

    // Compute contour centroid
    double cx = 0.0, cy = 0.0;
    for (const auto& pt : contour_) {
        cx += pt.X();
        cy += pt.Y();
    }
    cx /= contour_.size();
    cy /= contour_.size();
    gp_Pnt center(cx, cy, 0.0);

    // Start Z is the top of the stock (Z=0 by convention)
    double startZ = 0.0;
    double effectiveTargetZ = targetZ_;

    // Rapid to safe height above entry point
    {
        ToolpathSegment rapid;
        rapid.type = ToolpathSegmentType::Rapid;
        rapid.start = gp_Pnt(0, 0, safeZ_);
        rapid.end = gp_Pnt(cx, cy, safeZ_);
        tp.add(rapid);
    }

    // Process each depth level
    double currentZ = startZ;
    while (currentZ > effectiveTargetZ + 1e-6) {
        double nextZ = std::max(currentZ - stepDown_, effectiveTargetZ);

        // Helix ramp entry to new depth level
        double helixRadius = toolDiameter_ * stepOver_ * 0.5;
        generateHelixEntry(tp, center, helixRadius, currentZ, nextZ);

        // Adaptive clearing passes at this depth
        generateAdaptivePasses(tp, nextZ);

        // Retract for next depth or completion
        if (nextZ > effectiveTargetZ + 1e-6) {
            ToolpathSegment retract;
            retract.type = ToolpathSegmentType::Rapid;
            retract.start = tp.segments().back().end;
            retract.end = gp_Pnt(cx, cy, safeZ_);
            tp.add(retract);
        }

        currentZ = nextZ;
    }

    // Final retract to safe height
    if (!tp.empty()) {
        ToolpathSegment finalRetract;
        finalRetract.type = ToolpathSegmentType::Rapid;
        finalRetract.start = tp.segments().back().end;
        finalRetract.end = gp_Pnt(cx, cy, safeZ_);
        tp.add(finalRetract);
    }

    return tp;
}

} // namespace MilCAD
