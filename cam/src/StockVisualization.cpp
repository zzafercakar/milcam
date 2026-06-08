/**
 * @file StockVisualization.cpp
 * @brief 3D visualization of rectangular stock material in the OCCT scene.
 *
 * Creates a semi-transparent box shape in the SceneManager to represent the
 * raw stock block. The stock is rendered as a translucent blue solid so that
 * the workpiece geometry and toolpaths remain visible through it.
 */

#include "StockVisualization.h"

#include "SceneManager.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <Quantity_Color.hxx>

namespace MilCAD {

/**
 * @brief Render the stock block as a translucent 3D box in the scene.
 * @param stock  Stock definition (origin, sizeX/Y/Z).
 * @param sceneManager  Pointer to the scene manager for shape registration.
 * @param layerId  Base identifier string for the stock shape in the scene.
 * @return true if the stock was successfully rendered, false on invalid input or failure.
 *
 * Any previously rendered stock is cleared first. The box is assigned a blue
 * color with 72% transparency to allow visibility of underlying geometry.
 */
bool StockVisualization::render(const Stock &stock, SceneManager *sceneManager,
                                const std::string &layerId)
{
    if (!sceneManager)
        return false;

    if (stock.sizeX <= 0.0 || stock.sizeY <= 0.0 || stock.sizeZ <= 0.0)
        return false;

    clear(sceneManager);

    const TopoDS_Shape shape = BRepPrimAPI_MakeBox(stock.origin, stock.sizeX, stock.sizeY, stock.sizeZ).Shape();
    if (shape.IsNull())
        return false;

    shapeId_ = layerId + "_solid";
    const Quantity_Color stockColor(0.27, 0.58, 0.88, Quantity_TOC_sRGB);

    if (!sceneManager->addShape(shapeId_, shape, stockColor, true))
        return false;

    sceneManager->setShapeTransparency(shapeId_, 0.72);
    visible_ = true;
    return true;
}

/**
 * @brief Remove the stock visualization from the scene.
 * @param sceneManager  Pointer to the scene manager (may be nullptr for safety).
 */
void StockVisualization::clear(SceneManager *sceneManager)
{
    if (sceneManager && !shapeId_.empty())
        sceneManager->removeShape(shapeId_);

    shapeId_.clear();
    visible_ = false;
}

} // namespace MilCAD
