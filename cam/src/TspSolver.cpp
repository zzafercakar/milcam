/**
 * @file TspSolver.cpp
 * @brief Implementation of TSP solver using 2-opt heuristic.
 *
 * Ported from FreeCAD's tsp_solver.{h,cpp}. The 2-opt algorithm repeatedly
 * looks for pairs of edges that can be swapped (reversed) to shorten the total
 * tour distance. Convergence is typically fast for typical CAM point counts
 * (< 1000 locations).
 */

#include "TspSolver.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace MilCAD {

// ── Distance helper ──────────────────────────────────────────────────────────

static double dist(const gp_Pnt& a, const gp_Pnt& b)
{
    return a.Distance(b);
}

// ── Tour distance computation ────────────────────────────────────────────────

double TspSolver::tourDistance(const std::vector<gp_Pnt>& points,
                               const std::vector<int>& order)
{
    if (order.size() < 2) return 0.0;

    double total = 0.0;
    for (size_t i = 0; i + 1 < order.size(); ++i) {
        total += dist(points[order[i]], points[order[i + 1]]);
    }
    return total;
}

// ── 2-opt improvement ────────────────────────────────────────────────────────

bool TspSolver::twoOptImprove(const std::vector<gp_Pnt>& points,
                               std::vector<int>& order)
{
    bool improved = false;
    int n = static_cast<int>(order.size());

    for (int i = 0; i < n - 2; ++i) {
        for (int j = i + 2; j < n; ++j) {
            // Check if reversing the segment [i+1..j] improves the tour
            // Current cost: dist(i, i+1) + dist(j, j+1)
            // New cost:     dist(i, j) + dist(i+1, j+1)
            double currentCost = dist(points[order[i]], points[order[i + 1]]);
            double newCost     = dist(points[order[i]], points[order[j]]);

            if (j + 1 < n) {
                currentCost += dist(points[order[j]], points[order[j + 1]]);
                newCost     += dist(points[order[i + 1]], points[order[j + 1]]);
            }

            if (newCost < currentCost - 1e-10) {
                // Reverse the segment [i+1..j]
                std::reverse(order.begin() + i + 1, order.begin() + j + 1);
                improved = true;
            }
        }
    }
    return improved;
}

// ── solve: point-based TSP ───────────────────────────────────────────────────

std::vector<int> TspSolver::solve(const std::vector<gp_Pnt>& points,
                                   int startIdx)
{
    int n = static_cast<int>(points.size());
    if (n == 0) return {};
    if (n == 1) return {0};

    // Initialize with nearest-neighbor heuristic
    std::vector<int> order;
    order.reserve(n);
    std::vector<bool> visited(n, false);

    // Start from startIdx or find the best starting point
    int current = (startIdx >= 0 && startIdx < n) ? startIdx : 0;
    order.push_back(current);
    visited[current] = true;

    for (int step = 1; step < n; ++step) {
        // Find nearest unvisited point
        double bestDist = std::numeric_limits<double>::max();
        int bestIdx = -1;
        for (int j = 0; j < n; ++j) {
            if (visited[j]) continue;
            double d = dist(points[current], points[j]);
            if (d < bestDist) {
                bestDist = d;
                bestIdx = j;
            }
        }
        if (bestIdx < 0) break;

        order.push_back(bestIdx);
        visited[bestIdx] = true;
        current = bestIdx;
    }

    // Apply 2-opt improvement until no more improvements found
    // Limit iterations to prevent infinite loops
    for (int iter = 0; iter < 100; ++iter) {
        if (!twoOptImprove(points, order))
            break;
    }

    return order;
}

// ── solveTunnels: segment-based TSP with flipping ────────────────────────────

TspSolver::TunnelResult TspSolver::solveTunnels(
    const std::vector<gp_Pnt>& entries,
    const std::vector<gp_Pnt>& exits,
    int startIdx)
{
    TunnelResult result;
    int n = static_cast<int>(entries.size());
    if (n == 0 || entries.size() != exits.size()) return result;
    if (n == 1) {
        result.order = {0};
        result.flipped = {false};
        return result;
    }

    // Start with nearest-neighbor for tunnel midpoints
    std::vector<gp_Pnt> midpoints;
    midpoints.reserve(n);
    for (int i = 0; i < n; ++i) {
        double mx = (entries[i].X() + exits[i].X()) * 0.5;
        double my = (entries[i].Y() + exits[i].Y()) * 0.5;
        double mz = (entries[i].Z() + exits[i].Z()) * 0.5;
        midpoints.emplace_back(mx, my, mz);
    }

    // Get initial order using point-based solver on midpoints
    result.order = solve(midpoints, startIdx);
    result.flipped.resize(n, false);

    // Now optimize flipping: for each consecutive pair, decide direction
    for (size_t i = 0; i + 1 < result.order.size(); ++i) {
        int curr = result.order[i];
        int next = result.order[i + 1];

        // Current exit point (depends on whether current segment is flipped)
        gp_Pnt currExit = result.flipped[i] ? entries[curr] : exits[curr];

        // Compare: normal vs flipped entry for next segment
        double distNormal  = dist(currExit, entries[next]);
        double distFlipped = dist(currExit, exits[next]);

        result.flipped[i + 1] = (distFlipped < distNormal);
    }

    return result;
}

} // namespace MilCAD
