/**
 * @file ToolpathOptimizer.h
 * @brief Toolpath optimization utilities for reducing rapid traverse distances.
 *
 * Provides algorithms to reorder toolpath segments for more efficient
 * CNC execution, minimizing non-cutting travel time.
 */

#ifndef MILCAD_TOOLPATH_OPTIMIZER_H
#define MILCAD_TOOLPATH_OPTIMIZER_H

#include "Toolpath.h"

namespace MilCAD {

/**
 * @class ToolpathOptimizer
 * @brief Static utility class for toolpath optimization algorithms.
 */
class ToolpathOptimizer
{
public:
    /**
     * @brief Reorder cutting segments using a nearest-neighbor heuristic to minimize rapid travel.
     *
     * Extracts all non-rapid segments from the input, then reorders them so that
     * each subsequent segment starts as close as possible to the previous segment's
     * end point (greedy nearest-neighbor). New rapid segments are inserted between
     * non-adjacent cutting segments.
     *
     * @param input The original toolpath (rapids are discarded and rebuilt).
     * @return An optimized Toolpath with minimized rapid traverse distances.
     *         Returns the original toolpath if it contains no cutting segments.
     */
    static Toolpath optimizeRapidOrder(const Toolpath &input);
};

} // namespace MilCAD

#endif // MILCAD_TOOLPATH_OPTIMIZER_H
