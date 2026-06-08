/**
 * @file ToolpathRenderer.cpp
 * @brief 3D visualization of toolpath segments in the OCCT scene.
 *
 * Renders each toolpath segment as a colored edge in the SceneManager.
 * Segment types are color-coded: yellow for rapid moves, red for plunges,
 * and green for linear cuts and arcs. Arc segments are rendered using
 * OCCT's GC_MakeArcOfCircle; if arc construction fails, a straight-line
 * fallback is used.
 */

#include "ToolpathRenderer.h"

#include "SceneManager.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeArcOfCircle.hxx>

namespace MilCAD {

/**
 * @brief Return the display color for a given toolpath segment type.
 * @param type  The segment type to color.
 * @return Quantity_Color: yellow for rapids, red for plunges, green for cuts/arcs.
 */
Quantity_Color ToolpathRenderer::colorForSegment(ToolpathSegmentType type)
{
    switch (type) {
    case ToolpathSegmentType::Rapid:
        return Quantity_Color(0.95, 0.78, 0.18, Quantity_TOC_sRGB); // yellow
    case ToolpathSegmentType::Plunge:
        return Quantity_Color(0.86, 0.16, 0.16, Quantity_TOC_sRGB); // red
    case ToolpathSegmentType::CWArc:
    case ToolpathSegmentType::CCWArc:
    case ToolpathSegmentType::Linear:
    default:
        return Quantity_Color(0.18, 0.70, 0.29, Quantity_TOC_sRGB); // green
    }
}

/**
 * @brief Render all toolpath segments as colored edges in the 3D scene.
 * @param toolpath  The toolpath to visualize.
 * @param sceneManager  Pointer to the scene manager for shape registration.
 * @param layerId  Base identifier prefix for segment shapes in the scene.
 * @return true if at least one segment was rendered successfully.
 *
 * Each segment is rendered as an individual edge shape. Arc segments attempt
 * OCCT arc construction; on failure they fall back to a straight edge.
 * Previously rendered segments are cleared before re-rendering.
 */
bool ToolpathRenderer::render(const Toolpath &toolpath, SceneManager *sceneManager,
                              const std::string &layerId)
{
    if (!sceneManager)
        return false;

    clear(sceneManager);

    bool anyRendered = false;
    for (std::size_t i = 0; i < toolpath.segments().size(); ++i) {
        const auto &seg = toolpath.segments()[i];
        TopoDS_Shape shape;

        if (seg.isArc()) {
            GC_MakeArcOfCircle arc(seg.start, seg.center, seg.end);
            if (arc.IsDone()) {
                shape = BRepBuilderAPI_MakeEdge(arc.Value()).Shape();
            } else {
                shape = BRepBuilderAPI_MakeEdge(seg.start, seg.end).Shape();
            }
        } else {
            shape = BRepBuilderAPI_MakeEdge(seg.start, seg.end).Shape();
        }

        const std::string id = layerId + "_" + std::to_string(i);
        if (sceneManager->addShape(id, shape, colorForSegment(seg.type), true)) {
            segmentIds_.push_back(id);
            anyRendered = true;
        }
    }

    return anyRendered;
}

/**
 * @brief Remove all previously rendered toolpath segments from the scene.
 * @param sceneManager  Pointer to the scene manager (may be nullptr for safety).
 */
void ToolpathRenderer::clear(SceneManager *sceneManager)
{
    if (sceneManager) {
        for (const auto &id : segmentIds_)
            sceneManager->removeShape(id);
    }
    segmentIds_.clear();
}

} // namespace MilCAD
