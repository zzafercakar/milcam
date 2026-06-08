/**
 * @file ProfileOperation.cpp
 * @brief Profile (contour) milling CAM operation with professional parameters.
 *
 * Generates a toolpath that follows a closed 2D contour at progressively
 * deeper Z levels. Supports lead-in/out arcs for smooth tool engagement,
 * holding tabs for workpiece retention, stock-to-leave for finishing passes,
 * and climb/conventional milling direction control.
 */

#include "ProfileOperation.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace MilCAD {

namespace {

/// Pi constant for trigonometric calculations
constexpr double kPi = 3.14159265358979323846;

/**
 * @brief Compute cumulative distances along a closed contour ring.
 * @param ring The contour points (should be closed: first == last).
 * @return Vector of cumulative distances from ring[0] to each vertex.
 */
std::vector<double> computeCumulativeDistances(const std::vector<gp_Pnt> &ring)
{
    std::vector<double> dists(ring.size(), 0.0);
    for (std::size_t i = 1; i < ring.size(); ++i)
        dists[i] = dists[i - 1] + ring[i - 1].Distance(ring[i]);
    return dists;
}

/**
 * @brief Check if a position along the contour falls inside a tab zone.
 * @param dist Current cumulative distance along the contour.
 * @param totalLen Total contour perimeter length.
 * @param tabWidth Width of each tab.
 * @param tabCount Number of evenly-distributed tabs.
 * @return true if the position is inside a tab zone.
 */
bool isInTabZone(double dist, double totalLen, double tabWidth, int tabCount)
{
    if (tabCount <= 0 || tabWidth <= 0.0 || totalLen <= 0.0)
        return false;

    // Evenly distribute tabs along the contour perimeter
    const double spacing = totalLen / tabCount;
    for (int t = 0; t < tabCount; ++t) {
        const double tabCenter = spacing * (t + 0.5);
        const double tabStart = tabCenter - tabWidth / 2.0;
        const double tabEnd = tabCenter + tabWidth / 2.0;
        if (dist >= tabStart && dist <= tabEnd)
            return true;
    }
    return false;
}

/**
 * @brief Compute the first-segment direction vector of the contour.
 * @param ring Closed contour ring.
 * @return Normalized direction from ring[0] to ring[1].
 */
gp_Vec firstSegmentDir(const std::vector<gp_Pnt> &ring)
{
    gp_Vec dir(ring[0], ring[1]);
    if (dir.Magnitude() > 1e-12)
        dir.Normalize();
    return dir;
}

/**
 * @brief Compute the last-segment direction vector of the contour.
 * @param ring Closed contour ring (last point == first point).
 * @return Normalized direction of the segment ending at ring.back().
 */
gp_Vec lastSegmentDir(const std::vector<gp_Pnt> &ring)
{
    // ring[size-2] -> ring[size-1] is the last actual segment
    gp_Vec dir(ring[ring.size() - 2], ring[ring.size() - 1]);
    if (dir.Magnitude() > 1e-12)
        dir.Normalize();
    return dir;
}

} // anonymous namespace

/**
 * @brief Generate the profile toolpath with professional CAM features.
 * @return Toolpath with lead-in/out, tabs, stock-to-leave, and multi-depth passes.
 *
 * Algorithm per depth level:
 * 1. Rapid to safe-Z above contour start
 * 2. Plunge to cutting depth
 * 3. Generate lead-in arc/line (smooth tool engagement)
 * 4. Trace contour segments with tab height adjustments
 * 5. Generate lead-out arc/line (smooth tool disengagement)
 * 6. Retract to safe-Z
 *
 * Stock-to-leave is applied as an axial offset (floor is raised by the amount).
 * Radial stock-to-leave must be handled by the contour offset upstream.
 */
Toolpath ProfileOperation::generateToolpath() const
{
    Toolpath out;

    // Resolve contour: prefer CamGeometrySource if set, otherwise use manual contour
    std::vector<gp_Pnt> resolvedContour = contour_;
    if (geometrySource() && geometrySource()->isValid()) {
        auto fromSource = geometrySource()->resolveContour();
        if (!fromSource.empty())
            resolvedContour = std::move(fromSource);
    }

    if (resolvedContour.size() < 2 || stepDown_ <= 1e-9 || feedCut_ <= 1e-9 || feedPlunge_ <= 1e-9)
        return out;

    // Build a closed contour ring — auto-close if first != last
    std::vector<gp_Pnt> ring = resolvedContour;

    // Reverse contour direction for conventional milling (CW vs CCW swap)
    if (millDirection_ == MillDirection::Conventional)
        std::reverse(ring.begin(), ring.end());

    // Auto-close: append first point if ring is not already closed
    if (ring.front().Distance(ring.back()) > 1e-9)
        ring.push_back(ring.front());
    if (ring.size() < 3)
        return out;

    // Apply axial stock-to-leave: raise the effective floor
    const double effectiveTargetZ = targetZ_ + stockToLeaveAxial_;

    // Precompute cumulative distances for tab placement
    const auto cumDists = computeCumulativeDistances(ring);
    const double totalLen = cumDists.back();

    // Direction vectors for lead-in/out geometry
    const gp_Vec firstDir = firstSegmentDir(ring);
    const gp_Vec lastDir = lastSegmentDir(ring);

    // Perpendicular inward normal (left-hand rule for CCW contour)
    const gp_Vec inwardNormal(-firstDir.Y(), firstDir.X(), 0.0);

    const double startZ = ring.front().Z();

    for (double z = startZ - stepDown_; z >= effectiveTargetZ - 1e-9; z -= stepDown_) {
        const double cutZ = std::max(z, effectiveTargetZ);
        const gp_Pnt contourStart(ring.front().X(), ring.front().Y(), cutZ);

        // --- Compute lead-in start position ---
        gp_Pnt approachPt = contourStart; // Default: no lead-in, start at contour

        if (leadInType_ == LeadType::Arc && leadInRadius_ > 1e-9) {
            // Arc lead-in: start from a point offset by radius along inward normal
            approachPt = gp_Pnt(
                contourStart.X() + inwardNormal.X() * leadInRadius_,
                contourStart.Y() + inwardNormal.Y() * leadInRadius_,
                cutZ);
        } else if (leadInType_ == LeadType::Line && leadInRadius_ > 1e-9) {
            // Line lead-in: start from a point along the negative first-segment direction
            approachPt = gp_Pnt(
                contourStart.X() - firstDir.X() * leadInRadius_,
                contourStart.Y() - firstDir.Y() * leadInRadius_,
                cutZ);
        }

        // 1. Rapid positioning to safe-Z above approach point
        ToolpathSegment rapidToStart;
        rapidToStart.type = ToolpathSegmentType::Rapid;
        rapidToStart.start = gp_Pnt(approachPt.X(), approachPt.Y(), safeZ_);
        rapidToStart.end = gp_Pnt(approachPt.X(), approachPt.Y(), safeZ_);
        out.add(rapidToStart);

        // 2. Plunge to cutting depth at approach point
        ToolpathSegment plunge;
        plunge.type = ToolpathSegmentType::Plunge;
        plunge.start = gp_Pnt(approachPt.X(), approachPt.Y(), safeZ_);
        plunge.end = approachPt;
        plunge.feedRate = feedPlunge_;
        out.add(plunge);

        // 3. Lead-in motion (arc or line from approach point to contour start)
        if (leadInType_ == LeadType::Arc && leadInRadius_ > 1e-9) {
            // CCW arc from approach point to contour start, center at contour start
            ToolpathSegment leadIn;
            leadIn.type = ToolpathSegmentType::CCWArc;
            leadIn.start = approachPt;
            leadIn.end = contourStart;
            // Arc center is at the contour start point (tangent entry)
            leadIn.center = contourStart;
            leadIn.feedRate = feedCut_;
            out.add(leadIn);
        } else if (leadInType_ == LeadType::Line && leadInRadius_ > 1e-9) {
            // Linear approach from offset to contour start
            ToolpathSegment leadIn;
            leadIn.type = ToolpathSegmentType::Linear;
            leadIn.start = approachPt;
            leadIn.end = contourStart;
            leadIn.feedRate = feedCut_;
            out.add(leadIn);
        }

        // 4. Trace contour segments with tab height adjustments
        for (std::size_t i = 1; i < ring.size(); ++i) {
            const double segMidDist = (cumDists[i - 1] + cumDists[i]) / 2.0;
            double segZ = cutZ;

            // If tabs are enabled on the final depth pass, raise Z in tab zones
            if (tabsEnabled_ && tabCount_ > 0 && cutZ <= effectiveTargetZ + 1e-9) {
                if (isInTabZone(segMidDist, totalLen, tabWidth_, tabCount_))
                    segZ = cutZ + tabHeight_;
            }

            ToolpathSegment seg;
            seg.type = ToolpathSegmentType::Linear;
            seg.start = gp_Pnt(ring[i - 1].X(), ring[i - 1].Y(), segZ);
            seg.end = gp_Pnt(ring[i].X(), ring[i].Y(), segZ);
            seg.feedRate = feedCut_;
            out.add(seg);
        }

        // 5. Lead-out motion (arc or line from contour end outward)
        const gp_Pnt contourEnd(ring.back().X(), ring.back().Y(), cutZ);

        if (leadOutType_ == LeadType::Arc && leadOutRadius_ > 1e-9) {
            // Outward perpendicular from last segment direction
            const gp_Vec outNormal(-lastDir.Y(), lastDir.X(), 0.0);
            gp_Pnt exitPt(
                contourEnd.X() + outNormal.X() * leadOutRadius_,
                contourEnd.Y() + outNormal.Y() * leadOutRadius_,
                cutZ);

            ToolpathSegment leadOut;
            leadOut.type = ToolpathSegmentType::CCWArc;
            leadOut.start = contourEnd;
            leadOut.end = exitPt;
            leadOut.center = contourEnd; // Tangent exit arc
            leadOut.feedRate = feedCut_;
            out.add(leadOut);
        } else if (leadOutType_ == LeadType::Line && leadOutRadius_ > 1e-9) {
            gp_Pnt exitPt(
                contourEnd.X() + lastDir.X() * leadOutRadius_,
                contourEnd.Y() + lastDir.Y() * leadOutRadius_,
                cutZ);

            ToolpathSegment leadOut;
            leadOut.type = ToolpathSegmentType::Linear;
            leadOut.start = contourEnd;
            leadOut.end = exitPt;
            leadOut.feedRate = feedCut_;
            out.add(leadOut);
        }

        // 6. Retract to safe-Z
        if (out.segments().empty())
            continue;
        const gp_Pnt &lastPt = out.segments().back().end;
        ToolpathSegment retract;
        retract.type = ToolpathSegmentType::Rapid;
        retract.start = lastPt;
        retract.end = gp_Pnt(lastPt.X(), lastPt.Y(), safeZ_);
        out.add(retract);
    }

    return out;
}

} // namespace MilCAD
