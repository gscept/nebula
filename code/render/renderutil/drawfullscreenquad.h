#pragma once
//------------------------------------------------------------------------------
/**
    @class RenderUtil::DrawFullScreenQuad
    
    Actually draws one big triangle which covers the entire screen, which
	is more efficient than drawing a quad using two triangles.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/shader.h"
#include "coregraphics/buffer.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/primitivegroup.h"

namespace RenderUtil
{
class DrawFullScreenQuad
{
public:

    /// setup the object
    static void Setup();
    /// discard the object
    static void Discard();
    /// return true if object is valid
    static bool IsValid();

	/// apply mesh
	static void ApplyMesh();

private:
    static CoreGraphics::BufferId vertexBuffer;
	static CoreGraphics::VertexLayoutId vertexLayout;
    static CoreGraphics::PrimitiveGroup primGroup;
    static bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
DrawFullScreenQuad::IsValid()
{
    return DrawFullScreenQuad::isValid;
}

} // namespace RnederUtil
//------------------------------------------------------------------------------
