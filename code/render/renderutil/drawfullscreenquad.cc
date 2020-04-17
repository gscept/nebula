//------------------------------------------------------------------------------
//  drawfullscreenquad.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/graphicsdevice.h"

namespace RenderUtil
{
using namespace Util;
using namespace CoreGraphics;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
DrawFullScreenQuad::DrawFullScreenQuad() :
    isValid(false)
{
    // empty
}    

//------------------------------------------------------------------------------
/**
*/
DrawFullScreenQuad::~DrawFullScreenQuad()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DrawFullScreenQuad::Setup(SizeT rtWidth, SizeT rtHeight)
{
    n_assert(!this->IsValid());
    this->isValid = true;

    // setup vertex components
    Array<VertexComponent> vertexComponents;
    vertexComponents.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float3));
    vertexComponents.Append(VertexComponent(VertexComponent::TexCoord1, 0, VertexComponent::Float2));
    
#if COREGRAPHICS_PIXEL_CENTER_HALF_PIXEL
	// compute screen rectangle coordinates
	Math::vec4 pixelSize(1.0f / float(rtWidth), 1.0f / float(rtHeight), 0.0f, 0.0f);
    Math::vec4 halfPixelSize = pixelSize * 0.5f;
#else
	Math::vec4 halfPixelSize = 0.0f;
#endif

	// create corners and uvs
    float left   = -1.0f - halfPixelSize.x;
    float right  = +3.0f - halfPixelSize.x;
    float top    = +3.0f + halfPixelSize.y;
    float bottom = -1.0f + halfPixelSize.y;

    float u0 = 0.0f;
    float u1 = 2.0f;
    float v0 = 0.0f;
    float v1 = 2.0f;

    // setup a vertex buffer with 2 triangles
    float v[3][5];

#if (__VULKAN__ || __DX12__)
	// first triangle
	v[0][0] = left;		v[0][1] = bottom;	v[0][2] = 0.0f; v[0][3] = u0; v[0][4] = v0;
	v[1][0] = left;		v[1][1] = top;		v[1][2] = 0.0f; v[1][3] = u0; v[1][4] = v1;
	v[2][0] = right;	v[2][1] = bottom;	v[2][2] = 0.0f; v[2][3] = u1; v[2][4] = v0;
#else
	// first triangle
	v[0][0] = left;		v[0][1] = top;		v[0][2] = 0.0f; v[0][3] = u0; v[0][4] = v0;
	v[1][0] = left;		v[1][1] = bottom;	v[1][2] = 0.0f; v[1][3] = u0; v[1][4] = v1;
	v[2][0] = right;	v[2][1] = top;		v[2][2] = 0.0f; v[2][3] = u1; v[2][4] = v0;
#endif

    // load vertex buffer
	VertexBufferCreateInfo info =
	{
		"FullScreen Quad VBO"_atm,
		GpuBufferTypes::AccessNone, GpuBufferTypes::UsageImmutable, GpuBufferTypes::SyncingManual,
		3, vertexComponents, v, sizeof(v)
	};
	this->vertexBuffer = CreateVertexBuffer(info);
	this->vertexLayout = VertexBufferGetLayout(this->vertexBuffer);

    // setup a primitive group object
    this->primGroup.SetBaseVertex(0);
    this->primGroup.SetNumVertices(3);
    this->primGroup.SetBaseIndex(0);
    this->primGroup.SetNumIndices(0);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawFullScreenQuad::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;
	DestroyVertexBuffer(this->vertexBuffer);
	DestroyVertexLayout(this->vertexLayout);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawFullScreenQuad::ApplyMesh()
{
	// setup pipeline
	CoreGraphics::SetVertexLayout(this->vertexLayout);
	CoreGraphics::SetPrimitiveTopology(PrimitiveTopology::TriangleList);
	CoreGraphics::SetGraphicsPipeline();

	// setup input data
	CoreGraphics::SetStreamVertexBuffer(0, this->vertexBuffer, 0);
	CoreGraphics::SetPrimitiveGroup(this->primGroup);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawFullScreenQuad::Draw()
{
	CoreGraphics::Draw();
}

} // namespace RenderUtil
