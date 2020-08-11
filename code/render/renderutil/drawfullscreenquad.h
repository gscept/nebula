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
    /// constructor
    DrawFullScreenQuad();
    /// destructor
    ~DrawFullScreenQuad();

    /// setup the object
    void Setup(SizeT rtWidth, SizeT rtHeight);
    /// discard the object
    void Discard();
    /// return true if object is valid
    bool IsValid() const;

	/// apply mesh
	void ApplyMesh();
    /// draw the fullscreen quad
    void Draw();

private:
    CoreGraphics::BufferId vertexBuffer;
	CoreGraphics::VertexLayoutId vertexLayout;
    CoreGraphics::PrimitiveGroup primGroup;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
DrawFullScreenQuad::IsValid() const
{
    return this->isValid;
}

} // namespace RnederUtil
//------------------------------------------------------------------------------
