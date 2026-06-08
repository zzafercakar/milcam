#include "CadEngine.h"
#include "CadSession.h"
#include "CadDocument.h"
#include "FacadeError.h"
#include "ScopedTransaction.h"
#include "SketchFacade.h"
#include "PartFacade.h"
#include "CamFacade.h"
#include "NestFacade.h"
#include "OccViewport.h"

#include <cmath>
#include <TopoDS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <App/Document.h>
#include <App/PropertyStandard.h>
#include <Mod/Part/App/PartFeature.h>

namespace CADNC {

// RAII transaction scope: open on construction, commit on destruction. Keeps
// Q_INVOKABLE mutation methods from littering the code with explicit commit
// calls in every early-return branch.
struct CadEngine::TxScope {
    CadEngine* engine;
    TxScope(CadEngine* e, const char* name) : engine(e) {
        if (engine && engine->document_) engine->document_->openTransaction(name);
    }
    ~TxScope() {
        if (engine && engine->document_) {
            engine->document_->commitTransaction();
            // Notify QML so Edit > Undo / Redo actions (and toolbar buttons)
            // pick up the new availability immediately after each operation.
            Q_EMIT engine->undoStateChanged();
        }
    }
    TxScope(const TxScope&) = delete;
    TxScope& operator=(const TxScope&) = delete;
};

CadEngine::CadEngine(QObject* parent)
    : QObject(parent)
    , session_(std::make_unique<CadSession>())
    , cam_(std::make_unique<CamFacade>())
    , nest_(std::make_unique<NestFacade>())
{
}

CadEngine::~CadEngine() = default;

bool CadEngine::init(int argc, char** argv)
{
    bool ok = session_->initialize(argc, argv);
    if (ok) setStatus("Backend initialized");
    else    setStatus("Backend init FAILED");
    return ok;
}

void CadEngine::setViewport(OccViewport* viewport)
{
    viewport_ = viewport;
    if (viewport_) {
        // Bridge viewport face-pick signal to the Q_PROPERTY-facing one that
        // QML Connections consume.
        QObject::connect(viewport_, &OccViewport::facePicked, this,
                         [this](const QString& feat, const QString& sub) {
                             reportFacePicked(feat, sub);
                         });
    }
}

// ── Document ────────────────────────────────────────────────────────

bool CadEngine::newDocument(const QString& name)
{
    // Close existing document if any
    if (document_) {
        activeSketch_.reset();
        document_.reset();
    }

    // Clear 3D viewport — remove all shapes from previous document
    if (viewport_) viewport_->clearShapes();

    document_ = session_->newDocument(name.toStdString());
    documentPath_.clear();
    if (document_) {
        setStatus("New document: " + name);
        Q_EMIT featureTreeChanged();
        Q_EMIT sketchChanged();
        return true;
    }
    return false;
}

bool CadEngine::openDocument(const QString& filePath)
{
    if (filePath.isEmpty()) return false;

    // Close existing
    if (document_) {
        activeSketch_.reset();
        document_.reset();
    }

    auto doc = std::make_shared<CadDocument>("Loading");
    if (doc->load(filePath.toStdString())) {
        document_ = doc;
        documentPath_ = filePath;
        setStatus("Opened: " + filePath);
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
        return true;
    }
    setStatus("Failed to open: " + filePath);
    return false;
}

bool CadEngine::saveDocument()
{
    if (!document_) return false;

    if (documentPath_.isEmpty()) {
        setStatus("No file path — use Save As");
        return false;
    }
    return saveDocumentAs(documentPath_);
}

bool CadEngine::saveDocumentAs(const QString& filePath)
{
    if (!document_ || filePath.isEmpty()) return false;

    bool ok = document_->save(filePath.toStdString());
    if (ok) {
        documentPath_ = filePath;
        setStatus("Saved: " + filePath);
        Q_EMIT featureTreeChanged();
    } else {
        setStatus("Save failed: " + filePath);
    }
    return ok;
}

bool CadEngine::exportDocument(const QString& filePath)
{
    if (!document_ || filePath.isEmpty()) return false;

    bool ok = document_->exportTo(filePath.toStdString());
    if (ok)
        setStatus("Exported: " + filePath);
    else
        setStatus("Export failed: " + filePath);
    return ok;
}

bool CadEngine::importFile(const QString& filePath)
{
    if (filePath.isEmpty()) return false;

    // Auto-create document if none exists
    if (!document_) {
        if (!newDocument("Untitled")) return false;
    }

    bool ok = document_->importFrom(filePath.toStdString());
    if (ok) {
        setStatus("Imported: " + filePath);
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    } else {
        setStatus("Import failed: " + filePath);
    }
    return ok;
}

void CadEngine::closeDocument()
{
    if (activeSketch_) closeSketch();
    document_.reset();
    documentPath_.clear();
    if (viewport_) viewport_->clearShapes();
    setStatus("Document closed");
    Q_EMIT featureTreeChanged();
    Q_EMIT sketchChanged();
}

void CadEngine::undo()
{
    if (document_) {
        document_->undo();
        refreshSketch();
        Q_EMIT featureTreeChanged();
        Q_EMIT undoStateChanged();
        updateViewportShapes();
    }
}

void CadEngine::redo()
{
    if (document_) {
        document_->redo();
        refreshSketch();
        Q_EMIT featureTreeChanged();
        Q_EMIT undoStateChanged();
        updateViewportShapes();
    }
}

bool CadEngine::deleteFeature(const QString& name)
{
    if (!document_) return false;

    // Don't allow deleting the active sketch
    if (activeSketch_ && name.toStdString() == activeSketchName_) {
        setStatus("Cannot delete active sketch — close it first");
        return false;
    }

    TxScope tx(this, "Delete Feature");
    bool ok = document_->deleteFeature(name.toStdString());
    if (ok) {
        setStatus("Deleted: " + name);
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    } else {
        setStatus("Delete failed: " + name);
    }
    return ok;
}

QString CadEngine::duplicateFeature(const QString& name)
{
    if (!document_) return QString();
    TxScope tx(this, "Duplicate Feature");
    std::string newName = document_->duplicateFeature(name.toStdString());
    if (newName.empty()) {
        setStatus("Duplicate failed: " + name);
        return QString();
    }
    setStatus("Duplicated: " + name);
    Q_EMIT featureTreeChanged();
    updateViewportShapes();
    return QString::fromStdString(newName);
}

bool CadEngine::renameFeature(const QString& name, const QString& newLabel)
{
    if (!document_) return false;

    bool ok = document_->renameFeature(name.toStdString(), newLabel.toStdString());
    if (ok) {
        setStatus("Renamed: " + newLabel);
        Q_EMIT featureTreeChanged();
    }
    return ok;
}

bool CadEngine::renameDocument(const QString& newLabel)
{
    if (!document_ || newLabel.isEmpty()) return false;
    // The FreeCAD document's display label goes through its `Label`
    // property; its internal name (returned by getName()) is immutable.
    auto* doc = static_cast<App::Document*>(document_->internalDoc());
    if (!doc) return false;
    doc->Label.setValue(newLabel.toStdString());
    setStatus("Document: " + newLabel);
    Q_EMIT featureTreeChanged();
    return true;
}

bool CadEngine::toggleFeatureVisibility(const QString& name)
{
    if (!document_) return false;
    bool now = document_->toggleVisibility(name.toStdString());
    setStatus(QString("%1: %2").arg(name, now ? "shown" : "hidden"));
    updateViewportShapes();
    Q_EMIT featureTreeChanged();
    return now;
}

bool CadEngine::isFeatureVisible(const QString& name) const
{
    if (!document_) return false;
    return document_->isVisible(name.toStdString());
}

QStringList CadEngine::featureDependents(const QString& name) const
{
    QStringList out;
    if (!document_) return out;
    for (const auto& s : document_->dependents(name.toStdString()))
        out.append(QString::fromStdString(s));
    return out;
}

// ── Sketch lifecycle ────────────────────────────────────────────────

bool CadEngine::createSketch(const QString& name, int planeType)
{
    // Auto-create document if none exists
    if (!document_) {
        if (!newDocument("Untitled")) return false;
    }
    // Close any sketch that's currently being edited. Without this, the
    // old SketchFacade raw pointer keeps referencing the previous
    // SketchObject and every draw call mutates the wrong sketch — which
    // is why "create new sketch → circle/rectangle does nothing" in the
    // user's datum-plane flow: the active sketch was still the first
    // one, unclosed.
    if (activeSketch_) closeSketch();
    activeSketch_ = document_->addSketch(name.toStdString(), planeType);
    if (activeSketch_) {
        static const char* planeNames[] = {"XY", "XZ", "YZ"};
        const char* pn = (planeType >= 0 && planeType <= 2) ? planeNames[planeType] : "XY";
        activeSketchName_ = name.toStdString();
        // Remember plane so re-opening this sketch later can restore the view.
        sketchPlanes_[activeSketchName_] = planeType;
        setStatus(QString("Sketch created: %1 (%2 plane)").arg(name, pn));

        // Keep the existing body shapes visible while sketching — SolidWorks /
        // FreeCAD behaviour. The SketchCanvas is a transparent overlay on top
        // of OccViewport, so the user still sees their solid as reference when
        // starting a new sketch on a datum plane or base plane.
        updateViewportShapes();

        // Orient viewport camera to face the sketch plane
        if (viewport_) {
            switch (planeType) {
                case 0: viewport_->viewTop(); break;     // XY → top view
                case 1: viewport_->viewFront(); break;   // XZ → front view
                case 2: viewport_->viewRight(); break;   // YZ → right view
            }
        }

        Q_EMIT featureTreeChanged();
        Q_EMIT sketchChanged();
        return true;
    }
    return false;
}

bool CadEngine::createSketchOnFace(const QString& name,
                                    const QString& featureName,
                                    const QString& subElement)
{
    if (!document_) {
        if (!newDocument("Untitled")) return false;
    }
    if (activeSketch_) closeSketch();
    activeSketch_ = document_->addSketchOnFace(name.toStdString(),
                                                featureName.toStdString(),
                                                subElement.toStdString());
    if (!activeSketch_) {
        setStatus(QString("Sketch on face failed: %1/%2").arg(featureName, subElement));
        return false;
    }
    activeSketchName_ = name.toStdString();
    // No plane-index for face-attached sketches — leave out of sketchPlanes_
    // so the re-open path falls back to Placement detection.
    setStatus(QString("Sketch on %1/%2").arg(featureName, subElement));

    // Keep the body visible — essential when sketching on a face of the
    // existing solid so the user can see what surface they're working on.
    updateViewportShapes();

    Q_EMIT featureTreeChanged();
    Q_EMIT sketchChanged();
    return true;
}

QString CadEngine::addDatumPlane(int basePlane, double offset, const QString& label)
{
    return addDatumPlaneRotated(basePlane, offset, 0.0, 0.0, label);
}

QString CadEngine::addDatumPlaneRotated(int basePlane, double offset,
                                          double rotXDeg, double rotYDeg,
                                          const QString& label)
{
    if (!document_) {
        if (!newDocument("Untitled")) return {};
    }
    TxScope tx(this, "Add Datum Plane");
    std::string name = document_->addDatumPlaneRotated(basePlane, offset,
                                                        rotXDeg, rotYDeg,
                                                        label.toStdString());
    if (!name.empty()) {
        setStatus(QString("Datum plane added: %1").arg(QString::fromStdString(name)));
        Q_EMIT featureTreeChanged();
    }
    return QString::fromStdString(name);
}

QString CadEngine::addDatumPlaneOnFace(const QString& featureName,
                                         const QString& faceName,
                                         double offset,
                                         const QString& label)
{
    if (!document_) return {};
    TxScope tx(this, "Add Datum Plane (on face)");
    std::string name = document_->addDatumPlaneOnFace(
        featureName.toStdString(), faceName.toStdString(), offset,
        label.toStdString());
    if (!name.empty()) {
        setStatus(QString("Datum plane on %1/%2").arg(featureName, faceName));
        Q_EMIT featureTreeChanged();
    }
    return QString::fromStdString(name);
}

QVariantList CadEngine::availableSketchPlanes() const
{
    // Three base planes are always there; datum planes come from the
    // document tree. Keeping them in a single list lets the sketch-plane
    // picker render one scrollable list instead of two sections.
    QVariantList out;
    auto push = [&out](const QString& name, const QString& label, const QString& type) {
        QVariantMap m;
        m["name"] = name;
        m["label"] = label;
        m["type"] = type;
        out.append(m);
    };
    push("__XY__", "XY Plane", "base");
    push("__XZ__", "XZ Plane", "base");
    push("__YZ__", "YZ Plane", "base");

    if (!document_) return out;
    for (const auto& f : document_->featureTree()) {
        // PartDesign::Plane == datum plane authored by the user. App::Plane
        // under Origin is already covered by the three base entries above.
        if (f.typeName == "PartDesign::Plane") {
            push(QString::fromStdString(f.name),
                 QString::fromStdString(f.label.empty() ? f.name : f.label),
                 "datum");
        }
    }
    return out;
}

bool CadEngine::createSketchOnPlane(const QString& name, const QString& planeFeatureName)
{
    // Auto-create document if none exists — matches createSketch semantics.
    // Without this the Create Sketch overlay on an empty workspace looked
    // inert: the dialog opened, the user clicked XY, but we returned false
    // before ever touching the document factory.
    if (!document_) {
        if (!newDocument("Untitled")) return false;
    }
    if (activeSketch_) closeSketch();

    // Special sentinel values for the three base planes — route those to
    // the simple index-based createSketch so Placement is set via rotation
    // rather than attachment (same behaviour as before datum planes).
    if (planeFeatureName == "__XY__") return createSketch(name, 0);
    if (planeFeatureName == "__XZ__") return createSketch(name, 1);
    if (planeFeatureName == "__YZ__") return createSketch(name, 2);

    activeSketch_ = document_->addSketchOnPlane(name.toStdString(),
                                                 planeFeatureName.toStdString());
    if (!activeSketch_) {
        setStatus(QString("Sketch on plane failed: %1").arg(planeFeatureName));
        return false;
    }
    activeSketchName_ = name.toStdString();
    setStatus(QString("Sketch on %1").arg(planeFeatureName));

    // Mirror the base-plane flow: keep the body visible as reference so the
    // user can see which face/position the sketch sits on.
    updateViewportShapes();

    Q_EMIT featureTreeChanged();
    Q_EMIT sketchChanged();
    return true;
}

QStringList CadEngine::featureFaces(const QString& featureName) const
{
    QStringList names;
    if (!document_ || featureName.isEmpty()) return names;
    for (const auto& s : document_->featureFaceNames(featureName.toStdString()))
        names.append(QString::fromStdString(s));
    return names;
}

QStringList CadEngine::solidFeatureNames() const
{
    QStringList names;
    if (!document_) return names;
    // Ordering matches featureTree (topological). Exclude sketches, bodies,
    // and anything that doesn't host a solid.
    for (const auto& f : document_->featureTree()) {
        if (f.typeName.find("Sketcher::SketchObject") != std::string::npos) continue;
        if (f.typeName.find("Body") != std::string::npos) continue;
        names.append(QString::fromStdString(f.name));
    }
    return names;
}

bool CadEngine::openSketch(const QString& name)
{
    if (!document_) return false;
    activeSketch_ = document_->getSketch(name.toStdString());
    if (activeSketch_) {
        activeSketchName_ = name.toStdString();
        setStatus("Editing sketch: " + name);

        updateViewportShapes();

        // Orient camera to face the sketch plane (UX-004).
        // Prefer the in-memory map (createSketch records it); fall back to
        // placement detection for sketches loaded from disk.
        if (viewport_) {
            int plane = -1;
            auto it = sketchPlanes_.find(activeSketchName_);
            if (it != sketchPlanes_.end()) plane = it->second;
            else                            plane = activeSketch_->planeType();
            switch (plane) {
                case 0: viewport_->viewTop();   break;
                case 1: viewport_->viewFront(); break;
                case 2: viewport_->viewRight(); break;
                default: break;
            }
        }

        Q_EMIT sketchChanged();
        return true;
    }
    return false;
}

void CadEngine::closeSketch()
{
    if (activeSketch_) {
        activeSketch_->close();
        activeSketch_.reset();

        // Recompute so FreeCAD builds the InternalShape (required for Pad/Pocket)
        if (document_) document_->recompute();

        setStatus("Sketch closed");
        Q_EMIT sketchChanged();
        Q_EMIT featureTreeChanged();

        // Show sketch wireframe + any 3D features in the viewport
        updateViewportShapes();
    }
}

// ── Sketch geometry ─────────────────────────────────────────────────

int CadEngine::addLine(double x1, double y1, double x2, double y2)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    try {
        ScopedTransaction tx(document_.get(), "Add Line");
        int id = activeSketch_->addLine({x1, y1}, {x2, y2});
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addCircle(double cx, double cy, double radius)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    try {
        ScopedTransaction tx(document_.get(), "Add Circle");
        int id = activeSketch_->addCircle({cx, cy}, radius);
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addArc(double cx, double cy, double radius,
                      double startAngle, double endAngle)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    const double sa = startAngle * M_PI / 180.0;
    const double ea = endAngle   * M_PI / 180.0;
    try {
        ScopedTransaction tx(document_.get(), "Add Arc");
        int id = activeSketch_->addArc({cx, cy}, radius, sa, ea);
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addArc3Point(double x1, double y1,
                             double x2, double y2,
                             double x3, double y3)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    try {
        ScopedTransaction tx(document_.get(), "Add Arc (3 Points)");
        int id = activeSketch_->addArc3Point({x1, y1}, {x2, y2}, {x3, y3});
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addArcEllipse(double cx, double cy,
                              double majorRadius, double minorRadius,
                              double rotationDeg,
                              double startAngleDeg, double endAngleDeg)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    const double rot = rotationDeg    * M_PI / 180.0;
    const double sa  = startAngleDeg  * M_PI / 180.0;
    const double ea  = endAngleDeg    * M_PI / 180.0;
    try {
        ScopedTransaction tx(document_.get(), "Add Arc (Ellipse)");
        int id = activeSketch_->addArcEllipse({cx, cy}, majorRadius, minorRadius,
                                               rot, sa, ea);
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addArcHyperbola(double cx, double cy,
                                double majorRadius, double minorRadius,
                                double rotationDeg,
                                double startParam, double endParam)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    const double rot = rotationDeg * M_PI / 180.0;
    try {
        ScopedTransaction tx(document_.get(), "Add Arc (Hyperbola)");
        int id = activeSketch_->addArcHyperbola({cx, cy}, majorRadius, minorRadius,
                                                 rot, startParam, endParam);
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addArcParabola(double vx, double vy, double focal,
                               double rotationDeg,
                               double startParam, double endParam)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    const double rot = rotationDeg * M_PI / 180.0;
    try {
        ScopedTransaction tx(document_.get(), "Add Arc (Parabola)");
        int id = activeSketch_->addArcParabola({vx, vy}, focal, rot,
                                                startParam, endParam);
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addCircle3Point(double x1, double y1,
                                double x2, double y2,
                                double x3, double y3)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    try {
        ScopedTransaction tx(document_.get(), "Add Circle (3 Points)");
        int id = activeSketch_->addCircle3Point({x1, y1}, {x2, y2}, {x3, y3});
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addEllipse3Point(double x1, double y1,
                                 double x2, double y2,
                                 double x3, double y3)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    try {
        ScopedTransaction tx(document_.get(), "Add Ellipse (3 Points)");
        int id = activeSketch_->addEllipse3Point({x1, y1}, {x2, y2}, {x3, y3});
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addRectangle(double x1, double y1, double x2, double y2)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    try {
        ScopedTransaction tx(document_.get(), "Add Rectangle");
        int id = activeSketch_->addRectangle({x1, y1}, {x2, y2});
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addPoint(double x, double y)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    try {
        ScopedTransaction tx(document_.get(), "Add Point");
        int id = activeSketch_->addPoint({x, y});
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addEllipse(double cx, double cy, double majorR, double minorR, double angleDeg)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    const double angleRad = angleDeg * M_PI / 180.0;
    try {
        ScopedTransaction tx(document_.get(), "Add Ellipse");
        int id = activeSketch_->addEllipse({cx, cy}, majorR, minorR, angleRad);
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addBSpline(const QVariantList& points, int degree)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    std::vector<CADNC::Point2D> poles;
    poles.reserve(points.size());
    for (const auto& pt : points) {
        auto map = pt.toMap();
        poles.push_back({map["x"].toDouble(), map["y"].toDouble()});
    }
    try {
        ScopedTransaction tx(document_.get(), "Add BSpline");
        int id = activeSketch_->addBSpline(poles, degree);
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

int CadEngine::addPolyline(const QVariantList& points)
{
    if (!activeSketch_) { setLastError(tr("No active sketch")); return -1; }
    std::vector<CADNC::Point2D> pts;
    pts.reserve(points.size());
    for (const auto& pt : points) {
        auto map = pt.toMap();
        pts.push_back({map["x"].toDouble(), map["y"].toDouble()});
    }
    try {
        ScopedTransaction tx(document_.get(), "Add Polyline");
        int id = activeSketch_->addPolyline(pts);
        tx.commit();
        setLastError(QString());
        Q_EMIT undoStateChanged();
        refreshSketch();
        return id;
    } catch (const FacadeError& e) { setLastError(e.userMessage()); return -1; }
}

void CadEngine::setLastError(const QString& message)
{
    if (lastError_ == message) return;
    lastError_ = message;
    if (!message.isEmpty()) Q_EMIT errorOccurred(message);
}

int CadEngine::toggleConstruction(int geoId)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Toggle Construction");
    int result = activeSketch_->toggleConstruction(geoId);
    refreshSketch();
    return result;
}

// ── Sketch constraints ──────────────────────────────────────────────

int CadEngine::addDistanceConstraint(int geoId, double value)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Add Distance");
    int id = activeSketch_->addDistance(geoId, value);
    refreshSketch();
    return id;
}

int CadEngine::addRadiusConstraint(int geoId, double value)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Add Radius");
    int id = activeSketch_->addRadius(geoId, value);
    refreshSketch();
    return id;
}

int CadEngine::addHorizontalConstraint(int geoId)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Horizontal");
    int id = activeSketch_->addHorizontal(geoId);
    refreshSketch();
    return id;
}

int CadEngine::addVerticalConstraint(int geoId)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Vertical");
    int id = activeSketch_->addVertical(geoId);
    refreshSketch();
    return id;
}

int CadEngine::addCoincidentConstraint(int geo1, int pos1, int geo2, int pos2)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Coincident");
    int id = activeSketch_->addCoincident(geo1, pos1, geo2, pos2);
    refreshSketch();
    return id;
}

int CadEngine::addAngleConstraint(int geo1, int geo2, double angleDeg)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Add Angle");
    double angleRad = angleDeg * M_PI / 180.0;
    int id = activeSketch_->addAngle(geo1, geo2, angleRad);
    refreshSketch();
    return id;
}

int CadEngine::addFixedConstraint(int geoId)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Fix");
    int id = activeSketch_->addFixed(geoId);
    refreshSketch();
    return id;
}

int CadEngine::addDistanceXConstraint(int geoId, double value)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Add DistanceX");
    int id = activeSketch_->addConstraint(ConstraintType::DistanceX, geoId, -1, value);
    refreshSketch();
    return id;
}

int CadEngine::addDistanceYConstraint(int geoId, double value)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Add DistanceY");
    int id = activeSketch_->addConstraint(ConstraintType::DistanceY, geoId, -1, value);
    refreshSketch();
    return id;
}

int CadEngine::addDiameterConstraint(int geoId, double value)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Add Diameter");
    int id = activeSketch_->addConstraint(ConstraintType::Diameter, geoId, -1, value);
    refreshSketch();
    return id;
}

int CadEngine::addSymmetricConstraint(int geo1, int pos1, int geo2, int pos2, int symGeo, int symPos)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Symmetric");
    // Delegates to SketchFacade::addSymmetric which sets all three elements +
    // their PointPos values. Needed for "snap vertex to midpoint of a line"
    // (FreeCAD auto-constraint path for midpoint snap).
    int id = activeSketch_->addSymmetric(geo1, pos1, geo2, pos2, symGeo, symPos);
    refreshSketch();
    return id;
}

int CadEngine::addPointOnObjectConstraint(int pointGeo, int pointPos, int objectGeo)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Point on Object");
    // Passing pointPos through is essential — Sketcher::PointOnObject needs
    // (vertex_geo, vertex_pos) on element 0 and (curve_geo, none) on element 1.
    int id = activeSketch_->addPointOnObject(pointGeo, pointPos, objectGeo);
    refreshSketch();
    return id;
}

int CadEngine::addConstraintTwoGeo(const QString& type, int geoId)
{
    if (!activeSketch_) return -1;

    // Two-click workflow: first call stores geo, second call applies constraint
    if (pendingConstraintType_ == type && pendingFirstGeo_ >= 0 && pendingFirstGeo_ != geoId) {
        int id = -1;
        int g1 = pendingFirstGeo_, g2 = geoId;

        if (type == "parallel")
            id = activeSketch_->addConstraint(ConstraintType::Parallel, g1, g2);
        else if (type == "perpendicular")
            id = activeSketch_->addConstraint(ConstraintType::Perpendicular, g1, g2);
        else if (type == "tangent")
            id = activeSketch_->addConstraint(ConstraintType::Tangent, g1, g2);
        else if (type == "equal")
            id = activeSketch_->addConstraint(ConstraintType::Equal, g1, g2);
        else if (type == "coincident")
            id = activeSketch_->addCoincident(g1, 1, g2, 1);  // start-to-start
        else if (type == "symmetric")
            id = activeSketch_->addConstraint(ConstraintType::Symmetric, g1, g2);
        else if (type == "pointOnObject")
            id = activeSketch_->addConstraint(ConstraintType::PointOnObject, g1, g2);

        pendingConstraintType_.clear();
        pendingFirstGeo_ = -1;
        setStatus(QString("%1 constraint applied (G%2, G%3)").arg(type).arg(g1).arg(g2));
        refreshSketch();
        return id;
    }

    // First click — store and wait for second selection
    pendingConstraintType_ = type;
    pendingFirstGeo_ = geoId;
    setStatus(QString("Select second geometry for %1 constraint").arg(type));
    return -1;
}

void CadEngine::removeConstraint(int constraintId)
{
    if (!activeSketch_) return;
    TxScope tx(this, "Delete Constraint");
    activeSketch_->removeConstraint(constraintId);
    refreshSketch();
}

void CadEngine::setDatum(int constraintId, double value)
{
    if (!activeSketch_) return;
    TxScope tx(this, "Change Dimension");
    activeSketch_->setDatum(constraintId, value);
    refreshSketch();
}

void CadEngine::toggleDriving(int constraintId)
{
    if (!activeSketch_) return;
    TxScope tx(this, "Toggle Driving");
    activeSketch_->toggleDriving(constraintId);
    refreshSketch();
}

void CadEngine::removeGeometry(int geoId)
{
    if (!activeSketch_) return;
    TxScope tx(this, "Delete Geometry");
    activeSketch_->removeGeometry(geoId);
    refreshSketch();
}

// ── Sketch tools ────────────────────────────────────────────────────

int CadEngine::trimAtPoint(int geoId, double px, double py)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Trim");
    int id = activeSketch_->trim(geoId, {px, py});
    refreshSketch();
    return id;
}

int CadEngine::filletVertex(int geoId, int posId, double radius)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Fillet");
    int id = activeSketch_->fillet(geoId, posId, radius);
    refreshSketch();
    return id;
}

int CadEngine::chamferVertex(int geoId, int posId, double size)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Chamfer");
    int id = activeSketch_->chamfer(geoId, posId, size);
    refreshSketch();
    return id;
}

int CadEngine::extendGeo(int geoId, double increment, int endPointPos)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Extend");
    int id = activeSketch_->extend(geoId, increment, endPointPos);
    refreshSketch();
    return id;
}

int CadEngine::splitAtPoint(int geoId, double px, double py)
{
    if (!activeSketch_) return -1;
    TxScope tx(this, "Split");
    int id = activeSketch_->split(geoId, {px, py});
    refreshSketch();
    return id;
}

// ── Part features (3D operations) ───────────────────────────────

QString CadEngine::pad(const QString& sketchName, double length)
{
    if (!document_) return {};

    // Close active sketch before creating pad
    if (activeSketch_) closeSketch();

    auto part = document_->partDesign();
    if (!part) return {};

    TxScope tx(this, "Pad");
    std::string result = part->pad(sketchName.toStdString(), length);
    if (!result.empty()) {
        setStatus("Pad created: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::pocket(const QString& sketchName, double depth)
{
    if (!document_) return {};

    if (activeSketch_) closeSketch();

    auto part = document_->partDesign();
    if (!part) return {};

    TxScope tx(this, "Pocket");
    std::string result = part->pocket(sketchName.toStdString(), depth);
    if (!result.empty()) {
        setStatus("Pocket created: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::revolution(const QString& sketchName, double angleDeg)
{
    if (!document_) return {};

    if (activeSketch_) closeSketch();

    auto part = document_->partDesign();
    if (!part) return {};

    TxScope tx(this, "Revolution");
    std::string result = part->revolution(sketchName.toStdString(), angleDeg);
    if (!result.empty()) {
        setStatus("Revolution created: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QVariantMap CadEngine::getFeatureParams(const QString& featureName)
{
    QVariantMap m;
    m["editable"] = false;
    if (!document_ || featureName.isEmpty()) return m;
    auto part = document_->partDesign();
    if (!part) return m;

    auto p = part->getFeatureParams(featureName.toStdString());
    m["typeName"]   = QString::fromStdString(p.typeName);
    m["sketchName"] = QString::fromStdString(p.sketchName);
    m["length"]     = p.length;
    m["length2"]    = p.length2;
    m["angle"]      = p.angle;
    m["reversed"]   = p.reversed;
    m["editable"]   = p.editable;
    m["sideType"]   = QString::fromStdString(p.sideType);
    m["method"]     = QString::fromStdString(p.method);
    return m;
}

namespace {
PadOptions padOptsFromMap(const QVariantMap& m) {
    PadOptions o;
    if (m.contains("length"))   o.length   = m.value("length").toDouble();
    if (m.contains("length2"))  o.length2  = m.value("length2").toDouble();
    if (m.contains("reversed")) o.reversed = m.value("reversed").toBool();
    if (m.contains("sideType")) o.sideType = m.value("sideType").toString().toStdString();
    if (m.contains("method"))   o.method   = m.value("method").toString().toStdString();
    return o;
}
} // namespace

QString CadEngine::padEx(const QString& sketchName, const QVariantMap& opts)
{
    if (!document_) return {};
    if (activeSketch_) closeSketch();
    auto part = document_->partDesign();
    if (!part) return {};
    TxScope tx(this, "Pad");
    std::string result = part->padEx(sketchName.toStdString(), padOptsFromMap(opts));
    if (!result.empty()) {
        setStatus("Pad created: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::pocketEx(const QString& sketchName, const QVariantMap& opts)
{
    if (!document_) return {};
    if (activeSketch_) closeSketch();
    auto part = document_->partDesign();
    if (!part) return {};
    TxScope tx(this, "Pocket");
    std::string result = part->pocketEx(sketchName.toStdString(), padOptsFromMap(opts));
    if (!result.empty()) {
        setStatus("Pocket created: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

bool CadEngine::updatePadEx(const QString& featureName, const QVariantMap& opts)
{
    if (!document_) return false;
    auto part = document_->partDesign();
    if (!part) return false;
    if (activeSketch_) closeSketch();
    TxScope tx(this, "Update Pad");
    bool ok = part->updatePadEx(featureName.toStdString(), padOptsFromMap(opts));
    if (ok) {
        setStatus(QString("Updated Pad: %1").arg(featureName));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return ok;
}

bool CadEngine::updatePocketEx(const QString& featureName, const QVariantMap& opts)
{
    if (!document_) return false;
    auto part = document_->partDesign();
    if (!part) return false;
    if (activeSketch_) closeSketch();
    TxScope tx(this, "Update Pocket");
    bool ok = part->updatePocketEx(featureName.toStdString(), padOptsFromMap(opts));
    if (ok) {
        setStatus(QString("Updated Pocket: %1").arg(featureName));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return ok;
}

bool CadEngine::updatePad(const QString& featureName, double length, bool reversed)
{
    if (!document_) return false;
    auto part = document_->partDesign();
    if (!part) return false;
    // Close active sketch first — its InternalShape may not be current, which
    // would race the recompute triggered inside updatePad.
    if (activeSketch_) closeSketch();
    TxScope tx(this, "Update Pad");
    bool ok = part->updatePad(featureName.toStdString(), length, reversed);
    if (ok) {
        setStatus(QString("Updated: %1 → %2 mm").arg(featureName).arg(length));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return ok;
}

bool CadEngine::updatePocket(const QString& featureName, double depth, bool reversed)
{
    if (!document_) return false;
    auto part = document_->partDesign();
    if (!part) return false;
    if (activeSketch_) closeSketch();
    TxScope tx(this, "Update Pocket");
    bool ok = part->updatePocket(featureName.toStdString(), depth, reversed);
    if (ok) {
        setStatus(QString("Updated: %1 → %2 mm").arg(featureName).arg(depth));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return ok;
}

bool CadEngine::updateRevolution(const QString& featureName, double angleDeg)
{
    if (!document_) return false;
    auto part = document_->partDesign();
    if (!part) return false;
    if (activeSketch_) closeSketch();
    TxScope tx(this, "Update Revolution");
    bool ok = part->updateRevolution(featureName.toStdString(), angleDeg);
    if (ok) {
        setStatus(QString("Updated: %1 → %2°").arg(featureName).arg(angleDeg));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return ok;
}

bool CadEngine::updateGroove(const QString& featureName, double angleDeg)
{
    if (!document_) return false;
    auto part = document_->partDesign();
    if (!part) return false;
    if (activeSketch_) closeSketch();
    TxScope tx(this, "Update Groove");
    bool ok = part->updateGroove(featureName.toStdString(), angleDeg);
    if (ok) {
        setStatus(QString("Updated: %1 → %2°").arg(featureName).arg(angleDeg));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return ok;
}

QString CadEngine::groove(const QString& sketchName, double angleDeg)
{
    if (!document_) return {};
    if (activeSketch_) closeSketch();
    auto part = document_->partDesign();
    if (!part) return {};
    TxScope tx(this, "Groove");
    std::string result = part->groove(sketchName.toStdString(), angleDeg);
    if (!result.empty()) {
        setStatus("Groove created: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::booleanFuse(const QString& baseName, const QString& toolName)
{
    if (!document_) return {};
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->booleanFuse(baseName.toStdString(), toolName.toStdString());
    if (!result.empty()) {
        setStatus("Boolean Fuse: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::booleanCut(const QString& baseName, const QString& toolName)
{
    if (!document_) return {};
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->booleanCut(baseName.toStdString(), toolName.toStdString());
    if (!result.empty()) {
        setStatus("Boolean Cut: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::booleanCommon(const QString& baseName, const QString& toolName)
{
    if (!document_) return {};
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->booleanCommon(baseName.toStdString(), toolName.toStdString());
    if (!result.empty()) {
        setStatus("Boolean Common: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::addBox(double length, double width, double height)
{
    if (!document_) {
        if (!newDocument("Untitled")) return {};
    }
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->addBox(length, width, height);
    if (!result.empty()) {
        setStatus(QString("Box: %1x%2x%3").arg(length).arg(width).arg(height));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::addCylinder(double radius, double height, double angle)
{
    if (!document_) {
        if (!newDocument("Untitled")) return {};
    }
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->addCylinder(radius, height, angle);
    if (!result.empty()) {
        setStatus(QString("Cylinder: R%1 H%2").arg(radius).arg(height));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::addSphere(double radius)
{
    if (!document_) {
        if (!newDocument("Untitled")) return {};
    }
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->addSphere(radius);
    if (!result.empty()) {
        setStatus(QString("Sphere: R%1").arg(radius));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::addCone(double radius1, double radius2, double height)
{
    if (!document_) {
        if (!newDocument("Untitled")) return {};
    }
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->addCone(radius1, radius2, height);
    if (!result.empty()) {
        setStatus(QString("Cone: R1=%1 R2=%2 H=%3").arg(radius1).arg(radius2).arg(height));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::filletAll(const QString& featureName, double radius)
{
    if (!document_) return {};
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->filletAll(featureName.toStdString(), radius);
    if (!result.empty()) {
        setStatus("3D Fillet: R" + QString::number(radius));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::chamferAll(const QString& featureName, double size)
{
    if (!document_) return {};
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->chamferAll(featureName.toStdString(), size);
    if (!result.empty()) {
        setStatus("3D Chamfer: " + QString::number(size));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::linearPattern(const QString& featureName, double length, int occurrences)
{
    if (!document_) return {};
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->linearPattern(featureName.toStdString(), 0, 0, 1, length, occurrences);
    if (!result.empty()) {
        setStatus(QString("Linear Pattern: %1x").arg(occurrences));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::polarPattern(const QString& featureName, double angleDeg, int occurrences)
{
    if (!document_) return {};
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->polarPattern(featureName.toStdString(), 0, 0, 1, angleDeg, occurrences);
    if (!result.empty()) {
        setStatus(QString("Polar Pattern: %1x @ %2°").arg(occurrences).arg(angleDeg));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

QString CadEngine::mirrorFeature(const QString& featureName)
{
    if (!document_) return {};
    auto part = document_->partDesign();
    if (!part) return {};
    std::string result = part->mirror(featureName.toStdString(), 1, 0, 0);
    if (!result.empty()) {
        setStatus("Mirror: " + QString::fromStdString(result));
        Q_EMIT featureTreeChanged();
        updateViewportShapes();
    }
    return QString::fromStdString(result);
}

// ── CAM ────────────────────────────────────────────────────────────

void CadEngine::camSetStock(double length, double width, double height)
{
    cam_->setStock(length, width, height);
    setStatus(QString("Stock set: %1 x %2 x %3 mm").arg(length).arg(width).arg(height));
}

int CadEngine::camAddTool(const QString& name, double diameter, double fluteLength)
{
    int id = cam_->addEndMill(name.toStdString(), diameter, fluteLength);
    setStatus(QString("Tool added: %1 (D%2)").arg(name).arg(diameter));
    return id;
}

int CadEngine::camAddController(int toolId, double rpm, double feedXY, double feedZ)
{
    return cam_->addToolController(toolId, rpm, feedXY, feedZ);
}

int CadEngine::camAddProfile(int controllerId, double depth, double stepDown,
                             double x1, double y1, double x2, double y2)
{
    int id = cam_->addProfileOp(controllerId, depth, stepDown, x1, y1, x2, y2);
    setStatus(QString("Profile operation added (depth: %1mm)").arg(depth));
    return id;
}

int CadEngine::camAddPocket(int controllerId, double depth, double stepDown, double stepOver,
                            double x1, double y1, double x2, double y2)
{
    int id = cam_->addPocketOp(controllerId, depth, stepDown, stepOver, x1, y1, x2, y2);
    setStatus(QString("Pocket operation added (depth: %1mm)").arg(depth));
    return id;
}

int CadEngine::camAddDrill(int controllerId, double depth, const QVariantList& points)
{
    std::vector<std::pair<double,double>> pts;
    for (const auto& pt : points) {
        auto map = pt.toMap();
        pts.emplace_back(map["x"].toDouble(), map["y"].toDouble());
    }
    int id = cam_->addDrillOp(controllerId, depth, pts);
    setStatus(QString("Drill operation added (%1 holes)").arg(pts.size()));
    return id;
}

int CadEngine::camAddFacing(int controllerId, double depth, double stepOver)
{
    int id = cam_->addFacingOp(controllerId, depth, stepOver);
    setStatus(QString("Facing operation added (depth: %1mm)").arg(depth));
    return id;
}

int CadEngine::camOpCount() const { return cam_->operationCount(); }

QString CadEngine::camGenerateGCode() const
{
    return QString::fromStdString(cam_->generateGCode());
}

bool CadEngine::camExportGCode(const QString& filePath, bool codesys) const
{
    return cam_->exportToFile(filePath.toStdString(), codesys);
}

// ── Nesting ────────────────────────────────────────────────────────

bool CadEngine::nestAddPart(const QString& id, double width, double height,
                            int quantity, bool allowRotation)
{
    bool ok = nest_->addPart(id.toStdString(), width, height, quantity, allowRotation);
    if (ok) setStatus(QString("Nest part added: %1 (%2x%3, qty:%4)").arg(id).arg(width).arg(height).arg(quantity));
    return ok;
}

void CadEngine::nestClearParts() { nest_->clearParts(); setStatus("Nest parts cleared"); }

void CadEngine::nestSetSheet(const QString& id, double width, double height)
{
    nest_->setSheet(id.toStdString(), width, height);
    setStatus(QString("Nest sheet: %1 (%2 x %3 mm)").arg(id).arg(width).arg(height));
}

void CadEngine::nestSetPartGap(double gap)  { nest_->setPartGap(gap); }
void CadEngine::nestSetEdgeGap(double gap)  { nest_->setEdgeGap(gap); }
void CadEngine::nestSetRotation(int mode)   { nest_->setRotationMode(mode); }

QVariantMap CadEngine::nestRun(int algorithm)
{
    auto result = nest_->run(algorithm);

    QVariantMap map;
    map["totalPlaced"] = result.totalPlaced;
    map["totalUnplaced"] = result.totalUnplaced;
    map["utilization"] = result.utilization;
    map["sheetsUsed"] = result.sheetsUsed;

    QVariantList placements;
    for (const auto& p : result.placements) {
        QVariantMap pm;
        pm["partId"] = QString::fromStdString(p.partId);
        pm["x"] = p.x; pm["y"] = p.y;
        pm["rotation"] = p.rotation;
        pm["sheetIndex"] = p.sheetIndex;
        placements.append(pm);
    }
    map["placements"] = placements;

    setStatus(QString("Nesting: %1 placed, %2 unplaced, %3% util")
        .arg(result.totalPlaced).arg(result.totalUnplaced)
        .arg(static_cast<int>(result.utilization * 100)));

    return map;
}

// ── Solver ──────────────────────────────────────────────────────────

QString CadEngine::solve()
{
    if (!activeSketch_) return "No sketch";

    auto result = activeSketch_->solve();
    int dof = activeSketch_->dof();
    switch (result) {
        case SolveResult::Solved:
            solverStatus_ = "Fully Constrained";
            break;
        case SolveResult::UnderConstrained:
            solverStatus_ = dof > 0
                ? QString("Under Constrained (%1 DoF)").arg(dof)
                : QString("Under Constrained");
            break;
        case SolveResult::OverConstrained:  solverStatus_ = "Over Constrained"; break;
        case SolveResult::Conflicting:      solverStatus_ = "Conflicting"; break;
        case SolveResult::Redundant:
            // Redundant means the sketch has extra constraints that don't
            // add information. When DoF=0 the geometry is effectively fully
            // constrained (the redundancy is purely informational). When
            // DoF>0 the sketch is still under-constrained and the redundancy
            // is a side-effect of solver ordering — treat like Under so the
            // colour stays blue, not amber. User feedback: seeing orange
            // after every new constraint was confusing because the geometry
            // was clearly not over-determined.
            if (dof == 0)
                solverStatus_ = "Fully Constrained (Redundant)";
            else
                solverStatus_ = QString("Under Constrained (Redundant, %1 DoF)").arg(dof);
            break;
        case SolveResult::SolverError:      solverStatus_ = "Solver Error"; break;
    }
    Q_EMIT sketchChanged();
    return solverStatus_;
}

// ── Property readers ────────────────────────────────────────────────

QVariantList CadEngine::featureTree() const
{
    QVariantList list;
    if (!document_) return list;

    for (const auto& f : document_->featureTree()) {
        QVariantMap item;
        item["name"] = QString::fromStdString(f.name);
        item["label"] = QString::fromStdString(f.label);
        item["typeName"] = QString::fromStdString(f.typeName);
        item["parent"] = QString::fromStdString(f.parent);
        list.append(item);
    }
    return list;
}

QVariantList CadEngine::sketchGeometry() const
{
    QVariantList list;
    if (!activeSketch_) return list;

    for (const auto& g : activeSketch_->geometry()) {
        QVariantMap item;
        item["id"] = g.id;
        item["type"] = QString::fromStdString(g.type);
        item["startX"] = g.start.x;
        item["startY"] = g.start.y;
        item["endX"] = g.end.x;
        item["endY"] = g.end.y;
        item["centerX"] = g.center.x;
        item["centerY"] = g.center.y;
        item["radius"] = g.radius;
        item["startAngle"] = g.startAngle;
        item["endAngle"] = g.endAngle;
        item["construction"] = g.construction;
        item["majorRadius"] = g.majorRadius;
        item["minorRadius"] = g.minorRadius;
        item["angle"] = g.angle;
        item["degree"] = g.degree;

        if (!g.poles.empty()) {
            QVariantList polesList;
            for (const auto& p : g.poles) {
                QVariantMap pm;
                pm["x"] = p.x;
                pm["y"] = p.y;
                polesList.append(pm);
            }
            item["poles"] = polesList;
        }
        list.append(item);
    }
    return list;
}

QVariantList CadEngine::sketchConstraints() const
{
    QVariantList list;
    if (!activeSketch_) return list;

    for (const auto& c : activeSketch_->constraints()) {
        QVariantMap item;
        item["id"] = c.id;
        item["value"] = c.value;
        item["isDriving"] = c.isDriving;
        item["firstGeoId"] = c.firstGeoId;
        item["secondGeoId"] = c.secondGeoId;

        // Type name for display
        switch (c.type) {
            case ConstraintType::Distance:      item["typeName"] = "Distance"; break;
            case ConstraintType::Radius:        item["typeName"] = "Radius"; break;
            case ConstraintType::Horizontal:    item["typeName"] = "Horizontal"; break;
            case ConstraintType::Vertical:      item["typeName"] = "Vertical"; break;
            case ConstraintType::Coincident:    item["typeName"] = "Coincident"; break;
            case ConstraintType::Angle:         item["typeName"] = "Angle"; break;
            case ConstraintType::Perpendicular: item["typeName"] = "Perpendicular"; break;
            case ConstraintType::Parallel:      item["typeName"] = "Parallel"; break;
            case ConstraintType::Tangent:       item["typeName"] = "Tangent"; break;
            case ConstraintType::Equal:         item["typeName"] = "Equal"; break;
            case ConstraintType::Fixed:         item["typeName"] = "Fixed"; break;
            case ConstraintType::DistanceX:     item["typeName"] = "DistanceX"; break;
            case ConstraintType::DistanceY:     item["typeName"] = "DistanceY"; break;
            case ConstraintType::Diameter:      item["typeName"] = "Diameter"; break;
            case ConstraintType::Symmetric:     item["typeName"] = "Symmetric"; break;
            case ConstraintType::PointOnObject: item["typeName"] = "PointOnObject"; break;
            default:                            item["typeName"] = "Other"; break;
        }
        list.append(item);
    }
    return list;
}

QString CadEngine::solverStatus() const { return solverStatus_; }
QString CadEngine::statusMessage() const { return statusMessage_; }
bool CadEngine::sketchActive() const { return activeSketch_ != nullptr; }

bool CadEngine::canUndo() const
{
    return document_ && document_->canUndo();
}

bool CadEngine::canRedo() const
{
    return document_ && document_->canRedo();
}

bool CadEngine::hasDocument() const { return document_ != nullptr; }
QString CadEngine::documentPath() const { return documentPath_; }
bool CadEngine::documentModified() const
{
    return document_ ? document_->isModified() : false;
}
QString CadEngine::documentName() const
{
    if (!document_) return {};
    // Prefer the user-editable Label over the internal name so renames
    // surface in the UI. Fall back to the internal name for fresh docs.
    if (auto* doc = static_cast<App::Document*>(document_->internalDoc())) {
        std::string lbl = doc->Label.getStrValue();
        if (!lbl.empty()) return QString::fromStdString(lbl);
    }
    return QString::fromStdString(document_->name());
}

void CadEngine::setGridSpacing(double mm)
{
    // BUG-009: clamp to 0.5mm minimum so the adaptive extent in OccRenderer
    // (step*100) doesn't collapse to a few millimetres of visible area.
    if (mm < 0.5) mm = 0.5;
    if (mm > 1000.0) mm = 1000.0;
    if (std::abs(mm - gridSpacing_) < 1e-9) return;

    gridSpacing_ = mm;
    // Propagate to the 3D viewport — OccViewport queues the change to the
    // render thread so the OCCT grid updates without tearing.
    if (viewport_) viewport_->setGridStep(mm);
    Q_EMIT gridSpacingChanged();
    setStatus(QString("Grid: %1 mm").arg(mm));
}

void CadEngine::setGridVisible(bool on)
{
    if (gridVisible_ == on) return;
    gridVisible_ = on;
    if (viewport_) viewport_->setGridVisible(on);
    Q_EMIT gridVisibleChanged();
}

void CadEngine::setFacePickMode(bool on)
{
    if (facePickMode_ == on) return;
    facePickMode_ = on;
    // Tell the render thread to resolve the next selection as a face pick.
    if (viewport_) viewport_->setFacePickMode(on);
    Q_EMIT facePickModeChanged();
}

void CadEngine::reportFacePicked(const QString& featureName, const QString& subName)
{
    // One-shot: drop the mode so the next click behaves normally again. The
    // panel flips it back on if it wants to pick another face.
    if (facePickMode_) {
        facePickMode_ = false;
        Q_EMIT facePickModeChanged();
    }
    Q_EMIT facePicked(featureName, subName);
}

QStringList CadEngine::sketchNames() const
{
    QStringList names;
    if (!document_) return names;

    for (const auto& f : document_->featureTree()) {
        // Sketcher::SketchObject type name
        if (f.typeName.find("Sketcher::SketchObject") != std::string::npos) {
            names.append(QString::fromStdString(f.name));
        }
    }
    return names;
}

// ── Internal ────────────────────────────────────────────────────────

void CadEngine::refreshSketch()
{
    if (activeSketch_) {
        solve(); // auto-solve after every change

        // FreeCAD-parity live preview: when the active sketch feeds a Pad/
        // Pocket/etc., the user expects the 3D body to update live as they
        // edit the profile. Without this, deleting a circle from the sketch
        // leaves the 3D Pad showing the old hole. recompute() runs the
        // dependency chain (Sketch → Pad → Body) and updateViewportShapes()
        // pushes the new shapes to the OCCT viewer.
        if (document_) {
            document_->recompute();
            updateViewportShapes();
        }
    }
    Q_EMIT sketchChanged();
}

void CadEngine::setStatus(const QString& msg)
{
    statusMessage_ = msg;
    Q_EMIT statusMessageChanged();
}

void CadEngine::updateViewportShapes()
{
    if (!viewport_ || !document_) return;

    // Clear existing shapes and re-display all features
    viewport_->clearShapes();

    auto features = document_->featureTree();

    // Find the last solid feature (Body Tip) — show only its result shape
    // plus any sketches as wireframe. PartDesign Body chain means the Tip
    // already contains the cumulative solid from all features.
    std::string tipName;
    for (auto it = features.rbegin(); it != features.rend(); ++it) {
        bool isSolid = it->typeName.find("Pad") != std::string::npos
                    || it->typeName.find("Pocket") != std::string::npos
                    || it->typeName.find("Revolution") != std::string::npos
                    || it->typeName.find("Groove") != std::string::npos
                    || it->typeName.find("Fuse") != std::string::npos
                    || it->typeName.find("Cut") != std::string::npos
                    || it->typeName.find("Common") != std::string::npos
                    || it->typeName.find("Box") != std::string::npos
                    || it->typeName.find("Cylinder") != std::string::npos
                    || it->typeName.find("Sphere") != std::string::npos
                    || it->typeName.find("Cone") != std::string::npos
                    || it->typeName.find("Fillet") != std::string::npos
                    || it->typeName.find("Chamfer") != std::string::npos
                    || it->typeName.find("LinearPattern") != std::string::npos
                    || it->typeName.find("PolarPattern") != std::string::npos
                    || it->typeName.find("Mirrored") != std::string::npos
                    || it->typeName.find("Body") != std::string::npos;
        if (isSolid) { tipName = it->name; break; }
    }

    for (const auto& f : features) {
        bool isSketch = f.typeName.find("Sketcher::SketchObject") != std::string::npos;
        bool isBody   = f.typeName.find("Body") != std::string::npos;

        // Skip body object itself (its shape = Tip shape, we show Tip directly)
        if (isBody) continue;

        // For solid features, only show the Tip (last in chain) to avoid
        // overlapping semi-transparent shapes
        bool isSolid = f.typeName.find("Pad") != std::string::npos
                    || f.typeName.find("Pocket") != std::string::npos
                    || f.typeName.find("Revolution") != std::string::npos
                    || f.typeName.find("Groove") != std::string::npos
                    || f.typeName.find("Fuse") != std::string::npos
                    || f.typeName.find("Cut") != std::string::npos
                    || f.typeName.find("Common") != std::string::npos
                    || f.typeName.find("Box") != std::string::npos
                    || f.typeName.find("Cylinder") != std::string::npos
                    || f.typeName.find("Sphere") != std::string::npos
                    || f.typeName.find("Cone") != std::string::npos
                    || f.typeName.find("Fillet") != std::string::npos
                    || f.typeName.find("Chamfer") != std::string::npos
                    || f.typeName.find("LinearPattern") != std::string::npos
                    || f.typeName.find("PolarPattern") != std::string::npos
                    || f.typeName.find("Mirrored") != std::string::npos;
        if (isSolid && f.name != tipName) continue;

        // UX-017: honour the feature's Visibility property. The tree's
        // "Toggle Visibility" menu flips it; we skip invisible features
        // entirely so the viewport reflects the user's intent.
        if (!document_->isVisible(f.name)) continue;

        void* featurePtr = document_->getFeatureShape(f.name);
        if (!featurePtr) continue;

        // getFeatureShape returns Part::Feature* — extract a COPY of the shape
        // so the pointer stays valid after we leave this scope
        auto* partFeature = static_cast<Part::Feature*>(featurePtr);
        TopoDS_Shape shape = partFeature->Shape.getShape().getShape(); // copy
        if (shape.IsNull()) continue;

        // Color: metallic gray for all solids (SolidWorks style), teal for sketch wireframe
        Quantity_Color color(0.72, 0.72, 0.75, Quantity_TOC_sRGB);  // metallic silver-gray
        if (isSketch)
            color = Quantity_Color(0.02, 0.6, 0.4, Quantity_TOC_sRGB);  // teal wireframe

        // Sketches shown as wireframe, solids as shaded
        viewport_->displayShape(f.name, shape, color, isSketch);
    }

    viewport_->fitAll();
}

} // namespace CADNC
