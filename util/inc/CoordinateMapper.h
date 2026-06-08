/**
 * @file CoordinateMapper.h
 * @brief Bidirectional coordinate mapping between 3D world space and 2D sketch planes.
 *
 * Provides conversion utilities for the sketch workbench, where geometry is
 * authored on a 2D plane embedded in 3D space. The mapper maintains an
 * orthonormal frame (origin, U, V, normal) derived from an OCCT gp_Pln or
 * gp_Ax2, and supports:
 *   - 3D-to-2D projection (world point to sketch UV coordinates)
 *   - 2D-to-3D reconstruction (sketch UV coordinates to world point)
 *   - Perpendicular projection onto the sketch plane
 *   - Signed distance from a point to the sketch plane
 */
#ifndef MILCAD_COORDINATE_MAPPER_H
#define MILCAD_COORDINATE_MAPPER_H

#include <gp_Ax2.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>

namespace MilCAD {

/**
 * @class CoordinateMapper
 * @brief Maps between 3D world coordinates and 2D sketch plane coordinates.
 *
 * The sketch plane is defined by an origin point and two orthogonal unit
 * direction vectors (U and V). The normal is the cross product U x V.
 * This class is lightweight, header-only, and safe to copy.
 */
class CoordinateMapper
{
public:
    /**
     * @brief Default constructor. Initializes with the XY plane at the origin.
     */
    CoordinateMapper() { setPlane(gp_Pln()); }

    /**
     * @brief Constructs a mapper from an OCCT plane definition.
     * @param plane The gp_Pln defining origin, U direction (XDirection),
     *              V direction (YDirection), and normal (Axis).
     */
    explicit CoordinateMapper(const gp_Pln &plane) { setPlane(plane); }

    /**
     * @brief Constructs a mapper from an OCCT coordinate system.
     * @param axes The gp_Ax2 defining origin, U (XDirection),
     *             V (YDirection), and normal (Direction).
     */
    explicit CoordinateMapper(const gp_Ax2 &axes) { setAxes(axes); }

    /**
     * @brief Reconfigures the mapper from an OCCT plane.
     *
     * Extracts origin, U direction, V direction, and normal from
     * the plane's Position() coordinate system.
     *
     * @param plane The gp_Pln to extract the coordinate frame from.
     */
    void setPlane(const gp_Pln &plane)
    {
        origin_ = plane.Location();
        uDir_ = gp_Vec(plane.Position().XDirection());
        vDir_ = gp_Vec(plane.Position().YDirection());
        normal_ = gp_Vec(plane.Axis().Direction());
    }

    /**
     * @brief Reconfigures the mapper from an OCCT coordinate system.
     *
     * Extracts origin, U direction (X), V direction (Y), and normal (Z)
     * from the given axes.
     *
     * @param axes The gp_Ax2 to extract the coordinate frame from.
     */
    void setAxes(const gp_Ax2 &axes)
    {
        origin_ = axes.Location();
        uDir_ = gp_Vec(axes.XDirection());
        vDir_ = gp_Vec(axes.YDirection());
        normal_ = gp_Vec(axes.Direction());
    }

    /**
     * @brief Projects a 3D world point onto the sketch plane as 2D coordinates.
     *
     * Computes the dot product of the origin-to-point vector with U and V
     * directions to obtain the local (U, V) coordinates on the plane.
     * Note: the point does not need to lie on the plane; the out-of-plane
     * component is simply discarded.
     *
     * @param worldPt The 3D point in world coordinates.
     * @return The 2D (U, V) coordinates on the sketch plane.
     */
    gp_Pnt2d to2D(const gp_Pnt &worldPt) const
    {
        gp_Vec v(origin_, worldPt);
        return gp_Pnt2d(v.Dot(uDir_), v.Dot(vDir_));
    }

    /**
     * @brief Converts 2D sketch coordinates back to a 3D world point.
     *
     * Reconstructs the 3D position as: origin + U * localPt.X + V * localPt.Y.
     * The resulting point always lies exactly on the sketch plane.
     *
     * @param localPt The 2D (U, V) coordinates on the sketch plane.
     * @return The corresponding 3D world point on the plane.
     */
    gp_Pnt to3D(const gp_Pnt2d &localPt) const
    {
        return gp_Pnt(
            origin_.X() + localPt.X() * uDir_.X() + localPt.Y() * vDir_.X(),
            origin_.Y() + localPt.X() * uDir_.Y() + localPt.Y() * vDir_.Y(),
            origin_.Z() + localPt.X() * uDir_.Z() + localPt.Y() * vDir_.Z());
    }

    /**
     * @brief Converts 2D sketch coordinates (u, v) back to a 3D world point.
     * @param u The U coordinate on the sketch plane.
     * @param v The V coordinate on the sketch plane.
     * @return The corresponding 3D world point on the plane.
     */
    gp_Pnt to3D(double u, double v) const { return to3D(gp_Pnt2d(u, v)); }

    /**
     * @brief Projects a 3D point perpendicularly onto the sketch plane.
     *
     * Computes the nearest point on the plane by subtracting the
     * normal-component of the origin-to-point vector.
     *
     * @param worldPt The 3D point to project.
     * @return The closest point on the sketch plane to worldPt.
     */
    gp_Pnt projectOntoPlane(const gp_Pnt &worldPt) const
    {
        gp_Vec v(origin_, worldPt);
        // Signed distance along the plane normal
        double dist = v.Dot(normal_);
        // Subtract the normal component to project onto the plane
        return gp_Pnt(worldPt.X() - dist * normal_.X(),
                       worldPt.Y() - dist * normal_.Y(),
                       worldPt.Z() - dist * normal_.Z());
    }

    /**
     * @brief Computes the signed distance from a 3D point to the sketch plane.
     *
     * Positive values indicate the point is on the side of the normal;
     * negative values indicate the opposite side.
     *
     * @param worldPt The 3D point to measure.
     * @return The signed perpendicular distance to the plane.
     */
    double distanceToPlane(const gp_Pnt &worldPt) const
    {
        gp_Vec v(origin_, worldPt);
        return v.Dot(normal_);
    }

    /**
     * @brief Returns the origin of the sketch plane.
     * @return Const reference to the origin point.
     */
    const gp_Pnt &origin() const { return origin_; }

    /**
     * @brief Returns the normal direction of the sketch plane.
     * @return The plane normal as a gp_Dir.
     */
    gp_Dir normalDir() const { return gp_Dir(normal_); }

    /**
     * @brief Returns the U (horizontal) direction of the sketch plane.
     * @return The U axis direction as a gp_Dir.
     */
    gp_Dir uDirection() const { return gp_Dir(uDir_); }

    /**
     * @brief Returns the V (vertical) direction of the sketch plane.
     * @return The V axis direction as a gp_Dir.
     */
    gp_Dir vDirection() const { return gp_Dir(vDir_); }

private:
    gp_Pnt origin_;   ///< Origin point of the sketch plane in world coordinates.
    gp_Vec uDir_;     ///< U (horizontal) axis direction vector of the sketch plane.
    gp_Vec vDir_;     ///< V (vertical) axis direction vector of the sketch plane.
    gp_Vec normal_;   ///< Normal direction vector of the sketch plane (U x V).
};

} // namespace MilCAD

#endif // MILCAD_COORDINATE_MAPPER_H
