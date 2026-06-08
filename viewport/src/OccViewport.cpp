/**
 * @file OccViewport.cpp
 * @brief QML-facing 3D viewport widget implementation.
 */

#include "OccViewport.h"
#include "OccRenderer.h"
#include "GlTools.h"

#include <QQuickWindow>

namespace CADNC {

OccViewport::OccViewport(QQuickItem* parent)
    : QQuickFramebufferObject(parent)
{
    setMirrorVertically(true);  // Qt FBO y-axis handling
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

OccViewport::~OccViewport() = default;

QQuickFramebufferObject::Renderer* OccViewport::createRenderer() const
{
    auto* r = new OccRenderer(this);
    renderer_ = r;
    return r;
}

// ── Shape management ────────────────────────────────────────────────

void OccViewport::addShape(const QString& id, const QVariant& /*shapePtr*/,
                           double r, double g, double b)
{
    // QML-facing stub — real shape addition goes through displayShape()
    Q_UNUSED(id); Q_UNUSED(r); Q_UNUSED(g); Q_UNUSED(b);
}

void OccViewport::removeShape(const QString& id)
{
    if (renderer_)
        renderer_->queueRemoveShape(id.toStdString());
    update();
}

void OccViewport::clearShapes()
{
    if (renderer_)
        renderer_->queueClearShapes();
    update();
}

void OccViewport::setFacePickMode(bool on)
{
    if (renderer_) renderer_->setFacePickMode(on);
}

void OccViewport::facePickedFromRender(const QString& featureName, const QString& subName)
{
    Q_EMIT facePicked(featureName, subName);
}

void OccViewport::fitAll()
{
    if (renderer_)
        renderer_->queueFitAll();
    update();
}

void OccViewport::displayShape(const std::string& id, const TopoDS_Shape& shape,
                               const Quantity_Color& color, bool wireframe)
{
    if (renderer_)
        renderer_->queueAddShape(id, shape, color, wireframe);
    update();
}

// ── View presets ────────────────────────────────────────────────────

void OccViewport::viewTop()
{
    if (!renderer_) return;
    renderer_->queueViewPreset(1);
    update();
}

void OccViewport::viewFront()
{
    if (!renderer_) return;
    renderer_->queueViewPreset(2);
    update();
}

void OccViewport::viewRight()
{
    if (!renderer_) return;
    renderer_->queueViewPreset(3);
    update();
}

void OccViewport::setGridStep(double mm)
{
    if (!renderer_) return;
    renderer_->queueGridStep(mm);
    update();
}

void OccViewport::setGridVisible(bool on)
{
    userWantsGrid_ = on;
    if (!renderer_) return;
    // While sketching, the SketchCanvas owns the visible grid — never let the
    // OCCT 3D grid show through behind it (its world-space dots don't align
    // with the SketchCanvas's pan/zoom-space dots, which user testing flagged
    // as "visible grid doesn't match the snap grid").
    renderer_->queueGridVisible(on && !sketchMode_);
    update();
}

void OccViewport::setSketchMode(bool on)
{
    if (sketchMode_ == on) return;
    sketchMode_ = on;
    // Flip the OCCT grid in lockstep: off in sketch mode, back to the user's
    // preference in part mode.
    if (renderer_) {
        renderer_->queueGridVisible(!on && userWantsGrid_);
        update();
    }
    Q_EMIT sketchModeChanged();
}

void OccViewport::viewIsometric()
{
    if (!renderer_) return;
    renderer_->queueViewPreset(4);
    update();
}

void OccViewport::forwardNavCubeClick(int px, int py)
{
    // Convert from logical (DPI) to physical pixels and forward to the renderer.
    if (!renderer_) return;
    const double s = renderer_->scale();
    renderer_->queueViewCubeClick(static_cast<int>(px * s), static_cast<int>(py * s));
    update();
}

// ── Mouse events → AIS_ViewController on render thread ──────────────

void OccViewport::mousePressEvent(QMouseEvent* event)
{
    if (!renderer_ || renderer_->view().IsNull()) return;

    // In sketch mode, block right-button orbit (MilCAD pattern).
    // ViewCube clicks, middle-button pan, and scroll zoom still work.
    const bool blockOrbit = sketchMode_ && event->button() == Qt::RightButton;
    if (blockOrbit) return;

    double s = renderer_->scale();
    Graphic3d_Vec2i pos = GlTools::convertPos(event->position(), s);
    Aspect_VKeyFlags flags = GlTools::qtModifiers2VKeys(event->modifiers());
    Aspect_VKeyMouse buttons = GlTools::qtButtons2VKeys(event->buttons());

    // Left-click: check ViewCube first, then selection
    if (event->button() == Qt::LeftButton) {
        renderer_->queueViewCubeClick(pos.x(), pos.y());
        renderer_->queueSelect(pos.x(), pos.y());
    }

    renderer_->UpdateMouseButtons(pos, buttons, flags, false);
    update();
}

void OccViewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (!renderer_ || renderer_->view().IsNull()) return;

    double s = renderer_->scale();
    Graphic3d_Vec2i pos = GlTools::convertPos(event->position(), s);
    Aspect_VKeyFlags flags = GlTools::qtModifiers2VKeys(event->modifiers());
    Aspect_VKeyMouse buttons = GlTools::qtButtons2VKeys(event->buttons());

    renderer_->UpdateMouseButtons(pos, buttons, flags, false);
    update();
}

void OccViewport::mouseMoveEvent(QMouseEvent* event)
{
    if (!renderer_ || renderer_->view().IsNull()) return;

    double s = renderer_->scale();
    Graphic3d_Vec2i pos = GlTools::convertPos(event->position(), s);
    Aspect_VKeyFlags flags = GlTools::qtModifiers2VKeys(event->modifiers());

    renderer_->UpdateMousePosition(pos, renderer_->PressedMouseButtons(), flags, false);
    update();
}

void OccViewport::hoverMoveEvent(QHoverEvent* event)
{
    if (!renderer_ || renderer_->view().IsNull()) return;

    double s = renderer_->scale();
    Graphic3d_Vec2i pos = GlTools::convertPos(event->position(), s);
    Aspect_VKeyFlags flags = GlTools::qtModifiers2VKeys(event->modifiers());

    renderer_->UpdateMousePosition(pos, Aspect_VKeyMouse_NONE, flags, false);
    update();
}

void OccViewport::wheelEvent(QWheelEvent* event)
{
    if (!renderer_ || renderer_->view().IsNull()) return;

    double s = renderer_->scale();
    Graphic3d_Vec2i pos(static_cast<int>(event->position().x() * s),
                        static_cast<int>(event->position().y() * s));
    // One notch on a typical mouse is 120 angleDelta units. OCCT's UpdateZoom
    // expects a factor around the same order of magnitude as the screen-space
    // pixel delta; the previous /8 divisor under-damped that and the camera
    // crawled in and out. /2 gives a proportional step per notch which lines
    // up with FreeCAD's "middle-speed" feel.
    double delta = event->angleDelta().y() / 2.0;

    renderer_->UpdateZoom(Aspect_ScrollDelta(pos, delta));
    update();
}

} // namespace CADNC
