/**
 * @file EngraveOperation.cpp
 * @brief Implementation of EngraveOperation — edge tracing for engraving.
 *
 * Traces each polyline path at a fixed depth. Paths are ordered using a
 * nearest-neighbor heuristic to minimize rapid traverse distance between
 * disconnected paths.
 */

#include "EngraveOperation.h"
#include "TspSolver.h"

#include <cmath>

namespace MilCAD {

EngraveOperation::EngraveOperation()
{
    setName("Engrave");
}

Toolpath EngraveOperation::generateToolpath() const
{
    Toolpath tp;

    if (paths_.empty()) return tp;

    // Use TSP solver to optimize path ordering (minimize rapid traverse)
    // Build a list of path start points for TSP
    std::vector<gp_Pnt> startPoints;
    startPoints.reserve(paths_.size());
    for (const auto& path : paths_) {
        if (!path.empty()) {
            startPoints.push_back(path.front());
        } else {
            startPoints.emplace_back(0, 0, 0);
        }
    }

    std::vector<int> order = TspSolver::solve(startPoints);

    // Rapid to safe Z
    {
        ToolpathSegment rapid;
        rapid.type = ToolpathSegmentType::Rapid;
        rapid.start = gp_Pnt(0, 0, safeZ_);
        rapid.end = gp_Pnt(0, 0, safeZ_);
        tp.add(rapid);
    }

    // Trace each path in optimized order
    for (int idx : order) {
        if (idx < 0 || idx >= static_cast<int>(paths_.size())) continue;
        const auto& path = paths_[idx];
        if (path.size() < 2) continue;

        // Rapid to start of this path at safe Z
        {
            ToolpathSegment rapid;
            rapid.type = ToolpathSegmentType::Rapid;
            rapid.start = tp.empty() ? gp_Pnt(0, 0, safeZ_) : tp.segments().back().end;
            rapid.end = gp_Pnt(path.front().X(), path.front().Y(), safeZ_);
            tp.add(rapid);
        }

        // Plunge to engraving depth
        {
            ToolpathSegment plunge;
            plunge.type = ToolpathSegmentType::Plunge;
            plunge.start = gp_Pnt(path.front().X(), path.front().Y(), safeZ_);
            plunge.end = gp_Pnt(path.front().X(), path.front().Y(), targetZ_);
            plunge.feedRate = feedPlunge_;
            tp.add(plunge);
        }

        // Trace the path segments at target Z
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            ToolpathSegment seg;
            seg.type = ToolpathSegmentType::Linear;
            seg.start = gp_Pnt(path[i].X(), path[i].Y(), targetZ_);
            seg.end = gp_Pnt(path[i + 1].X(), path[i + 1].Y(), targetZ_);
            seg.feedRate = feedCut_;
            tp.add(seg);
        }

        // Retract to safe Z
        {
            ToolpathSegment retract;
            retract.type = ToolpathSegmentType::Rapid;
            retract.start = gp_Pnt(path.back().X(), path.back().Y(), targetZ_);
            retract.end = gp_Pnt(path.back().X(), path.back().Y(), safeZ_);
            tp.add(retract);
        }
    }

    return tp;
}

} // namespace MilCAD
