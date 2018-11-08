#pragma once
//------------------------------------------------------------------------------
/**
    @class Im3dContext

    Nebula renderer for Im3d gizmos

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "coregraphics/constantbuffer.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "graphics/graphicscontext.h"
#include "input/inputevent.h"

namespace Im3d
{

class Im3dContext : public Graphics::GraphicsContext
{
    _DeclareContext();
public:
    /// constructor
    Im3dContext();
    /// destructor
    virtual ~Im3dContext();

    static void Create();
    static void Discard();

    /// called before frame
    static void OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
    /// called when rendering a frame batch
    static void OnRenderAsPlugin(const IndexT frameIndex, const Timing::Time frameTime, const Util::StringAtom& filter);
           
    /// handle event
    static bool HandleInput(const Input::InputEvent& event);

    ///
    static void SetGridStatus(bool enable);
    ///
    static void SetGridSize(float cellSize, int cellCount);
    ///
    static void SetGridColor(Math::float4 const& color);
    /// configure size and thickness of gizmos
    static void SetGizmoSize(int size, int width);
};


} // namespace Im3d