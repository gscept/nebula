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

CoreGraphics::BufferId DrawFullScreenQuad::vertexBuffer = CoreGraphics::BufferId::Invalid();
CoreGraphics::VertexLayoutId DrawFullScreenQuad::vertexLayout = CoreGraphics::VertexLayoutId::Invalid();
CoreGraphics::PrimitiveGroup DrawFullScreenQuad::primGroup;
bool DrawFullScreenQuad::isValid = false;

//------------------------------------------------------------------------------
/**
*/
void
DrawFullScreenQuad::Setup()
{
    n_assert(!DrawFullScreenQuad::IsValid());
    DrawFullScreenQuad::isValid = true;

    if (DrawFullScreenQuad::vertexBuffer == CoreGraphics::BufferId::Invalid())
    {
        // setup vertex components
        Array<VertexComponent> vertexComponents;
        vertexComponents.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float3));
        vertexComponents.Append(VertexComponent(VertexComponent::TexCoord1, 0, VertexComponent::Float2));

        // create corners and uvs
        float left = -1.0f;
        float right = +3.0f;
        float top = +3.0f;
        float bottom = -1.0f;

        float u0 = 0.0f;
        float u1 = 2.0f;
        float v0 = 0.0f;
        float v1 = 2.0f;

        // setup a vertex buffer with 2 triangles
        float v[3][5];

#if (__VULKAN__ || __DX12__)
        // first triangle
        v[0][0] = left;     v[0][1] = bottom;   v[0][2] = 0.0f; v[0][3] = u0; v[0][4] = v0;
        v[1][0] = left;     v[1][1] = top;      v[1][2] = 0.0f; v[1][3] = u0; v[1][4] = v1;
        v[2][0] = right;    v[2][1] = bottom;   v[2][2] = 0.0f; v[2][3] = u1; v[2][4] = v0;
#else
        // first triangle
        v[0][0] = left;     v[0][1] = top;      v[0][2] = 0.0f; v[0][3] = u0; v[0][4] = v0;
        v[1][0] = left;     v[1][1] = bottom;   v[1][2] = 0.0f; v[1][3] = u0; v[1][4] = v1;
        v[2][0] = right;    v[2][1] = top;      v[2][2] = 0.0f; v[2][3] = u1; v[2][4] = v0;
#endif

        DrawFullScreenQuad::vertexLayout = CoreGraphics::CreateVertexLayout({ vertexComponents });

        // load vertex buffer
        BufferCreateInfo info;
        info.name = "FullScreen Quad VBO"_atm;
        info.size = 3;
        info.elementSize = CoreGraphics::VertexLayoutGetSize(DrawFullScreenQuad::vertexLayout);
        info.mode = CoreGraphics::DeviceLocal;
        info.usageFlags = CoreGraphics::VertexBuffer;
        info.data = v;
        info.dataSize = sizeof(v);
        DrawFullScreenQuad::vertexBuffer = CreateBuffer(info);

        // setup a primitive group object
        DrawFullScreenQuad::primGroup.SetBaseVertex(0);
        DrawFullScreenQuad::primGroup.SetNumVertices(3);
        DrawFullScreenQuad::primGroup.SetBaseIndex(0);
        DrawFullScreenQuad::primGroup.SetNumIndices(0);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DrawFullScreenQuad::Discard()
{
    n_assert(DrawFullScreenQuad::IsValid());
    DrawFullScreenQuad::isValid = false;
    DestroyBuffer(DrawFullScreenQuad::vertexBuffer);
    DestroyVertexLayout(DrawFullScreenQuad::vertexLayout);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawFullScreenQuad::ApplyMesh()
{
    // setup pipeline
    CoreGraphics::SetVertexLayout(DrawFullScreenQuad::vertexLayout);
    CoreGraphics::SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    CoreGraphics::SetGraphicsPipeline();

    // setup input data
    CoreGraphics::SetStreamVertexBuffer(0, DrawFullScreenQuad::vertexBuffer, 0);
    CoreGraphics::SetPrimitiveGroup(DrawFullScreenQuad::primGroup);
}

} // namespace RenderUtil
