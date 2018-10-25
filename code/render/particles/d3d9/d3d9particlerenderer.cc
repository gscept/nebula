//------------------------------------------------------------------------------
//  d3d9particlerenderer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#ifndef __DX11__
#include "particles/d3d9/d3d9particlerenderer.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/memoryvertexbufferloader.h"
#include "coregraphics/memoryindexbufferloader.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/vertexlayoutserver.h"

namespace Direct3D9
{
__ImplementClass(Direct3D9::D3D9ParticleRenderer, 'D9PR', Base::ParticleRendererBase);
__ImplementSingleton(Direct3D9::D3D9ParticleRenderer);

using namespace Util;
using namespace Base;
using namespace CoreGraphics;
using namespace Resources;
using namespace Particles;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
D3D9ParticleRenderer::D3D9ParticleRenderer() :
    curParticleVertexIndex(0),
    mappedVertices(0),
    curVertexPtr(0)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
D3D9ParticleRenderer::~D3D9ParticleRenderer()
{
    if (this->IsValid())
    {
        this->Discard();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D9ParticleRenderer::Setup()
{
    n_assert(!this->IsValid());
    n_assert(!this->particleVertexBuffer.isvalid());
    n_assert(!this->cornerVertexBuffer.isvalid());
    n_assert(!this->cornerIndexBuffer.isvalid());

    ParticleRendererBase::Setup();
    this->curParticleVertexIndex = 0;
    this->mappedVertices = 0;
    this->curVertexPtr = 0;

    // we need to create 2 vertex buffers and 1 index buffer:
    //  - one static vertex buffer which contains 4 corner vertices
    //  - one dynamic vertex buffer with one vertex per particle
    //  - one index buffer which 6 indices, forming 2 triangles

    // setup the corner vertex buffer
    Array<VertexComponent> cornerComponents;
    cornerComponents.Append(VertexComponent(VertexComponent::TexCoord, 0, VertexComponent::Short2, 0));
    short cornerVertexData[] = { 0, 0,  1, 0,  1, 1,  0, 1 };
    Ptr<MemoryVertexBufferLoader> cornerVBLoader = MemoryVertexBufferLoader::Create();
    cornerVBLoader->Setup(cornerComponents, 4, cornerVertexData, sizeof(cornerVertexData), VertexBuffer::UsageImmutable, VertexBuffer::AccessNone);

    this->cornerVertexBuffer = VertexBuffer::Create();
    this->cornerVertexBuffer->SetLoader(cornerVBLoader.upcast<ResourceLoader>());
    this->cornerVertexBuffer->SetAsyncEnabled(false);
    this->cornerVertexBuffer->Load();
    if (!this->cornerVertexBuffer->IsLoaded())
    {
        n_error("D3D9ParticleRenderer: Failed to setup corner vertex buffer!");
    }
    this->cornerVertexBuffer->SetLoader(0);

    // setup the corner index buffer
    ushort cornerIndexData[] = { 0, 2, 1, 2, 0, 3 };
    Ptr<MemoryIndexBufferLoader> cornerIBLoader = MemoryIndexBufferLoader::Create();
    cornerIBLoader->Setup(IndexType::Index16, 6, cornerIndexData, sizeof(cornerIndexData), IndexBuffer::UsageImmutable, IndexBuffer::AccessNone);

    this->cornerIndexBuffer = IndexBuffer::Create();
    this->cornerIndexBuffer->SetLoader(cornerIBLoader.upcast<ResourceLoader>());
    this->cornerIndexBuffer->SetAsyncEnabled(false);
    this->cornerIndexBuffer->Load();
    if (!this->cornerIndexBuffer->IsLoaded())
    {
        n_error("D3D9ParticleRenderer: Failed to setup corner index buffer!");
    }
    this->cornerIndexBuffer->SetLoader(0);

    // setup the cornerPrimitiveGroup which describes one particle instance
    this->primGroup.SetBaseVertex(0);
    this->primGroup.SetNumVertices(4);
    this->primGroup.SetBaseIndex(0);
    this->primGroup.SetNumIndices(6);
    this->primGroup.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    // setup the dynamic particle vertex buffer (contains one vertex per particle)
    Array<VertexComponent> particleComponents;
    particleComponents.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float4, 1));   // Particle::position
    particleComponents.Append(VertexComponent(VertexComponent::Position, 1, VertexComponent::Float4, 1));   // Particle::stretchPosition
    particleComponents.Append(VertexComponent(VertexComponent::Color, 0, VertexComponent::Float4, 1));      // Particle::color
    particleComponents.Append(VertexComponent(VertexComponent::TexCoord, 1, VertexComponent::Float4, 1));   // Particle::uvMinMax
    particleComponents.Append(VertexComponent(VertexComponent::TexCoord, 2, VertexComponent::Float4, 1));   // x: Particle::rotation, y: Particle::size

    Ptr<MemoryVertexBufferLoader> particleVBLoader = MemoryVertexBufferLoader::Create();
    particleVBLoader->Setup(particleComponents, MaxNumRenderedParticles, NULL, 0, VertexBuffer::UsageDynamic, VertexBuffer::AccessWrite);

    this->particleVertexBuffer = VertexBuffer::Create();
    this->particleVertexBuffer->SetLoader(particleVBLoader.upcast<ResourceLoader>());
    this->particleVertexBuffer->SetAsyncEnabled(false);
    this->particleVertexBuffer->Load();
    if (!this->particleVertexBuffer->IsLoaded())
    {
        n_error("D3D9ParticleRenderer: Failed to setup particle vertex buffer!");
    }
    this->particleVertexBuffer->SetLoader(0);

    // we need to setup a common vertex layout which describes both streams
    Array<VertexComponent> vertexComponents;
    vertexComponents = cornerComponents;
    vertexComponents.AppendArray(particleComponents);
    this->vertexLayout = VertexLayoutServer::Instance()->CreateSharedVertexLayout(vertexComponents);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D9ParticleRenderer::Discard()
{
    n_assert(this->IsValid());

    this->cornerVertexBuffer->Unload();
    this->cornerVertexBuffer = 0;

    this->cornerIndexBuffer->Unload();
    this->cornerIndexBuffer->SetLoader(0);
    this->cornerIndexBuffer = 0;

    this->particleVertexBuffer->Unload();
    this->particleVertexBuffer = 0;

    this->vertexLayout = 0;

    ParticleRendererBase::Discard();
}

//------------------------------------------------------------------------------
/**
    This method is called once per frame before visible particle systems are
    attached.
*/
void
D3D9ParticleRenderer::BeginAttach()
{
	
    n_assert(!this->inAttach);
    n_assert(0 == this->mappedVertices);
    this->inAttach = true;
    this->mappedVertices = this->particleVertexBuffer->Map(VertexBuffer::MapWriteDiscard);
    n_assert(0 != this->mappedVertices);
    this->curVertexPtr = this->mappedVertices;
    this->curParticleVertexIndex = 0;
	
}

//------------------------------------------------------------------------------
/**
    This method is called once per frame after visible particle systems are
    attached.
*/
void
D3D9ParticleRenderer::EndAttach()
{
	
    n_assert(this->inAttach);
	n_assert(0 != this->mappedVertices);
    this->inAttach = false;

    // unmap particle vertex buffer, since we're done with writing particle vertices
    this->particleVertexBuffer->Unmap();
    this->mappedVertices = 0;
    this->curVertexPtr = 0;
    this->curParticleVertexIndex = 0;
	
}

} // namespace Direct3D9

#endif