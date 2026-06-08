#pragma once

/**
 * @file FrameBuffer.h
 * @brief Qt-compatible OpenGL framebuffer wrapper for OCCT rendering.
 *
 * Qt creates its FBO with GL_RGBA8 format; OCCT expects sRGB handling.
 * This subclass disables GL_FRAMEBUFFER_SRGB on bind so OCCT handles
 * gamma correction in its shaders.
 */

#include <OpenGl_Context.hxx>
#include <OpenGl_FrameBuffer.hxx>

namespace CADNC {

class QtFrameBuffer : public OpenGl_FrameBuffer {
    DEFINE_STANDARD_RTTI_INLINE(QtFrameBuffer, OpenGl_FrameBuffer)

public:
    QtFrameBuffer() = default;

    void BindBuffer(const Handle(OpenGl_Context)& ctx) override
    {
        OpenGl_FrameBuffer::BindBuffer(ctx);
        ctx->SetFrameBufferSRGB(true, false);
    }

    void BindDrawBuffer(const Handle(OpenGl_Context)& ctx) override
    {
        OpenGl_FrameBuffer::BindDrawBuffer(ctx);
        ctx->SetFrameBufferSRGB(true, false);
    }

    void BindReadBuffer(const Handle(OpenGl_Context)& ctx) override
    {
        OpenGl_FrameBuffer::BindReadBuffer(ctx);
    }
};

} // namespace CADNC
