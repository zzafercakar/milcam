#include "GlTools.h"

#include <OpenGl_View.hxx>
#include <OpenGl_Window.hxx>

namespace CADNC {

Handle(OpenGl_Context) GlTools::getGlContext(const Handle(V3d_View)& view)
{
    if (view.IsNull()) return {};
    Handle(OpenGl_View) glView = Handle(OpenGl_View)::DownCast(view->View());
    if (glView.IsNull()) return {};
    Handle(OpenGl_Window) glWin = glView->GlWindow();
    if (glWin.IsNull()) return {};
    return glWin->GetGlContext();
}

Aspect_VKeyMouse GlTools::qtButtons2VKeys(Qt::MouseButtons buttons)
{
    Aspect_VKeyMouse result = Aspect_VKeyMouse_NONE;
    if (buttons & Qt::LeftButton)   result |= Aspect_VKeyMouse_LeftButton;
    if (buttons & Qt::MiddleButton) result |= Aspect_VKeyMouse_MiddleButton;
    if (buttons & Qt::RightButton)  result |= Aspect_VKeyMouse_RightButton;
    return result;
}

Aspect_VKeyFlags GlTools::qtModifiers2VKeys(Qt::KeyboardModifiers modifiers)
{
    Aspect_VKeyFlags flags = Aspect_VKeyFlags_NONE;
    if (modifiers & Qt::ShiftModifier)   flags |= Aspect_VKeyFlags_SHIFT;
    if (modifiers & Qt::ControlModifier) flags |= Aspect_VKeyFlags_CTRL;
    if (modifiers & Qt::AltModifier)     flags |= Aspect_VKeyFlags_ALT;
    return flags;
}

Graphic3d_Vec2i GlTools::convertPos(const QPointF& pos, double scale)
{
    return Graphic3d_Vec2i(static_cast<int>(pos.x() * scale),
                           static_cast<int>(pos.y() * scale));
}

} // namespace CADNC
