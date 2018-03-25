//------------------------------------------------------------------------------
//  drawfullscreenquad.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/memoryvertexbufferpool.h"
#include "coregraphics/renderdevice.h"

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
	Math::float4 pixelSize(1.0f / float(rtWidth), 1.0f / float(rtHeight), 0.0f, 0.0f);
    Math::float4 halfPixelSize = pixelSize * 0.5f;
#else
	Math::float4 halfPixelSize = 0.0f;
#endif

    float left   = -1.0f - halfPixelSize.x();
    float right  = +3.0f - halfPixelSize.x();
    float top    = +3.0f + halfPixelSize.y();
    float bottom = -1.0f + halfPixelSize.y();


    // compute uv coordinates, for GL and Vulkan, the Y coordinate is reversed
#if (__VULKAN__ || __OGL4__)
	float u0 = 1.0f;
	float u1 = 3.0f;
	float v0 = 3.0f;
	float v1 = 1.0f;
#else
    float u0 = 1.0f;
    float u1 = 3.0f;
    float v0 = 1.0f;
    float v1 = 3.0f;
#endif

    // setup a vertex buffer with 2 triangles
    float v[3][5];

#if COREGRAPHICS_TRIANGLE_FRONT_FACE_CCW
    // first triangle
    v[0][0] = left;  v[0][1] = top;		v[0][2] = 0.0f; v[0][3] = u0; v[0][4] = v0;
    v[1][0] = right; v[1][1] = top;		v[1][2] = 0.0f; v[1][3] = u1; v[1][4] = v0;
    v[2][0] = left;  v[2][1] = bottom;  v[2][2] = 0.0f; v[2][3] = u0; v[2][4] = v1;

#else
	// first triangle
	v[0][0] = left;  v[0][1] = top;    v[0][2] = 0.0f; v[0][3] = u0; v[0][4] = v0;
	v[1][0] = right; v[1][1] = top;    v[1][2] = 0.0f; v[1][3] = u1; v[1][4] = v0;
	v[2][0] = left;  v[2][1] = bottom; v[2][2] = 0.0f; v[2][3] = u0; v[2][4] = v1;
#endif

    // setup vertex buffer with memory-vertexbuffer-loader
	
	VertexBufferCreateInfo info =
	{
		"fsq_vbo", "render_system",
		GpuBufferTypes::AccessNone, GpuBufferTypes::UsageImmutable, GpuBufferTypes::SyncingFlush,
		3, vertexComponents, v, sizeof(v)
	};
	this->vertexBuffer = CreateVertexBuffer(info);

	this->vertexLayout = CreateVertexLayout(VertexLayoutCreateInfo{ vertexComponents });

    // setup a primitive group object
    this->primGroup.SetBaseVertex(0);
    this->primGroup.SetNumVertices(6);
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
	RenderDevice* renderDevice = RenderDevice::Instance();
	renderDevice->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
	VertexLayoutBind(this->vertexLayout);
	VertexBufferBind(this->vertexBuffer, 0, 0);
	renderDevice->SetPrimitiveGroup(this->primGroup);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawFullScreenQuad::Draw()
{
    RenderDevice* renderDevice = RenderDevice::Instance();
    renderDevice->Draw();
}

} // namespace RenderUtil
