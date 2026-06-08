/**
 * @file StockVisualization.h
 * @brief 3D visualization of the workpiece stock in the scene.
 *
 * Creates a semi-transparent box shape representing the raw material stock
 * and manages its lifecycle within the SceneManager.
 */

#ifndef MILCAD_STOCK_VISUALIZATION_H
#define MILCAD_STOCK_VISUALIZATION_H

#include "Tooling.h"

#include <string>

namespace MilCAD {

class SceneManager;

/**
 * @class StockVisualization
 * @brief Renders the workpiece stock as a translucent 3D box in the viewport.
 *
 * Manages a single OCCT box shape representing the stock material, with
 * automatic cleanup on re-render or explicit clear. The shape is displayed
 * with a blue color and 72% transparency for visual distinction from the
 * machined part.
 */
class StockVisualization
{
public:
    /**
     * @brief Render the stock as a translucent box in the scene.
     * @param stock The stock definition providing origin and dimensions.
     * @param sceneManager The scene manager to add the shape to.
     * @param layerId The layer/group identifier for the stock shape (default: "cam_stock").
     * @return true if rendering succeeded; false if sceneManager is null or stock is invalid.
     */
    bool render(const Stock &stock, SceneManager *sceneManager,
                const std::string &layerId = "cam_stock");

    /**
     * @brief Remove the stock shape from the scene and reset visibility state.
     * @param sceneManager The scene manager to remove the shape from.
     */
    void clear(SceneManager *sceneManager);

    /// @brief Check whether a stock shape is currently displayed.
    bool isVisible() const { return visible_; }

    /// @brief Get the shape identifier used in the scene manager.
    std::string shapeId() const { return shapeId_; }

private:
    std::string shapeId_;  ///< Scene manager shape identifier
    bool visible_ = false; ///< Whether the stock shape is currently rendered
};

} // namespace MilCAD

#endif // MILCAD_STOCK_VISUALIZATION_H
