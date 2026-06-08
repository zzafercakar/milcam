/**
 * @file DrillOperation.h
 * @brief Drilling operation with support for standard and peck drill cycles.
 *
 * Generates toolpaths for point-to-point drilling at a list of hole locations,
 * supporting G81 (standard), G82 (dwell), G83 (full-retract peck), and
 * G73 (chip-break peck) canned cycles.
 */

#ifndef MILCAD_DRILL_OPERATION_H
#define MILCAD_DRILL_OPERATION_H

#include "Operation.h"

#include <vector>

namespace MilCAD {

/**
 * @enum DrillCycleType
 * @brief Selects the canned drilling cycle behavior.
 */
enum class DrillCycleType {
    G81, ///< Standard drill cycle: single plunge to depth, then retract
    G82, ///< Dwell drill cycle: plunge to depth, dwell, then retract
    G83, ///< Peck drill cycle: incremental pecks with full retract between each
    G73, ///< Chip-break cycle: incremental pecks with partial retract (1mm chip break)
};

/**
 * @class DrillOperation
 * @brief CAM operation that generates drill-cycle toolpaths for a set of hole positions.
 *
 * For each drill point, the operation generates a rapid move to the retract
 * plane, then either a single plunge (G81/G82) or a series of peck-and-retract
 * cycles (G83/G73) down to the target depth, followed by a final retract.
 */
class DrillOperation : public Operation
{
public:
    std::string typeName() const override { return "Drill"; }

    /**
     * @brief Set the list of hole center positions (XY from points, Z ignored).
     * @param points Vector of 3D points defining hole locations.
     */
    void setPoints(const std::vector<gp_Pnt> &points) { points_ = points; }

    /**
     * @brief Add a single hole center position.
     * @param p The 3D point defining the hole location.
     */
    void addPoint(const gp_Pnt &p) { points_.push_back(p); }

    /// @brief Set the retract plane Z height (safe clearance above workpiece).
    void setRetractZ(double v) { retractZ_ = v; }

    /// @brief Set the target drilling depth Z coordinate.
    void setTargetZ(double v) { targetZ_ = v; }

    /// @brief Set the incremental peck depth for G83/G73 cycles, in mm.
    void setPeckDepth(double v) { peckDepth_ = v; }

    /// @brief Set the plunge feed rate in mm/min.
    void setFeedPlunge(double v) { feedPlunge_ = v; }

    /// @brief Set dwell time at bottom for G82 cycle, in seconds.
    void setDwellSeconds(double v) { dwellSeconds_ = v; }

    /// @brief Set the drill cycle type (G81, G82, G83, or G73).
    void setCycleType(DrillCycleType v) { cycleType_ = v; }

    /**
     * @brief Generate the drill toolpath for all configured hole positions.
     * @return A Toolpath containing rapid positioning, plunge, peck, and retract segments.
     */
    Toolpath generateToolpath() const override;

private:
    std::vector<gp_Pnt> points_; ///< Hole center positions
    double retractZ_ = 5.0;     ///< Retract plane Z height in mm
    double targetZ_ = -5.0;     ///< Target drilling depth Z in mm
    double peckDepth_ = 2.0;    ///< Incremental peck depth for G83/G73 in mm
    double feedPlunge_ = 200.0; ///< Plunge feed rate in mm/min
    double dwellSeconds_ = 0.0; ///< G82 dwell time at hole bottom, in seconds
    DrillCycleType cycleType_ = DrillCycleType::G81; ///< Selected drill cycle
};

} // namespace MilCAD

#endif // MILCAD_DRILL_OPERATION_H
