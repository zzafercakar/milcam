/**
 * @file ToolpathOptimizer.cpp
 * @brief Toolpath optimization using nearest-neighbor reordering of cutting segments.
 *
 * Reduces total rapid travel distance by reordering the cutting (non-rapid)
 * segments of a toolpath using a greedy nearest-neighbor heuristic. Rapids
 * between reordered cuts are automatically regenerated. This is particularly
 * effective for operations with many disjoint cut segments (e.g. drilling,
 * pocket islands) where the original ordering may cause excessive air travel.
 */

#include "ToolpathOptimizer.h"

#include <limits>
#include <vector>

namespace MilCAD {

/**
 * @brief Reorder cutting segments to minimize rapid travel distance.
 * @param input  The original toolpath with potentially suboptimal segment order.
 * @return A new Toolpath with cutting segments reordered by nearest-neighbor
 *         heuristic and rapid moves inserted between non-adjacent segments.
 *
 * Algorithm: extract all non-rapid segments, then greedily pick the closest
 * unvisited segment start point from the current position. Rapid segments are
 * inserted only when consecutive cuts are not spatially contiguous.
 */
Toolpath ToolpathOptimizer::optimizeRapidOrder(const Toolpath &input)
{
    // Reorder only non-rapid cutting moves with nearest-neighbor from first start point.
    // Rapids are then rebuilt between ordered segments.
    std::vector<ToolpathSegment> cuts;
    for (const auto &s : input.segments()) {
        if (s.type != ToolpathSegmentType::Rapid)
            cuts.push_back(s);
    }

    if (cuts.empty())
        return input;

    std::vector<bool> used(cuts.size(), false);
    Toolpath out;

    gp_Pnt current = cuts.front().start;

    for (std::size_t i = 0; i < cuts.size(); ++i) {
        double bestDist = std::numeric_limits<double>::max();
        std::size_t bestIdx = 0;

        for (std::size_t j = 0; j < cuts.size(); ++j) {
            if (used[j])
                continue;
            const double d = current.Distance(cuts[j].start);
            if (d < bestDist) {
                bestDist = d;
                bestIdx = j;
            }
        }

        used[bestIdx] = true;

        if (current.Distance(cuts[bestIdx].start) > 1e-6) {
            ToolpathSegment rapid;
            rapid.type = ToolpathSegmentType::Rapid;
            rapid.start = current;
            rapid.end = cuts[bestIdx].start;
            out.add(rapid);
        }

        out.add(cuts[bestIdx]);
        current = cuts[bestIdx].end;
    }

    return out;
}

} // namespace MilCAD
