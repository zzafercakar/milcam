/**
 * @file EngraveOperation.h
 * @brief Engrave operation — traces edges at a fixed depth for engraving/marking.
 *
 * Ported from FreeCAD's CAM Engrave operation. Traces a set of edges
 * (polyline segments) at a fixed Z depth for surface engraving, marking,
 * or V-carving applications.
 */

#ifndef MILCAD_ENGRAVE_OPERATION_H
#define MILCAD_ENGRAVE_OPERATION_H

#include "Operation.h"

#include <gp_Pnt.hxx>
#include <vector>

namespace MilCAD {

/**
 * @class EngraveOperation
 * @brief Generates toolpaths for edge-based engraving.
 *
 * Takes a set of polyline paths (each is a sequence of points) and
 * traces each at a fixed depth. Uses TSP ordering to minimize rapid
 * traverse between disconnected paths.
 */
class EngraveOperation : public Operation
{
public:
    EngraveOperation();

    std::string typeName() const override { return "Engrave"; }
    Toolpath generateToolpath() const override;

    /// Add a polyline path to engrave
    void addPath(const std::vector<gp_Pnt>& path) { paths_.push_back(path); }

    /// Set all paths at once
    void setPaths(const std::vector<std::vector<gp_Pnt>>& paths) { paths_ = paths; }
    const std::vector<std::vector<gp_Pnt>>& paths() const { return paths_; }

    void setTargetZ(double z) { targetZ_ = z; }
    double targetZ() const { return targetZ_; }

    void setSafeZ(double z) { safeZ_ = z; }
    double safeZ() const { return safeZ_; }

    void setFeedCut(double f) { feedCut_ = f; }
    double feedCut() const { return feedCut_; }

    void setFeedPlunge(double f) { feedPlunge_ = f; }
    double feedPlunge() const { return feedPlunge_; }

private:
    std::vector<std::vector<gp_Pnt>> paths_;
    double targetZ_ = -0.5;    // Shallow depth for engraving
    double safeZ_ = 5.0;
    double feedCut_ = 500.0;    // Slow feed for engraving quality
    double feedPlunge_ = 100.0;
};

} // namespace MilCAD

#endif // MILCAD_ENGRAVE_OPERATION_H
