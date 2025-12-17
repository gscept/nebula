#pragma once
//------------------------------------------------------------------------------
/**
    @class Im3dContext

    Nebula renderer for Im3d gizmos

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "coregraphics/buffer.h"
#include "graphics/graphicscontext.h"
#include "input/inputevent.h"
#include "math/bbox.h"
#include "math/line.h"

namespace Im3d
{
enum RenderFlag
{
    CheckDepth = 0x1,
    AlwaysOnTop = 0x2,
    Wireframe = 0x4,
    Solid = 0x8,
    NumDepthFlags
};

class Im3dContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    Im3dContext();
    /// destructor
    virtual ~Im3dContext();

    static void Create();
    static void Discard();

    static void DrawText(const Math::vec3& position, Util::String const& text, const float size = 10.0f, const Math::vec4 color = { 1.0f, 0.0f, 0.0f, 1.0f }, uint32_t renderFlags = CheckDepth);

    static void DrawPoint(const Math::vec3& position, const float size = 10.0f, const Math::vec4 color = { 1.0f, 0.0f, 0.0f, 1.0f }, uint32_t renderFlags = CheckDepth);

    static void DrawLine(const Math::line& line, const float size = 1.0f, const Math::vec4 color = {1.0f, 0.0f, 0.0f, 1.0f}, uint32_t renderFlags = CheckDepth);

    static void DrawQuad(const Math::point& a, const Math::point& b, const Math::point& c, const Math::point& d, const Math::vec4 color = {1.0f, 0.0f, 0.0f, 1.0f}, uint32_t renderFlags = CheckDepth);

    static void DrawBox(const Math::bbox& box, const Math::vec4& color, uint32_t renderFlags = CheckDepth|Wireframe);

    static void DrawOrientedBox(const Math::mat4& transform, const Math::bbox& box, const Math::vec4& color, uint32_t renderFlags = CheckDepth | Wireframe);

    static void DrawBox(const Math::mat4& modelTransform, const Math::vec4& color, uint32_t renderFlags = CheckDepth | Wireframe);
    /// draw a sphere
    static void DrawSphere(const Math::mat4& modelTransform, const Math::vec4& color, uint32_t renderFlags = CheckDepth | Wireframe);
    /// draw a sphere
    static void DrawSphere(const Math::point& pos, float radius, const Math::vec4& color, uint32_t renderFlags = CheckDepth | Wireframe);
    /// draw a cylinder
    static void DrawCylinder(const Math::point& start, const Math::point& end, float radius, const Math::vec4& color);
    /// draw a cone
    static void DrawCone(const Math::point& start, const Math::point& end, float startRadius, float endRadius, const Math::vec4& color, uint32_t renderFlags = CheckDepth | Wireframe);

    /// called before frame
    static void OnPrepareView(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    /// called when rendering a frame batch
    static void Render(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex);
           
    /// handle event
    static bool HandleInput(const Input::InputEvent& event);

    ///
    static void SetGridStatus(bool enable);
    ///
    static bool GetGridStatus();
    ///
    static void SetGridSize(float cellSize, int cellCount);
    ///
    static void GetGridSize(float& cellSize, int& cellCount);
    ///
    static void SetGridColor(Math::vec4 const& color);
    ///
    static const Math::vec4& GetGridColor();
    ///
    static void SetGridOffset(Math::vec2 const& offset);
    ///
    static Math::vec2 const& GetGridOffset();
    /// configure size and thickness of gizmos
    static void SetGizmoSize(int size, int width);

    /// configure size and thickness of gizmos
    static void GetGizmoSize(int& size, int& width);
    ///
    static void SetViewportRect(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize);
};


} // namespace Im3d
