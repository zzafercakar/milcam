/**
 * @file OccRenderer.cpp
 * @brief OCCT 3D renderer integrated into Qt Quick's FBO pipeline.
 */

#include <QOpenGLFramebufferObject>
#include <QQuickWindow>

#include <Standard_Version.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_FrameBuffer.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Graphic3d_TransformPers.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>

#include "FrameBuffer.h"
#include "GlTools.h"
#include "OccRenderer.h"
#include "OccViewport.h"

namespace CADNC {

OccRenderer::OccRenderer(const OccViewport* item)
    : viewerItem_(item)
{
    // Create graphic driver (no native display window needed for FBO)
    Handle(Aspect_DisplayConnection) display = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(display, false);

    // Critical for FBO rendering — don't swap buffers, Qt manages the FBO
    driver->ChangeOptions().buffersNoSwap = true;
    driver->ChangeOptions().buffersOpaqueAlpha = true;
    driver->ChangeOptions().useSystemBuffer = false;

    // Create 3D viewer with default lighting
    viewer_ = new V3d_Viewer(driver);
    viewer_->SetDefaultBackgroundColor(Quantity_Color(0.94, 0.95, 0.97, Quantity_TOC_sRGB));
    viewer_->SetDefaultLights();
    viewer_->SetLightOn();

    // Create AIS context for shape display and selection
    context_ = new AIS_InteractiveContext(viewer_);
    context_->SetDisplayMode(AIS_Shaded, false);
    context_->SetAutoActivateSelection(true);
    context_->SetAutomaticHilight(true);
    context_->SetPixelTolerance(5);

    // High-quality tessellation for smooth curves
    context_->SetDeviationCoefficient(0.0001);
    context_->SetDeviationAngle(0.02);

    // Create the 3D view
    view_ = viewer_->CreateView();
    view_->SetProj(V3d_XposYnegZpos);  // isometric
    view_->SetImmediateUpdate(false);
    view_->ChangeRenderingParams().ToShowStats = false;
    view_->ChangeRenderingParams().NbMsaaSamples = 8;
    view_->ChangeRenderingParams().IsAntialiasingEnabled = true;

    // Light modern gradient background
    view_->SetBgGradientColors(
        Quantity_Color(0.90, 0.92, 0.96, Quantity_TOC_sRGB),  // top
        Quantity_Color(0.96, 0.97, 0.99, Quantity_TOC_sRGB),  // bottom
        Aspect_GFM_VER);

    // Set AIS_ViewController mouse gesture bindings (OnShape-style)
    myMouseGestureMap.Clear();
    myMouseGestureMap.Bind(Aspect_VKeyMouse_RightButton, AIS_MouseGesture_RotateOrbit);
    myMouseGestureMap.Bind(Aspect_VKeyMouse_MiddleButton, AIS_MouseGesture_Pan);

    initializeSceneHelpers();
}

OccRenderer::~OccRenderer()
{
    context_->RemoveAll(false);
    context_.Nullify();
    view_->Remove();
    view_.Nullify();
    viewer_.Nullify();
}

void OccRenderer::synchronize(QQuickFramebufferObject* item)
{
    scale_.store(item->window()->devicePixelRatio(), std::memory_order_relaxed);

    // UI thread: drain any face picks produced by the render thread and let
    // OccViewport forward them to CadEngine. synchronize() runs on the UI
    // thread between render() calls, so Qt signal emission here is safe.
    auto picks = drainPickedFaces();
    if (!picks.empty()) {
        if (auto* vp = const_cast<OccViewport*>(viewerItem_)) {
            for (const auto& p : picks)
                vp->facePickedFromRender(QString::fromStdString(p.first),
                                         QString::fromStdString(p.second));
        }
    }
}

void OccRenderer::render()
{
    if (view_.IsNull() || view_->Window().IsNull())
        return;

    // Acquire Qt's GL context and set up FBO
    Handle(OpenGl_Context) glCtx = GlTools::getGlContext(view_);
    Handle(OpenGl_FrameBuffer) defaultFbo = glCtx->DefaultFrameBuffer();
    if (defaultFbo.IsNull()) {
        defaultFbo = new QtFrameBuffer();
        glCtx->SetDefaultFrameBuffer(defaultFbo);
    }
    if (!defaultFbo->InitWrapper(glCtx)) {
        defaultFbo.Nullify();
        return;
    }
    defaultFbo->SetupViewport(glCtx);

    // Keep OCCT window size in sync with FBO
    Graphic3d_Vec2i newSize = defaultFbo->GetVPSize();
    {
        Graphic3d_Vec2i oldSize;
        view_->Window()->Size(oldSize.x(), oldSize.y());
        if (newSize != oldSize) {
            auto neutralWin = Handle(Aspect_NeutralWindow)::DownCast(view_->Window());
            if (!neutralWin.IsNull())
                neutralWin->SetSize(newSize.x(), newSize.y());
            view_->MustBeResized();
            view_->Invalidate();
        }
    }

    // Process deferred fit-all
    if (pendingFitAll_.exchange(false)) {
        view_->FitAll();
        view_->ZFitAll();
    }

    // Process deferred grid-visibility change. ActivateGrid/DeactivateGrid
    // touch viewer state so they must run on the render thread.
    int vis = pendingGridVisible_.exchange(0, std::memory_order_acquire);
    if (vis != 0 && !viewer_.IsNull()) {
        if (vis > 0) viewer_->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Points);
        else         viewer_->DeactivateGrid();
        view_->Invalidate();
    }

    // Process deferred grid-step change. OCCT's grid API mutates viewer
    // geometry so it must run on the render thread.
    double newStep = pendingGridStep_.exchange(0.0, std::memory_order_acquire);
    if (newStep > 0.0 && !viewer_.IsNull()) {
        // BUG-009: a fixed 5000mm graphic extent with a 1mm step produces
        // 10000x10000 dots and freezes OCCT's point renderer. Bound the
        // total dot count by shrinking the extent when step is small.
        // Target: ≤200 dots per axis (40k total) — stays smooth on weak GPUs.
        double extent = newStep * 100.0;               // 200 dots per axis
        if (extent < 200.0)  extent = 200.0;           // keep a usable view
        if (extent > 2000.0) extent = 2000.0;          // cap for huge steps
        viewer_->SetRectangularGridValues(0.0, 0.0, newStep, newStep, 0.0);
        viewer_->SetRectangularGridGraphicValues(extent, extent, 0.0);
        view_->Invalidate();
    }

    // Process deferred view preset (SetProj must run on render thread)
    int preset = pendingViewPreset_.exchange(0, std::memory_order_acquire);
    if (preset != 0) {
        switch (preset) {
        case 1:  // Top
            view_->SetProj(V3d_Zpos);
            view_->SetUp(0.0, 1.0, 0.0);
            break;
        case 2:  // Front
            view_->SetProj(V3d_Yneg);
            view_->SetUp(0.0, 0.0, 1.0);
            break;
        case 3:  // Right
            view_->SetProj(V3d_Xpos);
            view_->SetUp(0.0, 0.0, 1.0);
            break;
        case 4:  // Isometric
            view_->SetProj(V3d_XposYnegZpos);
            view_->SetUp(0.0, 0.0, 1.0);
            break;
        }
        view_->FitAll();
        view_->ZFitAll();
        view_->Invalidate();
    }

    // Process deferred shape operations
    processPendingShapeOps();

    view_->InvalidateImmediate();

    // Flush camera animation (ViewCube, orbit, zoom)
    FlushViewEvents(context_, view_, true);

    // Process deferred ViewCube click
    if (pendingViewCubeClick_.exchange(false, std::memory_order_acquire)) {
        context_->MoveTo(pendingViewCubeClickX_, pendingViewCubeClickY_, view_, false);
        Handle(SelectMgr_EntityOwner) owner = context_->DetectedOwner();
        Handle(AIS_ViewCubeOwner) cubeOwner = Handle(AIS_ViewCubeOwner)::DownCast(owner);
        if (!cubeOwner.IsNull()) {
            const auto orient = cubeOwner->MainOrientation();
            view_->SetProj(orient);
            if (orient == V3d_Zpos || orient == V3d_Zneg)
                view_->SetUp(0.0, 1.0, 0.0);
            else
                view_->SetUp(0.0, 0.0, 1.0);
            view_->FitAll();
            view_->ZFitAll();
            view_->Invalidate();
        }
    }

    // Process deferred selection
    if (pendingSelect_.exchange(false, std::memory_order_acquire)) {
        context_->MoveTo(pendingSelectX_, pendingSelectY_, view_, false);
        // Skip ViewCube for entity selection
        bool isViewCube = false;
        if (context_->HasDetected()) {
            Handle(AIS_InteractiveObject) detected = context_->DetectedInteractive();
            isViewCube = (!detected.IsNull() && detected == viewCube_);
        }
        if (!isViewCube) {
            context_->SelectDetected();
            // If the DatumPlanePanel asked for a face pick, see if the click
            // landed on one and surface it to the UI thread.
            if (facePickMode_.load(std::memory_order_relaxed)) {
                tryResolveFacePick();
            }
        }
    }
}

// Resolve the current AIS selection into (featureName, "FaceN") if it's a
// face pick; pushes onto pickedFaces_. Caller runs on the render thread.
void OccRenderer::tryResolveFacePick()
{
    if (context_.IsNull()) return;

    for (context_->InitSelected(); context_->MoreSelected(); context_->NextSelected()) {
        Handle(AIS_InteractiveObject) ao = context_->SelectedInteractive();
        if (ao.IsNull()) continue;
        Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(ao);
        if (aisShape.IsNull()) continue;

        TopoDS_Shape picked = context_->SelectedShape();
        if (picked.IsNull() || picked.ShapeType() != TopAbs_FACE) continue;

        // Reverse-lookup feature name from the shapes_ map.
        std::string featureName;
        for (const auto& kv : shapes_) {
            if (kv.second == aisShape) { featureName = kv.first; break; }
        }
        if (featureName.empty()) continue;

        // Parent AIS holds the cumulative body shape; enumerate faces to
        // determine the "FaceN" index that matches what
        // CadDocument::featureFaceNames() produces.
        const TopoDS_Shape& parent = aisShape->Shape();
        int idx = 0, matched = -1;
        for (TopExp_Explorer ex(parent, TopAbs_FACE); ex.More(); ex.Next()) {
            ++idx;
            if (ex.Current().IsSame(picked)) { matched = idx; break; }
        }
        if (matched < 0) continue;

        std::string sub = "Face" + std::to_string(matched);
        std::lock_guard<std::mutex> lk(pickMutex_);
        pickedFaces_.emplace_back(featureName, sub);
    }
}

std::vector<std::pair<std::string,std::string>> OccRenderer::drainPickedFaces()
{
    std::lock_guard<std::mutex> lk(pickMutex_);
    std::vector<std::pair<std::string,std::string>> out;
    out.swap(pickedFaces_);
    return out;
}

QOpenGLFramebufferObject* OccRenderer::createFramebufferObject(const QSize& size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    format.setSamples(0);  // OCCT handles MSAA internally
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setInternalTextureFormat(GL_RGBA8);

    // First-time GL initialization
    if (view_->Window().IsNull())
        initializeGL(size);

    return new QOpenGLFramebufferObject(size, format);
}

void OccRenderer::initializeGL(const QSize& fboSize)
{
    const Graphic3d_Vec2i viewSize(fboSize.width(), fboSize.height());

    // Wrap Qt's current GL context for OCCT
    Handle(OpenGl_Context) glCtx = new OpenGl_Context();
    if (!glCtx->Init(false)) {  // false = not core profile on Linux
        return;
    }

    // On Linux: use NeutralWindow with X11 native handle for GLX
    WId xid = viewerItem_->window()->winId();
    Handle(Aspect_NeutralWindow) window = new Aspect_NeutralWindow();
    window->SetNativeHandle(static_cast<Aspect_Drawable>(xid));
    window->SetSize(viewSize.x(), viewSize.y());
    view_->SetWindow(window, glCtx->RenderingContext());
}

void OccRenderer::initializeSceneHelpers()
{
    if (view_.IsNull() || context_.IsNull() || viewer_.IsNull())
        return;

    // Axis triedron (lower-left corner)
    view_->ZBufferTriedronSetup(
        Quantity_Color(Quantity_NOC_RED),
        Quantity_Color(Quantity_NOC_GREEN),
        Quantity_Color(Quantity_NOC_BLUE1),
        0.82, 0.06, 18);
    view_->TriedronDisplay(Aspect_TOTP_LEFT_LOWER,
                           Quantity_Color(Quantity_NOC_WHITE), 0.075, V3d_ZBUFFER);

    // Interactive ViewCube (upper-right corner)
    viewCube_ = new AIS_ViewCube();
    viewCube_->SetSize(70.0, true);
    viewCube_->SetBoxColor(Quantity_Color(0.80, 0.85, 0.92, Quantity_TOC_sRGB));
    viewCube_->SetTransparency(0.15);
    viewCube_->SetAxesLabels("X", "Y", "Z");
    viewCube_->SetBoxSideLabel(V3d_Xpos, "RIGHT");
    viewCube_->SetBoxSideLabel(V3d_Xneg, "LEFT");
    viewCube_->SetBoxSideLabel(V3d_Yneg, "FRONT");
    viewCube_->SetBoxSideLabel(V3d_Ypos, "BACK");
    viewCube_->SetBoxSideLabel(V3d_Zpos, "TOP");
    viewCube_->SetBoxSideLabel(V3d_Zneg, "BOTTOM");
    viewCube_->SetTransformPersistence(new Graphic3d_TransformPers(
        Graphic3d_TMF_TriedronPers, Aspect_TOTP_RIGHT_UPPER, Graphic3d_Vec2i(120, 120)));
    viewCube_->SetFixedAnimationLoop(false);
    viewCube_->SetAutoStartAnimation(true);
    viewCube_->SetDuration(0.5);
    context_->Display(viewCube_, false);
    context_->Activate(viewCube_, 0, false);

    // Rectangular grid (points mode)
    viewer_->SetGridEcho(Standard_False);
    viewer_->SetRectangularGridValues(0.0, 0.0, 10.0, 10.0, 0.0);
    viewer_->SetRectangularGridGraphicValues(5000.0, 5000.0, 0.0);
    viewer_->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Points);
}

// ── Thread-safe queued operations ───────────────────────────────────

void OccRenderer::queueAddShape(const std::string& id, const TopoDS_Shape& shape,
                                const Quantity_Color& color, bool wireframe)
{
    std::lock_guard<std::mutex> lock(opsMutex_);
    pendingOps_.push_back({ShapeOp::Add, id, shape, color, wireframe});
}

void OccRenderer::queueRemoveShape(const std::string& id)
{
    std::lock_guard<std::mutex> lock(opsMutex_);
    pendingOps_.push_back({ShapeOp::Remove, id, {}, {}});
}

void OccRenderer::queueClearShapes()
{
    std::lock_guard<std::mutex> lock(opsMutex_);
    pendingOps_.push_back({ShapeOp::Clear, {}, {}, {}});
}

void OccRenderer::queueFitAll()
{
    pendingFitAll_.store(true, std::memory_order_release);
}

void OccRenderer::queueSelect(int px, int py)
{
    pendingSelectX_ = px;
    pendingSelectY_ = py;
    pendingSelect_.store(true, std::memory_order_release);
}

void OccRenderer::queueViewCubeClick(int px, int py)
{
    pendingViewCubeClickX_ = px;
    pendingViewCubeClickY_ = py;
    pendingViewCubeClick_.store(true, std::memory_order_release);
}

void OccRenderer::queueViewPreset(int preset)
{
    pendingViewPreset_.store(preset, std::memory_order_release);
}

void OccRenderer::queueGridStep(double mm)
{
    if (mm < 0.5) mm = 0.5;
    pendingGridStep_.store(mm, std::memory_order_release);
}

void OccRenderer::queueGridVisible(bool on)
{
    pendingGridVisible_.store(on ? 1 : -1, std::memory_order_release);
}

void OccRenderer::processPendingShapeOps()
{
    std::vector<ShapeOp> ops;
    {
        std::lock_guard<std::mutex> lock(opsMutex_);
        ops = std::move(pendingOps_);
        pendingOps_.clear();
    }

    for (const auto& op : ops) {
        switch (op.type) {
        case ShapeOp::Add: {
            Handle(AIS_Shape) aisShape = new AIS_Shape(op.shape);
            aisShape->SetColor(op.color);
            aisShape->SetDisplayMode(op.wireframe ? AIS_WireFrame : AIS_Shaded);
            if (op.wireframe) aisShape->SetWidth(2.0);
            context_->Display(aisShape, false);

            // Remove old shape with same ID if it exists
            auto it = shapes_.find(op.id);
            if (it != shapes_.end()) {
                context_->Remove(it->second, false);
                it->second = aisShape;
            } else {
                shapes_[op.id] = aisShape;
            }
            break;
        }
        case ShapeOp::Remove: {
            auto it = shapes_.find(op.id);
            if (it != shapes_.end()) {
                context_->Remove(it->second, false);
                shapes_.erase(it);
            }
            break;
        }
        case ShapeOp::Clear: {
            for (auto& [id, aisShape] : shapes_)
                context_->Remove(aisShape, false);
            shapes_.clear();
            break;
        }
        }
    }

    if (!ops.empty())
        context_->UpdateCurrentViewer();
}

void OccRenderer::handleViewRedraw(const Handle(AIS_InteractiveContext)& ctx,
                                   const Handle(V3d_View)& view)
{
    AIS_ViewController::handleViewRedraw(ctx, view);
    // Request QML update for camera animation continuation
    if (myToAskNextFrame && viewerItem_) {
        QMetaObject::invokeMethod(const_cast<OccViewport*>(viewerItem_), "update",
                                  Qt::QueuedConnection);
    }
}

} // namespace CADNC
