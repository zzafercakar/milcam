#pragma once

/**
 * @file CadEngine.h
 * @brief QML-facing bridge that exposes CADNC adapter to the UI.
 *
 * CadEngine is a QObject registered with QML. It owns the CadSession and
 * active CadDocument, and exposes sketch/part operations as Q_INVOKABLE
 * methods. QML never touches FreeCAD types — only this bridge.
 */

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QStringList>
#include <memory>
#include <unordered_map>

namespace CADNC {

class CadSession;
class CadDocument;
class SketchFacade;
class OccViewport;
class CamFacade;
class NestFacade;

class CadEngine : public QObject {
    Q_OBJECT

    /// Feature tree as a QVariantList for QML ListView
    Q_PROPERTY(QVariantList featureTree READ featureTree NOTIFY featureTreeChanged)

    /// Geometry list of active sketch as QVariantList
    Q_PROPERTY(QVariantList sketchGeometry READ sketchGeometry NOTIFY sketchChanged)

    /// Constraints of active sketch
    Q_PROPERTY(QVariantList sketchConstraints READ sketchConstraints NOTIFY sketchChanged)

    /// Solver status text
    Q_PROPERTY(QString solverStatus READ solverStatus NOTIFY sketchChanged)

    /// Status bar message
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

    /// Whether a sketch is currently active
    Q_PROPERTY(bool sketchActive READ sketchActive NOTIFY sketchChanged)

    /// Undo/redo availability. NOTIFY needs to fire after sketch mutations too
    /// (otherwise the Edit menu shortcut stays disabled after drawing), so we
    /// emit a dedicated undoStateChanged on every TxScope commit.
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoStateChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoStateChanged)

    /// List of sketch names for feature dialog dropdowns
    Q_PROPERTY(QStringList sketchNames READ sketchNames NOTIFY featureTreeChanged)

    /// Whether a document is loaded
    Q_PROPERTY(bool hasDocument READ hasDocument NOTIFY featureTreeChanged)

    /// Current document file path (empty if not saved)
    Q_PROPERTY(QString documentPath READ documentPath NOTIFY featureTreeChanged)

    /// True when the document has unsaved changes. Bound to FreeCAD's
    /// isTouched() — used by the QML close/new flow to prompt the user.
    Q_PROPERTY(bool documentModified READ documentModified NOTIFY featureTreeChanged)

    /// Current document name
    Q_PROPERTY(QString documentName READ documentName NOTIFY featureTreeChanged)

    /// Grid spacing in mm — single source of truth for the 2D Sketch canvas
    /// and the OCCT 3D viewport grid. Editable from status bar.
    Q_PROPERTY(double gridSpacing READ gridSpacing WRITE setGridSpacing NOTIFY gridSpacingChanged)

    /// Grid visibility — single source for both the 2D SketchCanvas overlay
    /// and the OCCT 3D viewport grid. Fixes BUG-013 where toggling the sketch
    /// canvas grid left the OCCT grid visible underneath.
    Q_PROPERTY(bool gridVisible READ gridVisible WRITE setGridVisible NOTIFY gridVisibleChanged)

    /// When true, the next viewport left-click is interpreted as a face pick
    /// and emits facePicked(featureName, subName) instead of the usual
    /// geometry selection. DatumPlanePanel flips this on demand.
    Q_PROPERTY(bool facePickMode READ facePickMode WRITE setFacePickMode NOTIFY facePickModeChanged)

    /// Last user-visible error message produced by a facade call. Cleared
    /// to empty on the next successful mutation. Surfaced as a transient
    /// toast in QML via errorOccurred; read here for status-bar display
    /// or headless diagnostics.
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)

public:
    explicit CadEngine(QObject* parent = nullptr);
    ~CadEngine() override;

    /// Initialize backend (call once from main before QML loads)
    Q_INVOKABLE bool init(int argc, char** argv);

    /// Set the 3D viewport for shape display (call from main after QML loads)
    void setViewport(OccViewport* viewport);

    // ── Document ────────────────────────────────────────────────────
    Q_INVOKABLE bool newDocument(const QString& name = "Untitled");
    Q_INVOKABLE bool openDocument(const QString& filePath);
    Q_INVOKABLE bool saveDocument();
    Q_INVOKABLE bool saveDocumentAs(const QString& filePath);
    Q_INVOKABLE bool exportDocument(const QString& filePath);
    Q_INVOKABLE bool importFile(const QString& filePath);
    Q_INVOKABLE void closeDocument();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    /// Delete a feature by internal name. Returns true on success.
    Q_INVOKABLE bool deleteFeature(const QString& name);
    /// Rename a feature's label. Returns true on success.
    Q_INVOKABLE bool renameFeature(const QString& name, const QString& newLabel);
    /// Deep-clone a feature. Returns the new feature's internal name, or
    /// empty string on failure. Forwarded from CadDocument::duplicateFeature.
    Q_INVOKABLE QString duplicateFeature(const QString& name);
    /// Rename the document itself (label). Different from renameFeature,
    /// which targets a DocumentObject inside the current doc.
    Q_INVOKABLE bool renameDocument(const QString& newLabel);
    /// UX-017: toggle a feature's `Visibility` property and refresh the
    /// viewport. Returns the new visibility state.
    Q_INVOKABLE bool toggleFeatureVisibility(const QString& name);
    Q_INVOKABLE bool isFeatureVisible(const QString& name) const;
    /// UX-017: list features that consume the given feature (used to warn
    /// before deletion and to show a "Dependencies" submenu).
    Q_INVOKABLE QStringList featureDependents(const QString& name) const;

    // ── Sketch lifecycle ────────────────────────────────────────────
    /// Create a new sketch. planeType: 0=XY, 1=XZ, 2=YZ
    Q_INVOKABLE bool createSketch(const QString& name = "Sketch", int planeType = 0);
    /// Create a sketch attached to an existing face. subElement is a FreeCAD
    /// sub-name like "Face1" (1-based, matching TopoDS_Shape::TopoDS_Iterator
    /// order). Uses Attacher::mmFlatFace so the sketch plane is derived from
    /// the face geometry on recompute.
    Q_INVOKABLE bool createSketchOnFace(const QString& name,
                                         const QString& featureName,
                                         const QString& subElement);
    /// Enumerate editable faces of a feature — used by the sketch-plane
    /// picker UI when "On Face" is chosen. Returns list of "Face1", "Face2"...
    Q_INVOKABLE QStringList featureFaces(const QString& featureName) const;
    /// Create a new datum plane offset from one of the base planes.
    /// basePlane: 0=XY, 1=XZ, 2=YZ. Returns the plane's internal name.
    Q_INVOKABLE QString addDatumPlane(int basePlane, double offset,
                                       const QString& label = "DatumPlane");
    /// Datum plane with tilt angles relative to the base plane. rotXDeg
    /// / rotYDeg are applied to the local X/Y axes before translation.
    Q_INVOKABLE QString addDatumPlaneRotated(int basePlane, double offset,
                                              double rotXDeg, double rotYDeg,
                                              const QString& label = "DatumPlane");
    /// Datum plane attached to a face of an existing solid feature.
    Q_INVOKABLE QString addDatumPlaneOnFace(const QString& featureName,
                                             const QString& faceName,
                                             double offset,
                                             const QString& label = "DatumPlane");
    /// List datum / base plane feature names so the sketch-plane picker
    /// can offer them alongside XY/XZ/YZ. Each entry is a QVariantMap
    /// with `name` (internal) + `label` (display) + `type`
    /// ("base" | "datum").
    Q_INVOKABLE QVariantList availableSketchPlanes() const;
    /// Start a sketch on a named plane (base or custom datum). Used by
    /// the sketch-plane picker when the user chose a datum from the
    /// list. Returns true on success.
    Q_INVOKABLE bool createSketchOnPlane(const QString& name,
                                          const QString& planeFeatureName);
    Q_INVOKABLE bool openSketch(const QString& name);
    Q_INVOKABLE void closeSketch();

    // ── Sketch geometry ─────────────────────────────────────────────
    Q_INVOKABLE int addLine(double x1, double y1, double x2, double y2);
    Q_INVOKABLE int addCircle(double cx, double cy, double radius);
    Q_INVOKABLE int addArc(double cx, double cy, double radius,
                           double startAngle, double endAngle);
    /// Arc from three points (p1 = start, p2 = mid, p3 = end).
    Q_INVOKABLE int addArc3Point(double x1, double y1,
                                  double x2, double y2,
                                  double x3, double y3);
    /// Elliptical arc — rotation and angles in degrees at the QML boundary,
    /// converted to radians for the facade.
    Q_INVOKABLE int addArcEllipse(double cx, double cy,
                                   double majorRadius, double minorRadius,
                                   double rotationDeg,
                                   double startAngleDeg, double endAngleDeg);
    /// Hyperbolic arc — same scalar contract as addArcEllipse, angles are
    /// hyperbolic parameters (not angles in the usual sense).
    Q_INVOKABLE int addArcHyperbola(double cx, double cy,
                                     double majorRadius, double minorRadius,
                                     double rotationDeg,
                                     double startParam, double endParam);
    /// Parabolic arc at @p vertex with focal length @p focal, trimmed
    /// between parametric coordinates (y² = 4·f·x in local frame).
    Q_INVOKABLE int addArcParabola(double vx, double vy, double focal,
                                    double rotationDeg,
                                    double startParam, double endParam);
    /// Circle from three points on its circumference.
    Q_INVOKABLE int addCircle3Point(double x1, double y1,
                                     double x2, double y2,
                                     double x3, double y3);
    /// Ellipse from three points — p1/p2 are major-axis endpoints, p3 a
    /// rim point used to infer the minor radius (FreeCAD ThreeRim mode).
    Q_INVOKABLE int addEllipse3Point(double x1, double y1,
                                      double x2, double y2,
                                      double x3, double y3);
    Q_INVOKABLE int addRectangle(double x1, double y1, double x2, double y2);
    Q_INVOKABLE int addPoint(double x, double y);
    Q_INVOKABLE int addEllipse(double cx, double cy, double majorR, double minorR, double angleDeg = 0);
    Q_INVOKABLE int addBSpline(const QVariantList& points, int degree = 3);
    Q_INVOKABLE int addPolyline(const QVariantList& points);
    Q_INVOKABLE int toggleConstruction(int geoId);
    Q_INVOKABLE void removeGeometry(int geoId);

    // ── Sketch constraints ──────────────────────────────────────────
    Q_INVOKABLE int addDistanceConstraint(int geoId, double value);
    Q_INVOKABLE int addRadiusConstraint(int geoId, double value);
    Q_INVOKABLE int addHorizontalConstraint(int geoId);
    Q_INVOKABLE int addVerticalConstraint(int geoId);
    Q_INVOKABLE int addCoincidentConstraint(int geo1, int pos1, int geo2, int pos2);
    Q_INVOKABLE int addAngleConstraint(int geo1, int geo2, double angleDeg);
    Q_INVOKABLE int addFixedConstraint(int geoId);
    Q_INVOKABLE int addDistanceXConstraint(int geoId, double value);
    Q_INVOKABLE int addDistanceYConstraint(int geoId, double value);
    Q_INVOKABLE int addDiameterConstraint(int geoId, double value);
    Q_INVOKABLE int addSymmetricConstraint(int geo1, int pos1, int geo2, int pos2, int symGeo, int symPos);
    Q_INVOKABLE int addPointOnObjectConstraint(int pointGeo, int pointPos, int objectGeo);
    /// Generic two-geometry constraint (parallel, perpendicular, tangent, equal, coincident).
    /// If secondGeoId is -1, the first geoId is stored and constraint is deferred
    /// until the method is called again with the same type and a different geoId.
    Q_INVOKABLE int addConstraintTwoGeo(const QString& type, int geoId);
    Q_INVOKABLE void removeConstraint(int constraintId);
    Q_INVOKABLE void setDatum(int constraintId, double value);
    Q_INVOKABLE void toggleDriving(int constraintId);

    // ── Sketch tools ────────────────────────────────────────────────
    Q_INVOKABLE int trimAtPoint(int geoId, double px, double py);
    Q_INVOKABLE int filletVertex(int geoId, int posId, double radius);
    Q_INVOKABLE int chamferVertex(int geoId, int posId, double size);
    Q_INVOKABLE int extendGeo(int geoId, double increment, int endPointPos);
    Q_INVOKABLE int splitAtPoint(int geoId, double px, double py);

    // ── Part features (3D operations) ─────────────────────────────
    Q_INVOKABLE QString pad(const QString& sketchName, double length);
    Q_INVOKABLE QString pocket(const QString& sketchName, double depth);
    Q_INVOKABLE QString revolution(const QString& sketchName, double angleDeg);
    Q_INVOKABLE QString groove(const QString& sketchName, double angleDeg);

    // ── Parametric edit (UX-010 / UX-011) ──────────────────────────
    /// Returns a map with the editable parameters of an existing feature.
    /// Keys: typeName, sketchName, length, angle, reversed, editable (bool).
    /// editable=false means the feature was produced by the OCCT fallback
    /// path and can't be mutated in place — the UI should fall back to the
    /// create-new flow in that case.
    Q_INVOKABLE QVariantMap getFeatureParams(const QString& featureName);
    Q_INVOKABLE bool updatePad(const QString& featureName, double length, bool reversed);
    Q_INVOKABLE bool updatePocket(const QString& featureName, double depth, bool reversed);
    Q_INVOKABLE bool updateRevolution(const QString& featureName, double angleDeg);
    Q_INVOKABLE bool updateGroove(const QString& featureName, double angleDeg);

    // UX-008: rich Pad/Pocket — the QML task panel passes sideType/method/
    // length2 together. `opts` is a QVariantMap with any of:
    //   length (double), length2 (double), reversed (bool),
    //   sideType ("One side"/"Two sides"/"Symmetric"),
    //   method   ("Length"/"ThroughAll")
    Q_INVOKABLE QString padEx(const QString& sketchName, const QVariantMap& opts);
    Q_INVOKABLE QString pocketEx(const QString& sketchName, const QVariantMap& opts);
    Q_INVOKABLE bool updatePadEx(const QString& featureName, const QVariantMap& opts);
    Q_INVOKABLE bool updatePocketEx(const QString& featureName, const QVariantMap& opts);

    // Boolean operations
    Q_INVOKABLE QString booleanFuse(const QString& baseName, const QString& toolName);
    Q_INVOKABLE QString booleanCut(const QString& baseName, const QString& toolName);
    Q_INVOKABLE QString booleanCommon(const QString& baseName, const QString& toolName);

    // Primitives
    Q_INVOKABLE QString addBox(double length, double width, double height);
    Q_INVOKABLE QString addCylinder(double radius, double height, double angle = 360.0);
    Q_INVOKABLE QString addSphere(double radius);
    Q_INVOKABLE QString addCone(double radius1, double radius2, double height);

    // Dress-up (all edges)
    Q_INVOKABLE QString filletAll(const QString& featureName, double radius);
    Q_INVOKABLE QString chamferAll(const QString& featureName, double size);

    // Patterns
    Q_INVOKABLE QString linearPattern(const QString& featureName, double length, int occurrences);
    Q_INVOKABLE QString polarPattern(const QString& featureName, double angleDeg, int occurrences);
    Q_INVOKABLE QString mirrorFeature(const QString& featureName);

    // ── CAM ─────────────────────────────────────────────────────────
    Q_INVOKABLE void camSetStock(double length, double width, double height);
    Q_INVOKABLE int camAddTool(const QString& name, double diameter, double fluteLength);
    Q_INVOKABLE int camAddController(int toolId, double rpm, double feedXY, double feedZ);
    Q_INVOKABLE int camAddProfile(int controllerId, double depth, double stepDown,
                                   double x1, double y1, double x2, double y2);
    Q_INVOKABLE int camAddPocket(int controllerId, double depth, double stepDown, double stepOver,
                                  double x1, double y1, double x2, double y2);
    Q_INVOKABLE int camAddDrill(int controllerId, double depth, const QVariantList& points);
    Q_INVOKABLE int camAddFacing(int controllerId, double depth, double stepOver);
    Q_INVOKABLE int camOpCount() const;
    Q_INVOKABLE QString camGenerateGCode() const;
    Q_INVOKABLE bool camExportGCode(const QString& filePath, bool codesys = false) const;

    // ── Nesting ─────────────────────────────────────────────────────
    Q_INVOKABLE bool nestAddPart(const QString& id, double width, double height,
                                  int quantity = 1, bool allowRotation = true);
    Q_INVOKABLE void nestClearParts();
    Q_INVOKABLE void nestSetSheet(const QString& id, double width, double height);
    Q_INVOKABLE void nestSetPartGap(double gap);
    Q_INVOKABLE void nestSetEdgeGap(double gap);
    Q_INVOKABLE void nestSetRotation(int mode);
    Q_INVOKABLE QVariantMap nestRun(int algorithm = 1);

    // ── Solver ──────────────────────────────────────────────────────
    Q_INVOKABLE QString solve();

    // ── Property readers ────────────────────────────────────────────
    QVariantList featureTree() const;
    QVariantList sketchGeometry() const;
    QVariantList sketchConstraints() const;
    QString solverStatus() const;
    QString statusMessage() const;
    bool sketchActive() const;
    bool canUndo() const;
    bool canRedo() const;
    QStringList sketchNames() const;
    /// Names of solid/shape features that can host a sketch (Pad, Pocket,
    /// Revolution, primitives, booleans, fillets, chamfers — everything
    /// producing a non-null TopoDS_Shape).
    Q_INVOKABLE QStringList solidFeatureNames() const;
    bool hasDocument() const;
    QString documentPath() const;
    QString documentName() const;
    bool documentModified() const;

    double gridSpacing() const { return gridSpacing_; }
    void setGridSpacing(double mm);

    bool gridVisible() const { return gridVisible_; }
    void setGridVisible(bool on);

    bool facePickMode() const { return facePickMode_; }
    void setFacePickMode(bool on);

    /// Render-thread callback: a face pick landed on this (feature, sub) pair.
    /// Forwards to the facePicked signal on the UI thread.
    void reportFacePicked(const QString& featureName, const QString& subName);

    QString lastError() const { return lastError_; }

Q_SIGNALS:
    void featureTreeChanged();
    void sketchChanged();
    void statusMessageChanged();
    void gridSpacingChanged();
    void gridVisibleChanged();
    void undoStateChanged();
    void facePickModeChanged();
    void facePicked(QString featureName, QString subName);

    /// Per WRAPPER_CONTRACT § 2.3 — emitted when a facade call raises
    /// FacadeError. QML shows a toast; message already translated via
    /// FacadeError::userMessage() / qsTr wrappers inside the facade.
    void errorOccurred(QString message);

private:
    // Internal RAII wrapper that opens a FreeCAD undo transaction on entry
    // and commits it on exit (definition in .cpp).
    struct TxScope;
    friend struct TxScope;

    std::unique_ptr<CadSession> session_;
    std::shared_ptr<CadDocument> document_;
    std::shared_ptr<SketchFacade> activeSketch_;
    std::string activeSketchName_;   // name of sketch being edited (for viewport sync)
    std::unordered_map<std::string, int> sketchPlanes_;  // sketch name → planeType (0/1/2)
    QString solverStatus_;
    QString statusMessage_;
    double gridSpacing_ = 10.0;  // mm — default, matches OccRenderer initial grid
    bool   gridVisible_ = true;  // on at startup; controls both 2D canvas and 3D viewer grid
    bool   facePickMode_ = false;

    void refreshSketch();
    void setStatus(const QString& msg);
    void updateViewportShapes();

    /// Store and broadcast a facade-layer error. Emits errorOccurred so
    /// QML toast UI reacts; setLastError("") clears after a successful
    /// mutation.
    void setLastError(const QString& message);

    QString lastError_;

    OccViewport* viewport_ = nullptr;
    QString documentPath_;
    std::unique_ptr<CamFacade> cam_;
    std::unique_ptr<NestFacade> nest_;

    // Pending two-geometry constraint state
    QString pendingConstraintType_;
    int pendingFirstGeo_ = -1;
};

} // namespace CADNC
