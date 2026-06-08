/**
 * @file ToolpathRenderer.h
 * @brief 3D visualization of toolpath segments in the viewport.
 *
 * Renders toolpath segments as colored edges in the scene, with distinct
 * colors for rapid moves (yellow), plunge moves (red), and cutting moves
 * (green). Supports both linear and arc segments.
 */

#ifndef MILCAD_TOOLPATH_RENDERER_H
#define MILCAD_TOOLPATH_RENDERER_H

#include "Toolpath.h"

#include <string>
#include <vector>

#include <Quantity_Color.hxx>

namespace MilCAD {

class SceneManager;

/**
 * @class ToolpathRenderer
 * @brief Renders a Toolpath as colored OCCT edge shapes in the 3D scene.
 *
 * Each segment is rendered as a separate edge shape, colored by segment type.
 * Arc segments attempt true circular arc construction; if the arc fails,
 * they fall back to a straight-line edge.
 */
class ToolpathRenderer
{
public:
    /**
     * @brief Render all segments of a toolpath into the scene.
     * @param toolpath The toolpath to render.
     * @param sceneManager The scene manager to add edge shapes to.
     * @param layerId Base layer/group identifier for segment shapes (default: "toolpath").
     * @return true if at least one segment was successfully rendered.
     */
    bool render(const Toolpath &toolpath, SceneManager *sceneManager,
                const std::string &layerId = "toolpath");

    /**
     * @brief Remove all previously rendered toolpath segments from the scene.
     * @param sceneManager The scene manager to remove shapes from.
     */
    void clear(SceneManager *sceneManager);

    /// @brief Get the number of segments currently rendered in the scene.
    std::size_t segmentCount() const { return segmentIds_.size(); }

    /**
     * @brief Get the display color for a given segment type.
     * @param type The toolpath segment type.
     * @return Yellow for Rapid, red for Plunge, green for Linear/Arc moves.
     */
    static Quantity_Color colorForSegment(ToolpathSegmentType type);

private:
    std::vector<std::string> segmentIds_; ///< Scene manager shape IDs for rendered segments
};

} // namespace MilCAD

#endif // MILCAD_TOOLPATH_RENDERER_H
