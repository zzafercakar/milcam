#include "SketchFacade.h"
#include "FacadeError.h"

#include <Base/Exception.h>
#include <Base/Vector3D.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeometryFacade.h>

#include <QCoreApplication>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeCircle.hxx>
#include <Geom_Circle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Standard_Failure.hxx>
#include <gp_Pnt.hxx>

#include <memory>
#include <cmath>

// ── Helper macros ───────────────────────────────────────────────────
// Per WRAPPER_CONTRACT § 2.3 — wrap backend exceptions in FacadeError
// at the facade boundary so CadEngine/QML see a single error type.

#define CADNC_SKETCH_FACADE_PRECHECK(method_name)                                 \
    do {                                                                          \
        if (!impl_->sketch) {                                                     \
            throw FacadeError(FacadeError::Code::NoActiveDocument,                \
                QCoreApplication::translate("SketchFacade",                       \
                    method_name " requires an active sketch"));                   \
        }                                                                         \
    } while (0)

#define CADNC_SKETCH_FACADE_TRY(method_name) try {

#define CADNC_SKETCH_FACADE_CATCH(method_name)                                    \
    } catch (const FacadeError&) { throw; }                                       \
    catch (const Base::Exception& e) { throw FacadeError::fromFreeCADException(e); } \
    catch (const Standard_Failure& e) { throw FacadeError::fromOCCTException(e); } \
    catch (const std::exception& e) { throw FacadeError::fromStdException(e); }

namespace CADNC {

struct SketchFacade::Impl {
    Sketcher::SketchObject* sketch = nullptr;
};

SketchFacade::SketchFacade(void* sketchObject)
    : impl_(std::make_unique<Impl>())
{
    impl_->sketch = static_cast<Sketcher::SketchObject*>(sketchObject);
}

SketchFacade::~SketchFacade() = default;

// ── Geometry ────────────────────────────────────────────────────────

int SketchFacade::addLine(Point2D p1, Point2D p2, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addLine");
    CADNC_SKETCH_FACADE_TRY("addLine")
        auto geo = std::make_unique<Part::GeomLineSegment>();
        geo->setPoints(Base::Vector3d(p1.x, p1.y, 0), Base::Vector3d(p2.x, p2.y, 0));
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addLine")
}

int SketchFacade::addCircle(Point2D center, double radius, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addCircle");
    if (radius < 1e-7) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addCircle: radius must be positive"));
    }
    CADNC_SKETCH_FACADE_TRY("addCircle")
        auto geo = std::make_unique<Part::GeomCircle>();
        geo->setCenter(Base::Vector3d(center.x, center.y, 0));
        geo->setRadius(radius);
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addCircle")
}

int SketchFacade::addArc(Point2D center, double radius,
                         double startAngle, double endAngle, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addArc");
    if (radius < 1e-7) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addArc: radius must be positive"));
    }
    CADNC_SKETCH_FACADE_TRY("addArc")
        auto geo = std::make_unique<Part::GeomArcOfCircle>();
        geo->setCenter(Base::Vector3d(center.x, center.y, 0));
        geo->setRadius(radius);
        geo->setRange(startAngle, endAngle, true);
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addArc")
}

int SketchFacade::addArc3Point(Point2D p1, Point2D p2, Point2D p3, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addArc3Point");
    CADNC_SKETCH_FACADE_TRY("addArc3Point")
        // OCCT's GC_MakeArcOfCircle handles the circumcircle math and the
        // arc orientation (p1→p2→p3). IsDone() returns false when the
        // three points are collinear or any two coincide — translate to
        // InvalidArgument so QML/tests see a consistent failure code.
        GC_MakeArcOfCircle maker(gp_Pnt(p1.x, p1.y, 0),
                                  gp_Pnt(p2.x, p2.y, 0),
                                  gp_Pnt(p3.x, p3.y, 0));
        if (!maker.IsDone()) {
            throw FacadeError(FacadeError::Code::InvalidArgument,
                QCoreApplication::translate("SketchFacade",
                    "addArc3Point: collinear or coincident points"));
        }
        auto geo = std::make_unique<Part::GeomArcOfCircle>();
        geo->setHandle(maker.Value());
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addArc3Point")
}

int SketchFacade::addArcEllipse(Point2D center, double majorRadius, double minorRadius,
                                 double rotation, double startAngle, double endAngle,
                                 bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addArcEllipse");
    if (majorRadius < 1e-7 || minorRadius < 1e-7) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addArcEllipse: major and minor radii must be positive"));
    }
    CADNC_SKETCH_FACADE_TRY("addArcEllipse")
        // Part::GeomArcOfEllipse default-constructs a unit ellipse; we
        // set its parameters exactly as addEllipse does, then setRange
        // to trim. OCCT invariant major >= minor is preserved via
        // max/min swap — same normalisation pattern as addEllipse above.
        auto geo = std::make_unique<Part::GeomArcOfEllipse>();
        geo->setCenter(Base::Vector3d(center.x, center.y, 0));
        geo->setMajorRadius(std::max(majorRadius, minorRadius));
        geo->setMinorRadius(std::min(majorRadius, minorRadius));
        if (std::abs(rotation) > 1e-9) {
            geo->setMajorAxisDir(Base::Vector3d(std::cos(rotation), std::sin(rotation), 0));
        }
        geo->setRange(startAngle, endAngle, /*emulateCCWXY=*/true);
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addArcEllipse")
}

int SketchFacade::addArcHyperbola(Point2D center, double majorRadius, double minorRadius,
                                   double rotation, double startAngle, double endAngle,
                                   bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addArcHyperbola");
    if (majorRadius < 1e-7 || minorRadius < 1e-7) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addArcHyperbola: major and minor radii must be positive"));
    }
    CADNC_SKETCH_FACADE_TRY("addArcHyperbola")
        auto geo = std::make_unique<Part::GeomArcOfHyperbola>();
        geo->setCenter(Base::Vector3d(center.x, center.y, 0));
        geo->setMajorRadius(majorRadius);
        geo->setMinorRadius(minorRadius);
        if (std::abs(rotation) > 1e-9) {
            geo->setMajorAxisDir(Base::Vector3d(std::cos(rotation), std::sin(rotation), 0));
        }
        geo->setRange(startAngle, endAngle, /*emulateCCWXY=*/true);
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addArcHyperbola")
}

int SketchFacade::addArcParabola(Point2D vertex, double focal, double rotation,
                                  double startParam, double endParam, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addArcParabola");
    if (focal < 1e-7) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addArcParabola: focal length must be positive"));
    }
    CADNC_SKETCH_FACADE_TRY("addArcParabola")
        // GeomArcOfParabola owns a default Geom_Parabola inside; setFocal
        // adjusts the focal length (distance vertex→focus). Center in
        // OCCT Parabola API refers to the vertex. Axis direction sets
        // the axis of symmetry.
        auto geo = std::make_unique<Part::GeomArcOfParabola>();
        geo->setCenter(Base::Vector3d(vertex.x, vertex.y, 0));
        geo->setFocal(focal);
        // No direct setMajorAxisDir on GeomArcOfParabola — rotation is
        // applied via the underlying Geom_Parabola's Axis. For the scope
        // of this tool we rely on the default axis (+X) and ignore
        // non-zero rotation — matches FreeCAD's DrawSketchHandlerArcOf
        // Parabola which places the axis from the vertex→focus picks.
        // A later sub-phase can extend if parametric rotation is needed.
        (void)rotation;
        geo->setRange(startParam, endParam, /*emulateCCWXY=*/true);
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addArcParabola")
}

int SketchFacade::addCircle3Point(Point2D p1, Point2D p2, Point2D p3, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addCircle3Point");
    CADNC_SKETCH_FACADE_TRY("addCircle3Point")
        GC_MakeCircle maker(gp_Pnt(p1.x, p1.y, 0),
                             gp_Pnt(p2.x, p2.y, 0),
                             gp_Pnt(p3.x, p3.y, 0));
        if (!maker.IsDone()) {
            throw FacadeError(FacadeError::Code::InvalidArgument,
                QCoreApplication::translate("SketchFacade",
                    "addCircle3Point: collinear or coincident points"));
        }
        auto geo = std::make_unique<Part::GeomCircle>();
        geo->setHandle(maker.Value());
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addCircle3Point")
}

int SketchFacade::addEllipse3Point(Point2D p1, Point2D p2, Point2D p3, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addEllipse3Point");
    // Compute axis-aligned ellipse in local frame per the major-axis
    // interpretation. A zero-length major axis (p1≈p2) is un-definable.
    const double mx = p2.x - p1.x;
    const double my = p2.y - p1.y;
    const double majorLen = std::sqrt(mx * mx + my * my);
    if (majorLen < 1e-7) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addEllipse3Point: p1 and p2 must differ (major axis endpoints)"));
    }
    const double majorRad = majorLen / 2.0;
    const double cx = (p1.x + p2.x) / 2.0;
    const double cy = (p1.y + p2.y) / 2.0;
    const double ux = mx / majorLen;
    const double uy = my / majorLen;
    // Rotate p3 into the axis-aligned local frame.
    const double dx = p3.x - cx;
    const double dy = p3.y - cy;
    const double localX = dx * ux + dy * uy;
    const double localY = -dx * uy + dy * ux;
    if (std::abs(localX) >= majorRad - 1e-9) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addEllipse3Point: p3 projection exceeds major-axis bounds"));
    }
    if (std::abs(localY) < 1e-9) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addEllipse3Point: p3 lies on the major axis (minor radius would be 0)"));
    }
    const double ratio = localX / majorRad;
    const double minorRad = std::abs(localY) / std::sqrt(1.0 - ratio * ratio);
    const double rotation = std::atan2(uy, ux);
    // Delegate to addEllipse which already has the major>=minor guarantee
    // via std::max/std::min swap. Call the facade-local method so we get
    // its FacadeError translation and the same OCCT-exception wrapping.
    return addEllipse({cx, cy}, majorRad, minorRad, rotation, construction);
}

int SketchFacade::addRectangle(Point2D p1, Point2D p2, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addRectangle");
    if (std::abs(p1.x - p2.x) < 1e-7 || std::abs(p1.y - p2.y) < 1e-7) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addRectangle: degenerate rectangle (zero area)"));
    }
    CADNC_SKETCH_FACADE_TRY("addRectangle")
        // Rectangle = 4 lines + 4 coincident constraints. addLine now throws
        // on failure — the enclosing transaction (opened by the engine)
        // rolls back any partial geometry on exception unwind.
        int id0 = addLine(p1, {p2.x, p1.y}, construction);
        int id1 = addLine({p2.x, p1.y}, p2, construction);
        int id2 = addLine(p2, {p1.x, p2.y}, construction);
        int id3 = addLine({p1.x, p2.y}, p1, construction);

        // Close corners with coincident constraints
        addCoincident(id0, 2, id1, 1);
        addCoincident(id1, 2, id2, 1);
        addCoincident(id2, 2, id3, 1);
        addCoincident(id3, 2, id0, 1);

        // Alignment constraints (mirrors FreeCAD DrawSketchHandlerRectangle
        // addAlignmentConstraints for Diagonal mode): L0/L2 share Y so are
        // horizontal; L1/L3 share X so are vertical. Without these the solver
        // has no way to preserve right angles when a Distance is applied.
        addHorizontal(id0);
        addHorizontal(id2);
        addVertical(id1);
        addVertical(id3);

        return id0;
    CADNC_SKETCH_FACADE_CATCH("addRectangle")
}

int SketchFacade::addPoint(Point2D p, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addPoint");
    CADNC_SKETCH_FACADE_TRY("addPoint")
        auto geo = std::make_unique<Part::GeomPoint>(Base::Vector3d(p.x, p.y, 0));
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addPoint")
}

int SketchFacade::addEllipse(Point2D center, double majorRadius, double minorRadius,
                              double angle, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addEllipse");
    if (majorRadius < 1e-7 || minorRadius < 1e-7) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addEllipse: major and minor radii must be positive"));
    }
    CADNC_SKETCH_FACADE_TRY("addEllipse")
        auto geo = std::make_unique<Part::GeomEllipse>();
        geo->setCenter(Base::Vector3d(center.x, center.y, 0));
        geo->setMajorRadius(std::max(majorRadius, minorRadius));
        geo->setMinorRadius(std::min(majorRadius, minorRadius));
        if (std::abs(angle) > 1e-9) {
            geo->setMajorAxisDir(Base::Vector3d(std::cos(angle), std::sin(angle), 0));
        }
        return impl_->sketch->addGeometry(geo.release(), construction);
    CADNC_SKETCH_FACADE_CATCH("addEllipse")
}

int SketchFacade::addBSpline(const std::vector<Point2D>& poles, int degree,
                              bool periodic, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addBSpline");
    if (poles.size() < 2) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addBSpline: need at least 2 control points"));
    }
    CADNC_SKETCH_FACADE_TRY("addBSpline")
        std::vector<Base::Vector3d> pts;
        pts.reserve(poles.size());
        for (const auto& p : poles)
            pts.emplace_back(p.x, p.y, 0);

        int nPoles = static_cast<int>(pts.size());
        int deg = std::min(degree, nPoles - 1);

        // Build clamped uniform knot vector
        std::vector<double> knots;
        std::vector<int> mults;
        knots.push_back(0.0);
        mults.push_back(deg + 1);
        for (int i = 1; i < nPoles - deg; ++i) {
            knots.push_back(static_cast<double>(i));
            mults.push_back(1);
        }
        knots.push_back(static_cast<double>(std::max(1, nPoles - deg)));
        mults.push_back(deg + 1);

        std::vector<double> weights(nPoles, 1.0);

        auto geo = std::make_unique<Part::GeomBSplineCurve>(pts, weights, knots, mults, deg, false, periodic);
        int geoId = impl_->sketch->addGeometry(geo.release(), construction);

        // Expose internal geometry (control points, knot points) for solver
        if (geoId >= 0) {
            try { impl_->sketch->exposeInternalGeometry(geoId); } catch (...) {}
        }
        return geoId;
    CADNC_SKETCH_FACADE_CATCH("addBSpline")
}

int SketchFacade::addPolyline(const std::vector<Point2D>& points, bool construction)
{
    CADNC_SKETCH_FACADE_PRECHECK("addPolyline");
    if (points.size() < 2) {
        throw FacadeError(FacadeError::Code::InvalidArgument,
            QCoreApplication::translate("SketchFacade",
                "addPolyline: need at least 2 points"));
    }
    CADNC_SKETCH_FACADE_TRY("addPolyline")
        int firstId = -1;
        int prevId = -1;
        for (size_t i = 0; i + 1 < points.size(); ++i) {
            int id = addLine(points[i], points[i + 1], construction);
            if (i == 0) firstId = id;
            if (prevId >= 0 && id >= 0) {
                addCoincident(prevId, 2, id, 1);
            }
            prevId = id;
        }
        return firstId;
    CADNC_SKETCH_FACADE_CATCH("addPolyline")
}

void SketchFacade::removeGeometry(int geoId)
{
    try { impl_->sketch->delGeometry(geoId); } catch (...) {}
}

// ── Constraints ─────────────────────────────────────────────────────

static Sketcher::ConstraintType toSketcherType(ConstraintType t)
{
    switch (t) {
        case ConstraintType::Coincident:    return Sketcher::Coincident;
        case ConstraintType::Horizontal:    return Sketcher::Horizontal;
        case ConstraintType::Vertical:      return Sketcher::Vertical;
        case ConstraintType::Parallel:      return Sketcher::Parallel;
        case ConstraintType::Perpendicular: return Sketcher::Perpendicular;
        case ConstraintType::Tangent:       return Sketcher::Tangent;
        case ConstraintType::Equal:         return Sketcher::Equal;
        case ConstraintType::Symmetric:     return Sketcher::Symmetric;
        case ConstraintType::Distance:      return Sketcher::Distance;
        case ConstraintType::DistanceX:     return Sketcher::DistanceX;
        case ConstraintType::DistanceY:     return Sketcher::DistanceY;
        case ConstraintType::Angle:         return Sketcher::Angle;
        case ConstraintType::Radius:        return Sketcher::Radius;
        case ConstraintType::Diameter:      return Sketcher::Diameter;
        case ConstraintType::PointOnObject: return Sketcher::PointOnObject;
        case ConstraintType::Fixed:         return Sketcher::Block;
        default:                            return Sketcher::None;
    }
}

int SketchFacade::addConstraint(ConstraintType type, int firstGeo, int secondGeo, double value)
{
    try {
        auto c = std::make_unique<Sketcher::Constraint>();
        c->Type = toSketcherType(type);
        c->setElement(0, Sketcher::GeoElementId(firstGeo, Sketcher::PointPos::none));
        if (secondGeo >= 0) {
            c->setElement(1, Sketcher::GeoElementId(secondGeo, Sketcher::PointPos::none));
        }
        if (value != 0.0) c->setValue(value);
        return impl_->sketch->addConstraint(c.release());
    } catch (...) { return -1; }
}

int SketchFacade::addCoincident(int geo1, int pos1, int geo2, int pos2)
{
    try {
        auto c = std::make_unique<Sketcher::Constraint>();
        c->Type = Sketcher::Coincident;
        c->setElement(0, Sketcher::GeoElementId(geo1, static_cast<Sketcher::PointPos>(pos1)));
        c->setElement(1, Sketcher::GeoElementId(geo2, static_cast<Sketcher::PointPos>(pos2)));
        return impl_->sketch->addConstraint(c.release());
    } catch (...) { return -1; }
}

int SketchFacade::addDistance(int geoId, double distance)
{
    auto c = std::make_unique<Sketcher::Constraint>();
    c->Type = Sketcher::Distance;
    c->setElement(0, Sketcher::GeoElementId(geoId, Sketcher::PointPos::start));
    c->setElement(1, Sketcher::GeoElementId(geoId, Sketcher::PointPos::end));
    c->setValue(distance);
    return impl_->sketch->addConstraint(c.release());
}

int SketchFacade::addRadius(int geoId, double radius)
{
    auto c = std::make_unique<Sketcher::Constraint>();
    c->Type = Sketcher::Radius;
    c->setElement(0, Sketcher::GeoElementId(geoId, Sketcher::PointPos::none));
    c->setValue(radius);
    return impl_->sketch->addConstraint(c.release());
}

int SketchFacade::addAngle(int geo1, int geo2, double angle)
{
    auto c = std::make_unique<Sketcher::Constraint>();
    c->Type = Sketcher::Angle;
    c->setElement(0, Sketcher::GeoElementId(geo1, Sketcher::PointPos::none));
    c->setElement(1, Sketcher::GeoElementId(geo2, Sketcher::PointPos::none));
    c->setValue(angle);
    return impl_->sketch->addConstraint(c.release());
}

int SketchFacade::addHorizontal(int geoId)
{
    auto c = std::make_unique<Sketcher::Constraint>();
    c->Type = Sketcher::Horizontal;
    c->setElement(0, Sketcher::GeoElementId(geoId, Sketcher::PointPos::none));
    return impl_->sketch->addConstraint(c.release());
}

int SketchFacade::addVertical(int geoId)
{
    auto c = std::make_unique<Sketcher::Constraint>();
    c->Type = Sketcher::Vertical;
    c->setElement(0, Sketcher::GeoElementId(geoId, Sketcher::PointPos::none));
    return impl_->sketch->addConstraint(c.release());
}

int SketchFacade::addFixed(int geoId)
{
    auto c = std::make_unique<Sketcher::Constraint>();
    c->Type = Sketcher::Block;
    c->setElement(0, Sketcher::GeoElementId(geoId, Sketcher::PointPos::none));
    return impl_->sketch->addConstraint(c.release());
}

int SketchFacade::addPointOnObject(int pointGeo, int pointPos, int curveGeo)
{
    try {
        auto c = std::make_unique<Sketcher::Constraint>();
        c->Type = Sketcher::PointOnObject;
        c->setElement(0, Sketcher::GeoElementId(pointGeo, static_cast<Sketcher::PointPos>(pointPos)));
        c->setElement(1, Sketcher::GeoElementId(curveGeo, Sketcher::PointPos::none));
        return impl_->sketch->addConstraint(c.release());
    } catch (...) { return -1; }
}

int SketchFacade::addSymmetric(int g1, int pos1, int g2, int pos2, int g3, int pos3)
{
    try {
        auto c = std::make_unique<Sketcher::Constraint>();
        c->Type = Sketcher::Symmetric;
        c->setElement(0, Sketcher::GeoElementId(g1, static_cast<Sketcher::PointPos>(pos1)));
        c->setElement(1, Sketcher::GeoElementId(g2, static_cast<Sketcher::PointPos>(pos2)));
        c->setElement(2, Sketcher::GeoElementId(g3, static_cast<Sketcher::PointPos>(pos3)));
        return impl_->sketch->addConstraint(c.release());
    } catch (...) { return -1; }
}

void SketchFacade::removeConstraint(int constraintId)
{
    try { impl_->sketch->delConstraint(constraintId); } catch (...) {}
}

void SketchFacade::setDatum(int constraintId, double value)
{
    try { impl_->sketch->setDatum(constraintId, value); } catch (...) {}
}

void SketchFacade::toggleDriving(int constraintId)
{
    try { impl_->sketch->toggleDriving(constraintId); } catch (...) {}
}

// ── Sketch tools ────────────────────────────────────────────────────

int SketchFacade::trim(int geoId, Point2D point)
{
    try {
        return impl_->sketch->trim(geoId, Base::Vector3d(point.x, point.y, 0));
    } catch (...) { return -1; }
}

int SketchFacade::fillet(int geoId1, int geoId2, double radius)
{
    // FreeCAD fillet on a vertex identified by geoId + PointPos.
    // BUG-017: `SketchObject::fillet` only adds Tangent + Coincident
    // constraints — no Radius. That leaves the arc radius undimensioned,
    // so Smart Dimension / constraint panel can't edit it later. We
    // attach a Radius datum on the new arc ourselves so the fillet is
    // parametric from the start (matches FreeCAD GUI expectations).
    try {
        int highestBefore = impl_->sketch->getHighestCurveIndex();
        int result = impl_->sketch->fillet(geoId1,
                                           static_cast<Sketcher::PointPos>(geoId2),
                                           radius);
        if (result < 0) return -1;

        // Locate the newly added arc — SketchObject::fillet appends it at
        // the end, so it's the first arc-of-circle above the pre-fillet
        // index. Search top-down to skip any support geometry inserted
        // after it (none currently, but be conservative).
        int newHighest = impl_->sketch->getHighestCurveIndex();
        for (int gid = newHighest; gid > highestBefore; --gid) {
            const Part::Geometry* g = impl_->sketch->getGeometry(gid);
            if (g && g->is<Part::GeomArcOfCircle>()) {
                addRadius(gid, radius);
                return gid;  // QML uses this to pre-select the arc
            }
        }
        return result;
    } catch (...) { return -1; }
}

int SketchFacade::chamfer(int geoId1, int geoId2, double size)
{
    // FreeCAD fillet with chamfer=true inserts a new line segment at the
    // corner. By default no Distance constraint is added, so the UI can't
    // later change the chamfer size via Smart Dimension. We find the new
    // line (highest index after the op) and add a Distance equal to its
    // current length so the dimension flow reaches parity with fillet.
    try {
        int highestBefore = impl_->sketch->getHighestCurveIndex();
        int result = impl_->sketch->fillet(geoId1,
                                           static_cast<Sketcher::PointPos>(geoId2),
                                           size, true, false, true);
        if (result < 0) return -1;

        int newHighest = impl_->sketch->getHighestCurveIndex();
        for (int gid = newHighest; gid > highestBefore; --gid) {
            const Part::Geometry* g = impl_->sketch->getGeometry(gid);
            if (g && g->is<Part::GeomLineSegment>()) {
                // Use actual chord length of the inserted segment (not the
                // requested `size` — the two offsets set distance along
                // each edge, not the resulting hypotenuse).
                auto* seg = static_cast<const Part::GeomLineSegment*>(g);
                Base::Vector3d a = seg->getStartPoint();
                Base::Vector3d b = seg->getEndPoint();
                double len = (b - a).Length();
                addDistance(gid, len);
                return gid;
            }
        }
        return result;
    } catch (...) { return -1; }
}

int SketchFacade::extend(int geoId, double increment, int endPointPos)
{
    try {
        return impl_->sketch->extend(geoId, increment, static_cast<Sketcher::PointPos>(endPointPos));
    } catch (...) { return -1; }
}

int SketchFacade::split(int geoId, Point2D point)
{
    try {
        return impl_->sketch->split(geoId, Base::Vector3d(point.x, point.y, 0));
    } catch (...) { return -1; }
}

int SketchFacade::toggleConstruction(int geoId)
{
    try {
        return impl_->sketch->toggleConstruction(geoId);
    } catch (...) { return -1; }
}

// ── Solver ──────────────────────────────────────────────────────────

SolveResult SketchFacade::solve()
{
    try {
        int result = impl_->sketch->solve();
        // FreeCAD's SketchObject::solve returns 0 whenever the planegcs
        // solver converges — even when the sketch still has remaining
        // degrees of freedom. We need DoF=0 to claim "fully constrained"
        // so the UI can switch from blue (free) to green (locked).
        switch (result) {
            case 0:  {
                int dof = impl_->sketch->getLastDoF();
                return dof == 0 ? SolveResult::Solved
                                : SolveResult::UnderConstrained;
            }
            case -1: return SolveResult::SolverError;
            case -2: return SolveResult::Redundant;
            case -3: return SolveResult::Conflicting;
            case -4: return SolveResult::OverConstrained;
            default: return SolveResult::UnderConstrained;
        }
    } catch (...) {
        return SolveResult::SolverError;
    }
}

int SketchFacade::dof() const
{
    try { return impl_->sketch->getLastDoF(); } catch (...) { return -1; }
}

// ── Query ───────────────────────────────────────────────────────────

std::vector<GeoInfo> SketchFacade::geometry() const
{
    std::vector<GeoInfo> result;
    const auto& geos = impl_->sketch->getInternalGeometry();

    for (int i = 0; i < static_cast<int>(geos.size()); ++i) {
        GeoInfo info;
        info.id = i;

        // Read construction flag via GeometryFacade
        auto geoFacade = impl_->sketch->getGeometryFacade(i);
        if (geoFacade) info.construction = geoFacade->getConstruction();

        if (auto* ls = dynamic_cast<const Part::GeomLineSegment*>(geos[i])) {
            info.type = "Line";
            auto p1 = ls->getStartPoint();
            auto p2 = ls->getEndPoint();
            info.start = {p1.x, p1.y};
            info.end = {p2.x, p2.y};
        }
        else if (auto* c = dynamic_cast<const Part::GeomCircle*>(geos[i])) {
            info.type = "Circle";
            auto ctr = c->getCenter();
            info.center = {ctr.x, ctr.y};
            info.radius = c->getRadius();
        }
        else if (auto* a = dynamic_cast<const Part::GeomArcOfCircle*>(geos[i])) {
            info.type = "Arc";
            auto ctr = a->getCenter();
            info.center = {ctr.x, ctr.y};
            info.radius = a->getRadius();
            double s, e;
            a->getRange(s, e, true);
            info.startAngle = s;
            info.endAngle = e;
        }
        else if (dynamic_cast<const Part::GeomPoint*>(geos[i])) {
            info.type = "Point";
            auto loc = dynamic_cast<const Part::GeomPoint*>(geos[i])->getPoint();
            info.center = {loc.x, loc.y};
        }
        else if (auto* el = dynamic_cast<const Part::GeomEllipse*>(geos[i])) {
            info.type = "Ellipse";
            auto ctr = el->getCenter();
            info.center = {ctr.x, ctr.y};
            info.majorRadius = el->getMajorRadius();
            info.minorRadius = el->getMinorRadius();
            auto dir = el->getMajorAxisDir();
            info.angle = std::atan2(dir.y, dir.x);
        }
        else if (auto* ae = dynamic_cast<const Part::GeomArcOfEllipse*>(geos[i])) {
            info.type = "ArcOfEllipse";
            auto ctr = ae->getCenter();
            info.center = {ctr.x, ctr.y};
            info.majorRadius = ae->getMajorRadius();
            info.minorRadius = ae->getMinorRadius();
            auto dir = ae->getMajorAxisDir();
            info.angle = std::atan2(dir.y, dir.x);
            double s, e;
            ae->getRange(s, e, /*emulateCCWXY=*/true);
            info.startAngle = s;
            info.endAngle = e;
        }
        else if (auto* ah = dynamic_cast<const Part::GeomArcOfHyperbola*>(geos[i])) {
            info.type = "ArcOfHyperbola";
            auto ctr = ah->getCenter();
            info.center = {ctr.x, ctr.y};
            info.majorRadius = ah->getMajorRadius();
            info.minorRadius = ah->getMinorRadius();
            auto dir = ah->getMajorAxisDir();
            info.angle = std::atan2(dir.y, dir.x);
            double s, e;
            ah->getRange(s, e, /*emulateCCWXY=*/true);
            info.startAngle = s;
            info.endAngle = e;
        }
        else if (auto* ap = dynamic_cast<const Part::GeomArcOfParabola*>(geos[i])) {
            info.type = "ArcOfParabola";
            auto ctr = ap->getCenter();
            info.center = {ctr.x, ctr.y};
            // Parabola has a single scalar — focal length — stored in
            // majorRadius per ConicSection convention used above.
            info.majorRadius = ap->getFocal();
            double s, e;
            ap->getRange(s, e, /*emulateCCWXY=*/true);
            info.startAngle = s;
            info.endAngle = e;
        }
        else if (auto* bs = dynamic_cast<const Part::GeomBSplineCurve*>(geos[i])) {
            info.type = "BSpline";
            auto bsPoles = bs->getPoles();
            for (const auto& p : bsPoles)
                info.poles.push_back({p.x, p.y});
            info.degree = bs->getDegree();
        }
        else {
            info.type = "Other";
        }

        result.push_back(std::move(info));
    }
    return result;
}

std::vector<ConstraintInfo> SketchFacade::constraints() const
{
    std::vector<ConstraintInfo> result;
    const auto& constrs = impl_->sketch->Constraints.getValues();

    for (int i = 0; i < static_cast<int>(constrs.size()); ++i) {
        ConstraintInfo info;
        info.id = i;
        info.value = constrs[i]->getValue();
        info.isDriving = constrs[i]->isDriving;
        info.firstGeoId = constrs[i]->getGeoId(0);
        info.secondGeoId = constrs[i]->getGeoId(1);

        // Map type back
        switch (constrs[i]->Type) {
            case Sketcher::Coincident:    info.type = ConstraintType::Coincident; break;
            case Sketcher::Distance:      info.type = ConstraintType::Distance; break;
            case Sketcher::Radius:        info.type = ConstraintType::Radius; break;
            case Sketcher::Horizontal:    info.type = ConstraintType::Horizontal; break;
            case Sketcher::Vertical:      info.type = ConstraintType::Vertical; break;
            case Sketcher::Angle:         info.type = ConstraintType::Angle; break;
            case Sketcher::Perpendicular: info.type = ConstraintType::Perpendicular; break;
            case Sketcher::Parallel:      info.type = ConstraintType::Parallel; break;
            case Sketcher::Tangent:       info.type = ConstraintType::Tangent; break;
            case Sketcher::Equal:         info.type = ConstraintType::Equal; break;
            case Sketcher::Symmetric:     info.type = ConstraintType::Symmetric; break;
            case Sketcher::DistanceX:     info.type = ConstraintType::DistanceX; break;
            case Sketcher::DistanceY:     info.type = ConstraintType::DistanceY; break;
            case Sketcher::Diameter:      info.type = ConstraintType::Diameter; break;
            case Sketcher::PointOnObject: info.type = ConstraintType::PointOnObject; break;
            case Sketcher::Block:         info.type = ConstraintType::Fixed; break;
            default:                      info.type = ConstraintType::Coincident; break;
        }
        result.push_back(std::move(info));
    }
    return result;
}

int SketchFacade::geometryCount() const
{
    return static_cast<int>(impl_->sketch->getInternalGeometry().size());
}

int SketchFacade::constraintCount() const
{
    return static_cast<int>(impl_->sketch->Constraints.getValues().size());
}

void SketchFacade::close()
{
    try { impl_->sketch->solve(); } catch (...) {}
}

int SketchFacade::planeType() const
{
    // Determine the standard plane by transforming both the sketch's
    // local +Z (normal) and local +X through the stored Placement.
    // Using both axes disambiguates rotations that produce identical
    // normals but differ in in-plane orientation.
    if (!impl_->sketch) return -1;
    try {
        Base::Placement pl = impl_->sketch->Placement.getValue();
        Base::Vector3d n(0, 0, 1), xLocal(1, 0, 0);
        pl.getRotation().multVec(n, n);
        pl.getRotation().multVec(xLocal, xLocal);

        // Pick the dominant axis of the normal
        double ax = std::abs(n.x), ay = std::abs(n.y), az = std::abs(n.z);
        if (az >= ax && az >= ay) return 0;                 // XY (normal mostly ±Z)
        // Normal mostly ±Y → XZ unless local X mapped onto Y (then YZ)
        if (ay >= ax && ay >= az) {
            if (std::abs(xLocal.y) > 0.5) return 2;          // YZ (X→Y)
            return 1;                                        // XZ (X stays X)
        }
        // Normal mostly ±X → YZ
        return 2;
    } catch (...) {}
    return -1;
}

} // namespace CADNC
