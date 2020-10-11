#pragma once
//------------------------------------------------------------------------------
/**
    @class Im3dContext

    Nebula renderer for Im3d gizmos

    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "coregraphics/constantbuffer.h"
#include "coregraphics/buffer.h"
#include "graphics/graphicscontext.h"
#include "input/inputevent.h"
#include "math/bbox.h"

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
    _DeclarePluginContext();
public:
    /// constructor
    Im3dContext();
    /// destructor
    virtual ~Im3dContext();

    static void Create();
    static void Discard();

    static void DrawBox(const Math::bbox& box, const Math::vec4& color, uint32_t depthFlag = CheckDepth|Wireframe);

    static void DrawOrientedBox(const Math::mat4& transform, const Math::bbox& box, const Math::vec4& color, uint32_t depthFlag = CheckDepth | Wireframe);

    static void DrawBox(const Math::mat4& modelTransform, const Math::vec4& color, uint32_t depthFlag = CheckDepth | Wireframe);
    /// draw a sphere
    static void DrawSphere(const Math::mat4& modelTransform, const Math::vec4& color, uint32_t depthFlag = CheckDepth | Wireframe);
    /// draw a cylinder
    static void DrawCylinder(const Math::mat4& modelTransform, const Math::vec4& color, uint32_t depthFlag = CheckDepth | Wireframe);
    /// draw a cone
    static void DrawCone(const Math::mat4& modelTransform, const Math::vec4& color, uint32_t depthFlag = CheckDepth | Wireframe);

    /// Start a new Im3d frame
    static void OnBeforeFrame(const Graphics::FrameContext& ctx);

    /// called before frame
    static void OnPrepareView(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    /// called when rendering a frame batch
    static void Render(const IndexT frameIndex);
           
    /// handle event
    static bool HandleInput(const Input::InputEvent& event);

    ///
    static void SetGridStatus(bool enable);
    ///
    static void SetGridSize(float cellSize, int cellCount);
    ///
    static void SetGridColor(Math::vec4 const& color);
    ///
    static void SetGridOffset(Math::vec2 const& offset);
    /// configure size and thickness of gizmos
    static void SetGizmoSize(int size, int width);
};


} // namespace Im3d
