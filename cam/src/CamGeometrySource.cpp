/**
 * @file CamGeometrySource.cpp
 * @brief Implementation of CAM-CAD geometry source linkage.
 */

#include "CamGeometrySource.h"
#include "SketchDocument.h"

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_QuasiUniformAbscissa.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

namespace MilCAD {

CamGeometrySource::CamGeometrySource(std::shared_ptr<SketchDocument> sketch)
    : sourceType_(SourceType::SketchProfile)
    , sketch_(std::move(sketch))
{
}

CamGeometrySource::CamGeometrySource(std::vector<gp_Pnt> contour)
    : sourceType_(SourceType::ManualContour)
    , manualContour_(std::move(contour))
{
}

std::vector<gp_Pnt> CamGeometrySource::resolveContour() const
{
    if (sourceType_ == SourceType::ManualContour)
        return manualContour_;

    if (sourceType_ == SourceType::SketchProfile && sketch_) {
        // Extract contour points from all sketch entities' edges
        std::vector<gp_Pnt> contour;
        for (const auto &eid : sketch_->entityIds()) {
            auto *ent = sketch_->entity(eid);
            if (!ent || ent->isConstruction())
                continue;

            auto edge = ent->toEdge();
            if (edge.IsNull())
                continue;

            // Sample points along the edge
            try {
                BRepAdaptor_Curve adaptor(edge);
                double f = adaptor.FirstParameter();
                double l = adaptor.LastParameter();
                // Sample 20 points per edge for smooth contour representation
                constexpr int nSamples = 20;
                for (int i = 0; i <= nSamples; ++i) {
                    double t = f + (l - f) * i / nSamples;
                    contour.push_back(adaptor.Value(t));
                }
            } catch (...) {
                continue;
            }
        }
        return contour;
    }

    return {};
}

bool CamGeometrySource::isValid() const
{
    switch (sourceType_) {
    case SourceType::SketchProfile:
        return sketch_ != nullptr && sketch_->entityCount() > 0;
    case SourceType::ManualContour:
        return !manualContour_.empty();
    case SourceType::PartFace:
        return false; // Not yet implemented
    }
    return false;
}

bool CamGeometrySource::isStale() const
{
    if (sourceType_ == SourceType::SketchProfile && sketch_) {
        // Detect change by comparing entity count (simple heuristic)
        return sketch_->entityCount() != lastResolvedEntityCount_;
    }
    return false;
}

void CamGeometrySource::markResolved()
{
    if (sketch_)
        lastResolvedEntityCount_ = sketch_->entityCount();
}

} // namespace MilCAD
