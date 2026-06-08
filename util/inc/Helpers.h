/**
 * @file Helpers.h
 * @brief Utility functions for common viewport coordinate conversions and geometry checks.
 *
 * Provides inline helper functions used across the application for:
 *   - Converting 2D mouse positions to 3D world points (with/without grid snapping)
 *   - Approximate point equality comparison with configurable tolerance
 *
 * All functions are inline and header-only for zero-overhead usage.
 */
#ifndef MILCAD_HELPERS_H
#define MILCAD_HELPERS_H

#include <QMouseEvent>
#include <V3d_View.hxx>
#include <gp_Pnt.hxx>

namespace MilCAD {

/**
 * @brief Converts a mouse click position to a 3D world point, snapped to the active grid.
 *
 * Uses V3d_View::ConvertToGrid() to unproject the 2D screen position into 3D
 * space and snap the result to the nearest grid point. The scale factor accounts
 * for high-DPI displays where logical and physical pixels differ.
 *
 * @param event The Qt mouse event containing the click position.
 * @param view  Handle to the OCCT 3D view for coordinate conversion.
 * @param scale Device-pixel-ratio scale factor (default 1.0).
 * @return The 3D world point snapped to the nearest grid intersection.
 */
inline gp_Pnt mouseToGridPoint(const QMouseEvent *event, Handle(V3d_View) view, double scale = 1.0)
{
    // Convert logical Qt coordinates to physical pixel coordinates
    Standard_Integer x = static_cast<Standard_Integer>(event->position().x() * scale);
    Standard_Integer y = static_cast<Standard_Integer>(event->position().y() * scale);

    // Unproject and snap to grid
    Standard_Real X, Y, Z;
    view->ConvertToGrid(x, y, X, Y, Z);
    return gp_Pnt(X, Y, Z);
}

/**
 * @brief Converts a mouse click position to a raw 3D world point without grid snapping.
 *
 * Uses V3d_View::Convert() to unproject the 2D screen position into 3D space.
 * Unlike mouseToGridPoint(), no grid snapping is applied, so the result
 * corresponds to the exact ray-plane intersection.
 *
 * @param event The Qt mouse event containing the click position.
 * @param view  Handle to the OCCT 3D view for coordinate conversion.
 * @param scale Device-pixel-ratio scale factor (default 1.0).
 * @return The raw 3D world point (no grid snap).
 */
inline gp_Pnt mouseToRawPoint(const QMouseEvent *event, Handle(V3d_View) view, double scale = 1.0)
{
    // Convert logical Qt coordinates to physical pixel coordinates
    Standard_Integer x = static_cast<Standard_Integer>(event->position().x() * scale);
    Standard_Integer y = static_cast<Standard_Integer>(event->position().y() * scale);

    // Unproject without grid snapping
    Standard_Real X, Y, Z;
    view->Convert(x, y, X, Y, Z);
    return gp_Pnt(X, Y, Z);
}

/**
 * @brief Checks if two 3D points are approximately equal within a given tolerance.
 *
 * Uses OCCT's gp_Pnt::IsEqual() which compares Euclidean distance.
 *
 * @param a         First point to compare.
 * @param b         Second point to compare.
 * @param tolerance Maximum allowed Euclidean distance for equality (default 1e-3 mm).
 * @return True if the distance between a and b is less than or equal to tolerance.
 */
inline bool pointsEqual(const gp_Pnt &a, const gp_Pnt &b, double tolerance = 1e-3)
{
    return a.IsEqual(b, tolerance);
}

} // namespace MilCAD

#endif // MILCAD_HELPERS_H
