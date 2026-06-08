#pragma once

/**
 * @file OccViewport.h
 * @brief QML-facing 3D viewport widget using OCCT V3d rendering.
 *
 * OccViewport is a QQuickFramebufferObject that embeds an OCCT 3D viewer.
 * It handles mouse input (orbit, pan, zoom via AIS_ViewController) and
 * delegates shape display/selection to the render-thread OccRenderer.
 *
 * Usage in QML:
 *   OccViewport { anchors.fill: parent }
 *
 * Must be registered via qmlRegisterType before QML engine loads.
 */

#include <QQuickFramebufferObject>
#include <QMouseEvent>
#include <QWheelEvent>

#include <TopoDS_Shape.hxx>
#include <Quantity_Color.hxx>

#include <string>

namespace CADNC {

class OccRenderer;

class OccViewport : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

    /// When true, right-button orbit is blocked (sketch mode)
    Q_PROPERTY(bool sketchMode READ sketchMode WRITE setSketchMode NOTIFY sketchModeChanged)

public:
    explicit OccViewport(QQuickItem* parent = nullptr);
    ~OccViewport() override;

    Renderer* createRenderer() const override;

    // ── Shape management (thread-safe, deferred to render thread) ───
    Q_INVOKABLE void addShape(const QString& id, const QVariant& shapePtr,
                               double r = 0.5, double g = 0.7, double b = 0.9);
    Q_INVOKABLE void removeShape(const QString& id);
    Q_INVOKABLE void clearShapes();
    Q_INVOKABLE void fitAll();

    // ── View presets ────────────────────────────────────────────────
    Q_INVOKABLE void viewTop();
    Q_INVOKABLE void viewFront();
    Q_INVOKABLE void viewRight();
    Q_INVOKABLE void viewIsometric();

    /// Forward a click to the ViewCube even when an overlay (SketchCanvas)
    /// covers this widget. Used so the NavCube stays interactive in sketch mode.
    Q_INVOKABLE void forwardNavCubeClick(int px, int py);

    /// Direct native shape API (called from C++ adapter, not QML)
    /// wireframe: true = display as wireframe (for sketches), false = shaded (for solids)
    void displayShape(const std::string& id, const TopoDS_Shape& shape,
                      const Quantity_Color& color, bool wireframe = false);

    /// Update the rectangular grid spacing (mm). Thread-safe: the change is
    /// queued to the render thread so the OCCT viewer is not touched from UI.
    Q_INVOKABLE void setGridStep(double mm);

    /// Toggle visibility of the OCCT 3D viewer grid. Queued to render thread.
    Q_INVOKABLE void setGridVisible(bool on);

    bool sketchMode() const { return sketchMode_; }
    void setSketchMode(bool on);

    /// Enable / disable face-pick mode on the render thread. When on, the
    /// next left-click resolves the picked (feature, "FaceN") pair and the
    /// viewport emits facePicked on the UI thread via synchronize().
    Q_INVOKABLE void setFacePickMode(bool on);

    /// Called from OccRenderer::synchronize() on the UI thread after a face
    /// was picked. Emits facePicked so CadEngine / DatumPlanePanel can react.
    void facePickedFromRender(const QString& featureName, const QString& subName);

Q_SIGNALS:
    void viewReady();
    void sketchModeChanged();
    void facePicked(QString featureName, QString subName);

protected:
    // Mouse event handling (forwarded to AIS_ViewController on render thread)
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    // Renderer is owned by Qt's scene graph, we keep a non-owning pointer
    mutable OccRenderer* renderer_ = nullptr;
    bool sketchMode_ = false;
    // Remember the user's intended grid visibility so we can restore it when
    // leaving sketch mode. Sketch mode force-hides the OCCT 3D grid so it
    // can't overlay the SketchCanvas 2D grid at a different coordinate origin.
    bool userWantsGrid_ = true;
};

} // namespace CADNC
