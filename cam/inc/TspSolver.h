/**
 * @file TspSolver.h
 * @brief Traveling Salesman Problem solver for toolpath sequencing.
 *
 * Ported from FreeCAD's CAM/App/tsp_solver.{h,cpp}. Uses 2-opt heuristic
 * to find near-optimal ordering of cut locations to minimize total rapid
 * traverse distance.
 */

#ifndef MILCAD_TSP_SOLVER_H
#define MILCAD_TSP_SOLVER_H

#include <gp_Pnt.hxx>
#include <vector>

namespace MilCAD {

/**
 * @class TspSolver
 * @brief 2-opt heuristic TSP solver for toolpath point/segment ordering.
 *
 * Given a set of points or segments (tunnels), finds a near-optimal ordering
 * that minimizes total travel distance. Supports optional start/end point
 * constraints and segment flipping for bidirectional cuts.
 */
class TspSolver
{
public:
    /**
     * @brief Solve TSP for a set of points.
     *
     * Returns the indices into `points` in the optimized order.
     * Uses 2-opt improvement heuristic.
     *
     * @param points    The locations to visit.
     * @param startIdx  Index of the starting point (-1 for free start).
     * @return Optimized visit order as indices into `points`.
     */
    static std::vector<int> solve(const std::vector<gp_Pnt>& points,
                                  int startIdx = -1);

    /**
     * @brief Solve TSP for segments (tunnels) that can be flipped.
     *
     * Each tunnel has an entry and exit point. The solver determines both
     * the ordering and the orientation (normal or flipped) that minimizes
     * total travel distance between consecutive tunnel endpoints.
     *
     * @param entries   Entry point of each segment.
     * @param exits     Exit point of each segment (same size as entries).
     * @param startIdx  Index of the starting segment (-1 for free start).
     * @return Pair of (optimized order indices, flipped flags per index).
     */
    struct TunnelResult {
        std::vector<int> order;     ///< Optimized indices
        std::vector<bool> flipped;  ///< True if segment should be traversed in reverse
    };

    static TunnelResult solveTunnels(const std::vector<gp_Pnt>& entries,
                                     const std::vector<gp_Pnt>& exits,
                                     int startIdx = -1);

private:
    /// Compute total tour distance for a given order of points.
    static double tourDistance(const std::vector<gp_Pnt>& points,
                              const std::vector<int>& order);

    /// Attempt 2-opt improvement swaps. Returns true if any improvement was found.
    static bool twoOptImprove(const std::vector<gp_Pnt>& points,
                              std::vector<int>& order);
};

} // namespace MilCAD

#endif // MILCAD_TSP_SOLVER_H
