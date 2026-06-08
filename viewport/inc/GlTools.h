#pragma once

/**
 * @file GlTools.h
 * @brief Utility functions bridging Qt input/GL types to OCCT equivalents.
 */

#include <QMouseEvent>
#include <Aspect_VKey.hxx>
#include <Graphic3d_Vec2.hxx>
#include <OpenGl_Context.hxx>
#include <V3d_View.hxx>

namespace CADNC {

class GlTools {
public:
    /// Extract OpenGL context from an OCCT V3d_View
    static Handle(OpenGl_Context) getGlContext(const Handle(V3d_View)& view);

    /// Convert Qt keyboard modifiers to OCCT flags
    static Aspect_VKeyFlags qtModifiers2VKeys(Qt::KeyboardModifiers modifiers);

    /// Convert Qt mouse buttons to OCCT flags
    static Aspect_VKeyMouse qtButtons2VKeys(Qt::MouseButtons buttons);

    /// Convert Qt position to OCCT pixel coordinates (with DPI scale)
    static Graphic3d_Vec2i convertPos(const QPointF& pos, double scale);
};

} // namespace CADNC
