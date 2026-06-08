/**
 * @file PocketOperation.cpp
 * @brief Rectangular pocket milling with ramp/helix entry and stock-to-leave.
 *
 * Generates a zigzag raster toolpath to clear a rectangular pocket area.
 * Supports three entry strategies: direct plunge, linear ramp, and helical
 * entry. Stock-to-leave applies radial (wall) and axial (floor) offsets
 * for roughing/finishing workflows.
 */

#include "PocketOperation.h"

#include <algorithm>
#include <cmath>

namespace MilCAD {

namespace {

constexpr double kPi = 3.14159265358979323846;

/**
 * @brief Generate a linear ramp entry from safe-Z to cut-Z.
 *
 * The ramp moves diagonally from (sx, rowY, prevZ) to (sx + rampLen, rowY, cutZ),
 * distributing the plunge load over horizontal distance. Ramp length is computed
 * from the angle: rampLen = deltaZ / tan(angle).
 *
 * @param out     Toolpath to append segments to.
 * @param sx      Start X position.
 * @param rowY    Y position of the first row.
 * @param prevZ   Previous Z height (start of ramp).
 * @param cutZ    Target Z depth (end of ramp).
 * @param maxX    Maximum X extent (pocket boundary, clamps ramp length).
 * @param angle   Ramp angle in degrees.
 * @param feed    Feed rate for ramp motion (mm/min).
 */
void generateRampEntry(Toolpath &out, double sx, double rowY,
                       double prevZ, double cutZ, double maxX,
                       double angle, double feed)
{
    const double deltaZ = std::abs(prevZ - cutZ);
    const double tanA = std::tan(angle * kPi / 180.0);
    if (tanA <= 1e-12) return;

    // Ramp horizontal length, clamped to pocket width
    const double rampLen = std::min(deltaZ / tanA, maxX - sx);

    ToolpathSegment ramp;
    ramp.type = ToolpathSegmentType::Linear;
    ramp.start = gp_Pnt(sx, rowY, prevZ);
    ramp.end = gp_Pnt(sx + rampLen, rowY, cutZ);
    ramp.feedRate = feed;
    out.add(ramp);

    // Move back to row start at cutting depth for consistent raster
    if (rampLen > 1e-9) {
        ToolpathSegment backtrack;
        backtrack.type = ToolpathSegmentType::Linear;
        backtrack.start = gp_Pnt(sx + rampLen, rowY, cutZ);
        backtrack.end = gp_Pnt(sx, rowY, cutZ);
        backtrack.feedRate = feed;
        out.add(backtrack);
    }
}

/**
 * @brief Generate a helical entry from safe-Z to cut-Z.
 *
 * Descends in a helix (approximated as quarter-circle CCW arc segments)
 * around the pocket center. Each quarter turn drops by a proportional
 * Z increment, distributing tool load over the helical path.
 *
 * @param out     Toolpath to append segments to.
 * @param cx      Helix center X.
 * @param cy      Helix center Y.
 * @param prevZ   Start Z height.
 * @param cutZ    Target Z depth.
 * @param radius  Helix radius in mm.
 * @param feed    Feed rate for helix motion (mm/min).
 */
void generateHelixEntry(Toolpath &out, double cx, double cy,
                        double prevZ, double cutZ, double radius, double feed)
{
    const double deltaZ = std::abs(prevZ - cutZ);
    if (deltaZ <= 1e-9 || radius <= 1e-9) return;

    // Approximate helix with quarter-circle CCW arcs, each dropping proportionally
    const int quarterTurns = std::max(4, static_cast<int>(std::ceil(deltaZ / 0.5)));
    const double dzPerQuarter = deltaZ / quarterTurns;

    double curZ = prevZ;
    // Start position at (cx + radius, cy)
    double curAngle = 0.0;
    const double quarterAngle = kPi / 2.0;

    for (int q = 0; q < quarterTurns; ++q) {
        const double startAngle = curAngle;
        const double endAngle = curAngle + quarterAngle;
        const double endZ = std::max(curZ - dzPerQuarter, cutZ);

        const gp_Pnt arcStart(cx + radius * std::cos(startAngle),
                              cy + radius * std::sin(startAngle), curZ);
        const gp_Pnt arcEnd(cx + radius * std::cos(endAngle),
                            cy + radius * std::sin(endAngle), endZ);

        ToolpathSegment arc;
        arc.type = ToolpathSegmentType::CCWArc;
        arc.start = arcStart;
        arc.end = arcEnd;
        arc.center = gp_Pnt(cx, cy, (curZ + endZ) / 2.0);
        arc.feedRate = feed;
        out.add(arc);

        curZ = endZ;
        curAngle = endAngle;
    }

    // Move from helix end to pocket corner to start raster
    if (out.segments().empty())
        return;
    const auto &lastEnd = out.segments().back().end;
    if (lastEnd.Distance(gp_Pnt(cx - radius, cy, cutZ)) > 1e-9) {
        ToolpathSegment toCorner;
        toCorner.type = ToolpathSegmentType::Linear;
        toCorner.start = lastEnd;
        toCorner.end = gp_Pnt(cx - radius, cy, cutZ);
        toCorner.feedRate = feed;
        out.add(toCorner);
    }
}

} // anonymous namespace

/**
 * @brief Generate the pocket clearing toolpath with professional features.
 * @return Toolpath with entry strategy, zigzag raster passes, and stock-to-leave.
 *
 * Each depth pass:
 * 1. Rapid to safe-Z above entry position
 * 2. Entry motion (plunge, ramp, or helix) to cutting depth
 * 3. Zigzag raster rows across the pocket width
 * 4. Retract to safe-Z
 *
 * Stock-to-leave radial shrinks the pocket bounds inward; axial raises the floor.
 */
Toolpath PocketOperation::generateToolpath() const
{
    Toolpath out;

    // Resolve pocket bounds: if a CamGeometrySource is set, compute bounding box from contour
    double resolvedMinX = minX_, resolvedMinY = minY_;
    double resolvedMaxX = maxX_, resolvedMaxY = maxY_;
    if (geometrySource() && geometrySource()->isValid()) {
        auto contour = geometrySource()->resolveContour();
        if (!contour.empty()) {
            resolvedMinX = resolvedMaxX = contour[0].X();
            resolvedMinY = resolvedMaxY = contour[0].Y();
            for (const auto &pt : contour) {
                resolvedMinX = std::min(resolvedMinX, pt.X());
                resolvedMinY = std::min(resolvedMinY, pt.Y());
                resolvedMaxX = std::max(resolvedMaxX, pt.X());
                resolvedMaxY = std::max(resolvedMaxY, pt.Y());
            }
        }
    }

    // Apply radial stock-to-leave by shrinking pocket bounds
    const double effMinX = resolvedMinX + stockToLeaveRadial_;
    const double effMinY = resolvedMinY + stockToLeaveRadial_;
    const double effMaxX = resolvedMaxX - stockToLeaveRadial_;
    const double effMaxY = resolvedMaxY - stockToLeaveRadial_;

    const double width = effMaxX - effMinX;
    const double height = effMaxY - effMinY;
    if (width <= 1e-9 || height <= 1e-9)
        return out;
    if (stepDown_ <= 1e-9 || stepOver_ <= 1e-9 || feedCut_ <= 1e-9 || feedPlunge_ <= 1e-9)
        return out;

    // Apply axial stock-to-leave by raising the floor
    const double effectiveTargetZ = targetZ_ + stockToLeaveAxial_;

    // Pocket center for helix entry
    const double cx = (effMinX + effMaxX) / 2.0;
    const double cy = (effMinY + effMaxY) / 2.0;

    const double startZ = 0.0;
    double prevLevelZ = startZ; // Track previous depth for entry calculations

    for (double z = startZ - stepDown_; z >= effectiveTargetZ - 1e-9; z -= stepDown_) {
        const double cutZ = std::max(z, effectiveTargetZ);

        // --- Entry motion based on selected strategy ---
        if (entryType_ == PocketEntryType::Helix) {
            // Rapid to safe-Z above helix start
            const double helixR = std::min(helixRadius_, std::min(width, height) / 2.0 - 0.5);
            ToolpathSegment rapidToHelix;
            rapidToHelix.type = ToolpathSegmentType::Rapid;
            rapidToHelix.start = gp_Pnt(cx + helixR, cy, safeZ_);
            rapidToHelix.end = gp_Pnt(cx + helixR, cy, safeZ_);
            out.add(rapidToHelix);

            // Plunge to previous level
            ToolpathSegment plungeToLevel;
            plungeToLevel.type = ToolpathSegmentType::Plunge;
            plungeToLevel.start = gp_Pnt(cx + helixR, cy, safeZ_);
            plungeToLevel.end = gp_Pnt(cx + helixR, cy, prevLevelZ);
            plungeToLevel.feedRate = feedPlunge_;
            out.add(plungeToLevel);

            // Helical descent from previous level to cut depth
            generateHelixEntry(out, cx, cy, prevLevelZ, cutZ, std::max(helixR, 0.5), feedPlunge_);

        } else if (entryType_ == PocketEntryType::Ramp) {
            // Rapid to safe-Z above ramp start
            ToolpathSegment rapidToRamp;
            rapidToRamp.type = ToolpathSegmentType::Rapid;
            rapidToRamp.start = gp_Pnt(effMinX, effMinY, safeZ_);
            rapidToRamp.end = gp_Pnt(effMinX, effMinY, safeZ_);
            out.add(rapidToRamp);

            // Plunge to previous level
            ToolpathSegment plungeToLevel;
            plungeToLevel.type = ToolpathSegmentType::Plunge;
            plungeToLevel.start = gp_Pnt(effMinX, effMinY, safeZ_);
            plungeToLevel.end = gp_Pnt(effMinX, effMinY, prevLevelZ);
            plungeToLevel.feedRate = feedPlunge_;
            out.add(plungeToLevel);

            // Ramp from previous level to cut depth
            generateRampEntry(out, effMinX, effMinY, prevLevelZ, cutZ,
                              effMaxX, rampAngle_, feedPlunge_);

        } else {
            // Standard plunge entry (PocketEntryType::Plunge)
            // — same as original behavior
        }

        // --- Zigzag raster clearing at cutting depth ---
        bool leftToRight = true;
        bool firstRow = true;

        for (double y = effMinY; y <= effMaxY + 1e-9; y += stepOver_) {
            const double rowY = std::min(y, effMaxY);
            const double sx = leftToRight ? effMinX : effMaxX;
            const double ex = leftToRight ? effMaxX : effMinX;

            // For plunge entry or first row after ramp/helix, rapid to row start
            if (entryType_ == PocketEntryType::Plunge || !firstRow) {
                ToolpathSegment rapidToStart;
                rapidToStart.type = ToolpathSegmentType::Rapid;
                rapidToStart.start = gp_Pnt(sx, rowY, safeZ_);
                rapidToStart.end = gp_Pnt(sx, rowY, safeZ_);
                out.add(rapidToStart);

                ToolpathSegment plunge;
                plunge.type = ToolpathSegmentType::Plunge;
                plunge.start = gp_Pnt(sx, rowY, safeZ_);
                plunge.end = gp_Pnt(sx, rowY, cutZ);
                plunge.feedRate = feedPlunge_;
                out.add(plunge);
            } else if (!out.segments().empty()) {
                // After ramp/helix, move to first row start at cutting depth
                const auto &lastEnd = out.segments().back().end;
                if (lastEnd.Distance(gp_Pnt(sx, rowY, cutZ)) > 1e-9) {
                    ToolpathSegment moveToRow;
                    moveToRow.type = ToolpathSegmentType::Linear;
                    moveToRow.start = lastEnd;
                    moveToRow.end = gp_Pnt(sx, rowY, cutZ);
                    moveToRow.feedRate = feedCut_;
                    out.add(moveToRow);
                }
            }

            // Linear cut across the pocket width
            ToolpathSegment cut;
            cut.type = ToolpathSegmentType::Linear;
            cut.start = gp_Pnt(sx, rowY, cutZ);
            cut.end = gp_Pnt(ex, rowY, cutZ);
            cut.feedRate = feedCut_;
            out.add(cut);

            // Retract only for non-ramp modes or between rows
            if (entryType_ == PocketEntryType::Plunge) {
                ToolpathSegment retract;
                retract.type = ToolpathSegmentType::Rapid;
                retract.start = gp_Pnt(ex, rowY, cutZ);
                retract.end = gp_Pnt(ex, rowY, safeZ_);
                out.add(retract);
            }

            leftToRight = !leftToRight;
            firstRow = false;
        }

        // Final retract to safe-Z after all rows at this depth
        if (entryType_ != PocketEntryType::Plunge && !out.empty()) {
            const auto &lastEnd = out.segments().back().end;
            ToolpathSegment retract;
            retract.type = ToolpathSegmentType::Rapid;
            retract.start = lastEnd;
            retract.end = gp_Pnt(lastEnd.X(), lastEnd.Y(), safeZ_);
            out.add(retract);
        }

        prevLevelZ = cutZ;
    }

    return out;
}

} // namespace MilCAD
