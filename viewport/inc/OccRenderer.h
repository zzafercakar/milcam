#pragma once

/**
 * @file OccRenderer.h
 * @brief FBO renderer driving the OCCT 3D viewer inside a QML scene.
 *
 * Lives on the Qt render thread. Owns V3d_Viewer, V3d_View, AIS context,
 * and ViewCube. Processes deferred operations (shape display, selection)
 * each frame.
 *
 * Thread safety: UI thread communicates via atomic flags and mutex-protected
 * queues. OCCT objects are only touched in render().
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <QQuickFramebufferObject>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <AIS_ViewController.hxx>
#include <Quantity_Color.hxx>
#include <Standard_Handle.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

namespace CADNC {

class OccViewport;  // forward

/**
 * @brief Pending shape operation queued from UI thread for render-thread execution.
 */
struct ShapeOp {
    enum Type { Add, Remove, Clear };
    Type type;
    std::string id;
    TopoDS_Shape shape;
    Quantity_Color color;
    bool wireframe = false;
};

class OccRenderer : public QQuickFramebufferObject::Renderer,
                    public AIS_ViewController
{
public:
    explicit OccRenderer(const OccViewport* item);
    ~OccRenderer() override;

    // QQuickFramebufferObject::Renderer interface
    void render() override;
    void synchronize(QQuickFramebufferObject* item) override;
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

    // ── Thread-safe queued operations (called from UI thread) ───────
    void queueAddShape(const std::string& id, const TopoDS_Shape& shape,
                       const Quantity_Color& color, bool wireframe = false);
    void queueRemoveShape(const std::string& id);
    void queueClearShapes();

    /// Queue a fit-all camera operation for next frame
    void queueFitAll();

    /// Queue a selection at pixel coordinates
    void queueSelect(int px, int py);

    /// Queue a ViewCube click
    void queueViewCubeClick(int px, int py);

    /// Queue a view preset change (1=top, 2=front, 3=right, 4=iso)
    void queueViewPreset(int preset);

    /// Queue a rectangular grid spacing change (mm, both X and Y step).
    /// Applied on the next render() — OCCT grid calls must run on the GL thread.
    void queueGridStep(double mm);

    /// Queue a grid visibility change (ActivateGrid / DeactivateGrid).
    void queueGridVisible(bool on);

    /// When on, the next viewport left-click is a face pick: the render
    /// thread resolves the selected (feature, subName) pair and the UI
    /// thread drains it in synchronize().
    void setFacePickMode(bool on) { facePickMode_.store(on, std::memory_order_relaxed); }
    bool facePickMode() const { return facePickMode_.load(std::memory_order_relaxed); }
    /// Drain any face picks produced since the last synchronize(). Called on
    /// UI thread so the caller can emit Qt signals safely.
    std::vector<std::pair<std::string,std::string>> drainPickedFaces();

    // ── Accessors ──────────────────────────────────────────────────
    Handle(V3d_View) view() const { return view_; }
    Handle(AIS_InteractiveContext) context() const { return context_; }
    double scale() const { return scale_.load(std::memory_order_relaxed); }

protected:
    /// Override to schedule QML update when OCCT requests another frame
    void handleViewRedraw(const Handle(AIS_InteractiveContext)& ctx,
                          const Handle(V3d_View)& view) override;

private:
    void initializeGL(const QSize& size);
    void initializeSceneHelpers();
    void processPendingShapeOps();

    const OccViewport* viewerItem_ = nullptr;

    Handle(V3d_Viewer) viewer_;
    Handle(V3d_View) view_;
    Handle(AIS_InteractiveContext) context_;
    Handle(AIS_ViewCube) viewCube_;

    // Shape storage: id → AIS_Shape (render thread only)
    std::map<std::string, Handle(AIS_Shape)> shapes_;

    // Read from UI thread (forwardNavCubeClick), written on render thread.
    std::atomic<double> scale_ {1.0};

    // Deferred operations
    std::mutex opsMutex_;
    std::vector<ShapeOp> pendingOps_;

    std::atomic<bool> pendingFitAll_{false};
    std::atomic<bool> pendingSelect_{false};
    int pendingSelectX_ = 0;
    int pendingSelectY_ = 0;

    std::atomic<bool> pendingViewCubeClick_{false};
    int pendingViewCubeClickX_ = 0;
    int pendingViewCubeClickY_ = 0;

    // Deferred view preset: projection direction queued from UI thread
    std::atomic<int> pendingViewPreset_{0};  // 0=none, 1=top, 2=front, 3=right, 4=iso

    // Deferred grid-step change. 0.0 = no pending change. Atomic double is
    // well-defined on common platforms; the value is only ever written by
    // the UI thread and consumed on the render thread.
    std::atomic<double> pendingGridStep_{0.0};

    // Deferred grid-visibility change. 0 = no pending, 1 = show, -1 = hide.
    std::atomic<int> pendingGridVisible_{0};

    // Face-pick workflow. UI thread sets facePickMode_ via setFacePickMode();
    // render thread resolves picks into pickedFaces_ after SelectDetected();
    // UI thread drains them in synchronize() to emit a Qt signal.
    std::atomic<bool> facePickMode_{false};
    std::mutex pickMutex_;
    std::vector<std::pair<std::string,std::string>> pickedFaces_;

    // Helper — resolve the currently-selected AIS_Shape + face into
    // (featureName, "FaceN"). Pushes onto pickedFaces_ if the selection is a
    // Face whose parent AIS is in shapes_.
    void tryResolveFacePick();
};

} // namespace CADNC
