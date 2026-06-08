/**
 * @file CamGeometrySource.h
 * @brief Links CAM operations to their source geometry (sketch or part model).
 *
 * Instead of storing raw geometry parameters, operations reference their source
 * through CamGeometrySource. When the source model changes, operations can
 * detect staleness and regenerate their toolpaths.
 */

#ifndef MILCAD_CAM_GEOMETRY_SOURCE_H
#define MILCAD_CAM_GEOMETRY_SOURCE_H

#include <memory>
#include <vector>

#include <gp_Pnt.hxx>
#include <TopoDS_Face.hxx>

namespace MilCAD {

class SketchDocument;
class FeatureManager;
class PersistentSubshapeRef;

/// Source geometry for a CAM operation, linking it back to the CAD model.
class CamGeometrySource
{
public:
    enum class SourceType {
        SketchProfile,  ///< Geometry comes from a sketch profile
        PartFace,       ///< Geometry comes from a part model face
        ManualContour   ///< Geometry is manually defined (backward compat)
    };

    /// Create from sketch profile (extracts contour from sketch entities).
    explicit CamGeometrySource(std::shared_ptr<SketchDocument> sketch);

    /// Create from manual contour (no model linkage — backward compatible).
    explicit CamGeometrySource(std::vector<gp_Pnt> contour);

    /// Default constructor — invalid source.
    CamGeometrySource() = default;

    /// Resolve to a concrete contour for toolpath generation.
    std::vector<gp_Pnt> resolveContour() const;

    /// Check if the source is valid.
    bool isValid() const;

    /// Check if the source model has changed since the last resolve.
    /// For manual contours, this always returns false.
    bool isStale() const;

    /// Mark the current state as resolved (reset staleness).
    void markResolved();

    /// Get the source type.
    SourceType sourceType() const { return sourceType_; }

private:
    SourceType sourceType_ = SourceType::ManualContour;
    std::shared_ptr<SketchDocument> sketch_;
    std::vector<gp_Pnt> manualContour_;
    std::size_t lastResolvedEntityCount_ = 0;
};

} // namespace MilCAD

#endif // MILCAD_CAM_GEOMETRY_SOURCE_H
